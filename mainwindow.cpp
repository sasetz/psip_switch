#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "network_switch.h"
#include "shared_storage.h"

#include <QTimer>
#include <tins/hw_address.h>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui_m(new Ui::MainWindow), interfaces_m{}, configurationTimer_m{this},
      threadTimer_m{this},
    networkSwitch_m{}
{
    ui_m->setupUi(this);

    connect(ui_m->start_stop_button, &QAbstractButton::released, this, &MainWindow::onStartStopButtonClicked);
    connect(ui_m->clear_macs, &QAbstractButton::released, this, &MainWindow::onClearMacsClicked);
    connect(ui_m->clear_stats, &QAbstractButton::released, this, &MainWindow::onClearStatsClicked);

    connect(&configurationTimer_m, &QTimer::timeout, this, &MainWindow::updateInterfaces);
    connect(&threadTimer_m, &QTimer::timeout, this, &MainWindow::refreshUi);
    configurationTimer_m.start(1000);
    updateInterfaces();
    ui_m->mac_table->setSortingEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui_m;
}

void MainWindow::onStartStopButtonClicked()
{
    if (networkSwitch_m.state() == NetworkSwitch::SwitchState::RunningNetwork)
    {
        stopThread();
    }
    else
    {
        startThread();
    }
}

void MainWindow::startThread()
{
    qInfo("Attempting to activate the switch...");
    if (ui_m->interface1->currentIndex() == -1 || ui_m->interface2->currentIndex() == -1)
    {
        qInfo("One or more ComboBoxes have no selected interfaces. Aborting.");
        return;
    }
    std::size_t if1_index = ui_m->interface1->currentIndex();
    std::size_t if2_index = ui_m->interface2->currentIndex();
    if (if1_index == if2_index)
    {
        qInfo("The chosen interfaces are identical. Aborting.");
        return;
    }

    if (if1_index >= interfaces_m.size() || if2_index >= interfaces_m.size())
    {
        qDebug("Ui list contains more elements than the internal state! Aborting.");
        return;
    }

    networkSwitch_m.startNetwork(interfaces_m[if1_index].name(), interfaces_m[if2_index].name());
    refreshUi();
    threadTimer_m.start(500);
}

void MainWindow::stopThread()
{
    networkSwitch_m.stopNetwork();
    refreshUi();
}

void MainWindow::refreshUi()
{
    auto index1 = ui_m->interface1->currentIndex();
    auto index2 = ui_m->interface2->currentIndex();

    ui_m->interface1->clear();
    ui_m->interface2->clear();
    QStringList list;
    for (auto interface : interfaces_m)
    {
        list << (interface.name()).c_str();
    }
    ui_m->interface1->addItems(list);
    ui_m->interface2->addItems(list);
    ui_m->interface1->setCurrentIndex(index1);
    ui_m->interface2->setCurrentIndex(index2);

    if (networkSwitch_m.state() == NetworkSwitch::SwitchState::RunningNetwork)
    {
        ui_m->interface1->setEnabled(false);
        ui_m->interface2->setEnabled(false);
        ui_m->start_stop_button->setText("Stop");
        ui_m->start_stop_button->setEnabled(true);
        auto activeInterfaces = networkSwitch_m.interfaces();
        ui_m->status->setText(
            QString("Running on interfaces: %1 -> %2")
                .arg(activeInterfaces.first.c_str(), activeInterfaces.second.c_str()));
    }
    else if (networkSwitch_m.state() == NetworkSwitch::SwitchState::Idle)
    {
        ui_m->interface1->setEnabled(true);
        ui_m->interface2->setEnabled(true);
        ui_m->start_stop_button->setText("Start");
        ui_m->start_stop_button->setEnabled(true);
        ui_m->status->setText("Idle");
        threadTimer_m.stop(); // the UI won't be updated until the start button is pressed again
    }
    else if (networkSwitch_m.state() == NetworkSwitch::SwitchState::StoppingNetwork)
    {
        ui_m->interface1->setEnabled(false);
        ui_m->interface2->setEnabled(false);
        ui_m->start_stop_button->setText("Stopping...");
        ui_m->start_stop_button->setEnabled(false);
        auto activeInterfaces = networkSwitch_m.interfaces();
        ui_m->status->setText(
            QString("Running on interfaces: %1 -> %2 (finishing...)")
                .arg(activeInterfaces.first.c_str(), activeInterfaces.second.c_str()));
    }
    else
    {
        qDebug("An unknown UI state detected!");
    }
    qDebug("Refreshing mac table...");

    ui_m->mac_table->clearContents();
    ui_m->stats_table->clearContents();

    auto guard = networkSwitch_m.getStorage().guard();
    auto macLength = guard.storage.macTable.size();
    ui_m->mac_table->setRowCount(macLength);

    int i = 0;
    for (const auto & entry : guard.storage.macTable)
    {
        auto *item = new QTableWidgetItem(tr("%1").arg(entry.first.to_string().c_str()));
        // item->setText(tr("%1").arg(entry.first.to_string().c_str()));
        ui_m->mac_table->setItem(i, 0, item);
        item = new QTableWidgetItem(tr("%1").arg(entry.second.interface.name().c_str()));
        // item->setText(tr("%1").arg(get<0>(entry.second).name().c_str()));
        ui_m->mac_table->setItem(i, 1, item);
        item = new QTableWidgetItem(tr("%1ms").arg(
            entry.second.expiration.timeLeft().count())
                                    );
        // item->setText(tr("%1ms").arg(duration_cast<milliseconds>(get<1>(entry.second) -
        // steady_clock::now()).count()));
        ui_m->mac_table->setItem(i, 2, item);
        i++;
    }

    auto statLength = guard.storage.statisticsTable.size();
    ui_m->stats_table->setRowCount(statLength);
    int j = 0;
    for (const auto & entry : guard.storage.statisticsTable)
    {
        for (const auto & net : entry.second.table)
        {
            ui_m->stats_table->setItem(j, 0, new QTableWidgetItem(tr("%1").arg(net.first.hw_address().to_string().c_str())));
            ui_m->stats_table->setItem(j, 1, new QTableWidgetItem(tr("%1").arg(protocolToString(entry.first).c_str())));
            ui_m->stats_table->setItem(j, 2, new QTableWidgetItem(tr("%1").arg(net.second.input)));
            ui_m->stats_table->setItem(j, 3, new QTableWidgetItem(tr("%1").arg(net.second.output)));
            j++;
        }
    }
}

void MainWindow::updateInterfaces()
{
    qInfo("Querying for network interfaces...");
    auto current = Tins::NetworkInterface::all();
    if (current != interfaces_m)
    {
        interfaces_m = current;
        qInfo("Configuration updated.");
        refreshUi();
    }

    networkSwitch_m.updateMac();
}

void MainWindow::onClearMacsClicked()
{
    networkSwitch_m.clearMac();
    refreshUi();
}

void MainWindow::onClearStatsClicked()
{
    networkSwitch_m.clearStats();
    refreshUi();
}

#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QTimer>

using std::get;
using std::chrono::duration_cast, std::chrono::milliseconds, std::chrono::steady_clock;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui_m(new Ui::MainWindow), interfaces_m{}, switchHandle_m{}, configurationTimer_m{this},
      threadTimer_m{this}
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
    if (switchHandle_m.running())
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

    switchHandle_m.launch(interfaces_m[if1_index], interfaces_m[if2_index]);
    refreshUi();
    threadTimer_m.start(500);
}

void MainWindow::stopThread()
{
    switchHandle_m.stop();
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

    if (switchHandle_m.running())
    {
        ui_m->interface1->setEnabled(false);
        ui_m->interface2->setEnabled(false);
        ui_m->start_stop_button->setText("Stop");
        ui_m->start_stop_button->setEnabled(true);
        auto activeInterfaces = switchHandle_m.interfaces();
        ui_m->status->setText(
            QString("Running on interfaces: %1 -> %2")
                .arg(std::get<0>(activeInterfaces).name().c_str(), std::get<1>(activeInterfaces).name().c_str()));
    }
    else if (switchHandle_m.state() == network::SwitchHandle::State::Idle)
    {
        ui_m->interface1->setEnabled(true);
        ui_m->interface2->setEnabled(true);
        ui_m->start_stop_button->setText("Start");
        ui_m->start_stop_button->setEnabled(true);
        ui_m->status->setText("Idle");
        threadTimer_m.stop(); // the UI won't be updated until the start button is pressed again
    }
    else if (switchHandle_m.state() == network::SwitchHandle::State::Stopping)
    {
        ui_m->interface1->setEnabled(false);
        ui_m->interface2->setEnabled(false);
        ui_m->start_stop_button->setText("Stopping...");
        ui_m->start_stop_button->setEnabled(false);
        auto activeInterfaces = switchHandle_m.interfaces();
        ui_m->status->setText(
            QString("Running on interfaces: %1 -> %2 (finishing...)")
                .arg(std::get<0>(activeInterfaces).name().c_str(), std::get<1>(activeInterfaces).name().c_str()));
    }
    else
    {
        qDebug("An unknown UI state detected!");
    }
    qDebug("Refreshing mac table...");

    ui_m->mac_table->clearContents();
    ui_m->stats_table->clearContents();

    auto data = switchHandle_m.copySharedData();
    auto macLength = data.macTable.size();
    ui_m->mac_table->setRowCount(macLength);

    int i = 0;
    for (const auto & entry : data.macTable)
    {
        auto *item = new QTableWidgetItem(tr("%1").arg(entry.first.to_string().c_str()));
        // item->setText(tr("%1").arg(entry.first.to_string().c_str()));
        ui_m->mac_table->setItem(i, 0, item);
        item = new QTableWidgetItem(tr("%1").arg(get<0>(entry.second).name().c_str()));
        // item->setText(tr("%1").arg(get<0>(entry.second).name().c_str()));
        ui_m->mac_table->setItem(i, 1, item);
        item = new QTableWidgetItem(tr("%1ms").arg(
            duration_cast<milliseconds>(get<1>(entry.second) - steady_clock::now() + network::SwitchHandle::MAC_TIMEOUT)
                .count()));
        // item->setText(tr("%1ms").arg(duration_cast<milliseconds>(get<1>(entry.second) -
        // steady_clock::now()).count()));
        ui_m->mac_table->setItem(i, 2, item);
        i++;
    }

    auto statLength = data.stats.size();
    ui_m->stats_table->setRowCount(statLength);
    int j = 0;
    for (const auto & entry : data.stats)
    {
        ui_m->stats_table->setItem(j, 0, new QTableWidgetItem(tr("%1").arg(entry.interface.name().c_str())));
        ui_m->stats_table->setItem(j, 1, new QTableWidgetItem(tr("%1").arg(protocolToStr(entry.protocol))));
        ui_m->stats_table->setItem(j, 2, new QTableWidgetItem(tr("%1").arg(entry.inputPackets)));
        ui_m->stats_table->setItem(j, 3, new QTableWidgetItem(tr("%1").arg(entry.outputPackets)));
        j++;
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

    switchHandle_m.updateMacTable();
}

void MainWindow::onClearMacsClicked()
{
    switchHandle_m.clearMacTable();
    refreshUi();
}

void MainWindow::onClearStatsClicked()
{
    switchHandle_m.clearStatsTable();
    refreshUi();
}

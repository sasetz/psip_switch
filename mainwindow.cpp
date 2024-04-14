#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "network_switch.h"
#include "settings.h"
#include "shared_storage.h"

#include <QAction>
#include <QTimer>
#include <cstddef>
#include <qaction.h>
#include <tins/hw_address.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui_m(new Ui::MainWindow),
      info_m(),
      interfaces_m{},
      configurationTimer_m{this},
      threadTimer_m{this},
      macTimer_m{this},
      packetsTimer_m{this},
      sessionTimer_m{this},
      networkSwitch_m{},
      firstModel{nullptr},
      secondModel{nullptr}
{
    ui_m->setupUi(this);

    connect(ui_m->start_stop_button, &QAbstractButton::released, this, &MainWindow::onStartStopButtonClicked);
    connect(ui_m->restStartStop, &QAbstractButton::released, this, &MainWindow::onRestStartStopButtonClicked);
    connect(ui_m->resetMacTimeoutsMenu, &QAction::triggered, this, &MainWindow::resetMac);
    connect(ui_m->clearMacMenu, &QAction::triggered, this, &MainWindow::clearMacTable);
    connect(ui_m->clearStatsMenu, &QAction::triggered, this, &MainWindow::clearStatsTable);
    connect(ui_m->interface1Clear, &QAbstractButton::released, this, &MainWindow::clearFirstStatsTable);
    connect(ui_m->interface2Clear, &QAbstractButton::released, this, &MainWindow::clearSecondStatsTable);

    connect(&configurationTimer_m, &QTimer::timeout, this, &MainWindow::updateInterfaces);
    connect(&threadTimer_m, &QTimer::timeout, this, &MainWindow::refreshUi);
    connect(&macTimer_m, &QTimer::timeout, this, &MainWindow::updateMac);
    connect(&packetsTimer_m, &QTimer::timeout, this, &MainWindow::updatePackets);
    connect(&sessionTimer_m, &QTimer::timeout, this, &MainWindow::updateSessions);
    macTimer_m.start(MAC_UPDATE_TIMER);
    packetsTimer_m.start(SENT_PACKETS_TIMER);
    sessionTimer_m.start(SESSION_UPDATE_TIMER);
    configurationTimer_m.start(INTERFACE_UPDATE_TIMER);
    updateInterfaces();

    info_m = new InfoTable(*this);
    info_m->show();
    connect(&threadTimer_m, &QTimer::timeout, info_m, &::InfoTable::refreshUi);
}

MainWindow::~MainWindow()
{
    delete ui_m;
    delete info_m;
    std::exit(0);
}

void MainWindow::onStartStopButtonClicked()
{
    if (networkSwitch_m.state() == NetworkSwitch::SwitchState::RunningNetwork)
    {
        stopNetworkThread();
    }
    else if (networkSwitch_m.state() == NetworkSwitch::SwitchState::RunningRest)
    {
        stopNetworkThread();
    }
    else if (networkSwitch_m.state() == NetworkSwitch::SwitchState::Idle)
    {
        startNetworkThread();
    }
}

void MainWindow::onRestStartStopButtonClicked()
{
    if (networkSwitch_m.state() == NetworkSwitch::SwitchState::RunningNetwork)
    {
        startRestThread();
    }
    else if (networkSwitch_m.state() == NetworkSwitch::SwitchState::RunningRest)
    {
        stopRestThread();
    }
}

void MainWindow::startNetworkThread()
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
    firstModel.reset(new StatisticsModel{networkSwitch_m.getStorage(), interfaces_m[if1_index], this});
    secondModel.reset(new StatisticsModel{networkSwitch_m.getStorage(), interfaces_m[if2_index], this});
    ui_m->interface1Statistics->setModel(firstModel.get());
    ui_m->interface2Statistics->setModel(secondModel.get());
    refreshUi();
    threadTimer_m.start(UI_REFRESH_TIMER);
}

void MainWindow::startRestThread()
{
    qInfo("Attempting to start REST...");
    networkSwitch_m.startRest(ui_m->port->value());
    refreshUi();
}

void MainWindow::stopThreads()
{
    networkSwitch_m.stopNetwork();
    networkSwitch_m.stopRest();
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
        ui_m->restStartStop->setEnabled(true);
        ui_m->restStartStop->setText("Start REST");
        ui_m->port->setEnabled(true);
        auto activeInterfaces = networkSwitch_m.interfaces();
        ui_m->status->setText(
            QString("Status: Running on interfaces: %1 <-> %2")
                .arg(activeInterfaces.first.identificator.c_str(), activeInterfaces.second.identificator.c_str())
        );
        ui_m->interface1Name->setText(QString("%1").arg(
            activeInterfaces.first.name.c_str()
        ));
        ui_m->interface2Name->setText(QString("%1").arg(
            activeInterfaces.second.name.c_str()
        ));
        ui_m->interface1Status->setText(QString("%1, %2").arg(
            activeInterfaces.first.identificator.c_str(),
            activeInterfaces.first.up ? "up" : "down"
        ));
        ui_m->interface2Status->setText(QString("%1, %2").arg(
            activeInterfaces.second.identificator.c_str(),
            activeInterfaces.second.up ? "up" : "down"
        ));
    }
    else if (networkSwitch_m.state() == NetworkSwitch::SwitchState::RunningRest)
    {
        ui_m->interface1->setEnabled(false);
        ui_m->interface2->setEnabled(false);
        ui_m->start_stop_button->setText("Stop");
        ui_m->start_stop_button->setEnabled(true);
        ui_m->restStartStop->setEnabled(true);
        ui_m->restStartStop->setText("Stop REST");
        ui_m->port->setEnabled(true);
        auto activeInterfaces = networkSwitch_m.interfaces();
        ui_m->status->setText(QString("Status: Running on interfaces: %1 <-> %2\nREST on port: %3")
                                  .arg(activeInterfaces.first.identificator.c_str(), activeInterfaces.second.identificator.c_str(), "8888"));
        ui_m->interface1Name->setText(QString("%1").arg(
            activeInterfaces.first.name.c_str()
        ));
        ui_m->interface2Name->setText(QString("%1").arg(
            activeInterfaces.second.name.c_str()
        ));
        ui_m->interface1Status->setText(QString("%1, %2").arg(
            activeInterfaces.first.identificator.c_str(),
            activeInterfaces.first.up ? "up" : "down"
        ));
        ui_m->interface2Status->setText(QString("%1, %2").arg(
            activeInterfaces.second.identificator.c_str(),
            activeInterfaces.second.up ? "up" : "down"
        ));
    }
    else if (networkSwitch_m.state() == NetworkSwitch::SwitchState::Idle)
    {
        ui_m->interface1->setEnabled(true);
        ui_m->interface2->setEnabled(true);
        ui_m->start_stop_button->setText("Start");
        ui_m->start_stop_button->setEnabled(true);
        ui_m->restStartStop->setEnabled(false);
        ui_m->restStartStop->setText("Start REST");
        ui_m->port->setEnabled(false);
        ui_m->status->setText("Status: Idle");
        threadTimer_m.stop(); // the UI won't be updated until the start button is pressed again
    }
    else if (networkSwitch_m.state() == NetworkSwitch::SwitchState::Stopping)
    {
        ui_m->interface1->setEnabled(false);
        ui_m->interface2->setEnabled(false);
        ui_m->start_stop_button->setText("Stopping...");
        ui_m->start_stop_button->setEnabled(false);
        ui_m->restStartStop->setEnabled(false);
        ui_m->restStartStop->setText("Stopping REST...");
        ui_m->port->setEnabled(false);
        auto activeInterfaces = networkSwitch_m.interfaces();
        ui_m->status->setText(QString("Status: Running on interfaces: %1 -> %2 (finishing...)")
                                  .arg(activeInterfaces.first.identificator.c_str(), activeInterfaces.second.identificator.c_str()));
    }
    else
    {
        qDebug("An unknown UI state detected!");
    }

    /* ui_m->mac_table->clearContents(); */
    /* ui_m->stats_table->clearContents(); */
    /**/
    /* auto guard = networkSwitch_m.getStorage().guard(); */
    /* auto macLength = guard.storage.macTable.size(); */
    /* ui_m->mac_table->setRowCount(macLength); */
    /**/
    /* int i = 0; */
    /* for (const auto & entry : guard.storage.macTable) */
    /* { */
    /*     auto *item = new QTableWidgetItem(tr("%1").arg(entry.first.to_string().c_str())); */
    /*     ui_m->mac_table->setItem(i, 0, item); */
    /*     item = new QTableWidgetItem(tr("%1").arg(entry.second.interface.name().c_str())); */
    /*     ui_m->mac_table->setItem(i, 1, item); */
    /*     item = new QTableWidgetItem(tr("%1ms").arg(entry.second.expiration.timeLeft().count())); */
    /*     ui_m->mac_table->setItem(i, 2, item); */
    /*     i++; */
    /* } */
    /**/
    /* auto statLength = guard.storage.statisticsTable.size(); */
    /* ui_m->stats_table->setRowCount(statLength); */
    /* int j = 0; */
    /* for (const auto & entry : guard.storage.statisticsTable) */
    /* { */
    /*     ui_m->stats_table->setItem( */
    /*         j, 0, new QTableWidgetItem(tr("%1").arg(entry.first.target.hw_address().to_string().c_str()))); */
    /*     ui_m->stats_table->setItem(j, 1, */
    /*                                new QTableWidgetItem(tr("%1").arg(protocolToString(entry.first.protocol).c_str()))); */
    /*     ui_m->stats_table->setItem(j, 2, new QTableWidgetItem(tr("%1").arg(entry.second.input))); */
    /*     ui_m->stats_table->setItem(j, 3, new QTableWidgetItem(tr("%1").arg(entry.second.output))); */
    /*     j++; */
    /* } */
}

void MainWindow::updateMac()
{
    networkSwitch_m.updateMac();
}

void MainWindow::updatePackets()
{
    networkSwitch_m.updatePackets();
}

void MainWindow::updateSessions()
{
    networkSwitch_m.updateSessions();
}

void MainWindow::updateInterfaces()
{
    auto current = Tins::NetworkInterface::all();
    if (current != interfaces_m)
    {
        interfaces_m = current;
        qInfo("Configuration updated.");
        refreshUi();
    }

    networkSwitch_m.updateMac();
}

void MainWindow::clearMacTable()
{
    networkSwitch_m.clearMac();
    refreshUi();
}

void MainWindow::clearStatsTable()
{
    networkSwitch_m.clearStats();
    refreshUi();
}

void MainWindow::clearFirstStatsTable()
{
    auto activeInterfaces = networkSwitch_m.interfaces();
    networkSwitch_m.clearStats(activeInterfaces.first.networkInterface);
    refreshUi();
}

void MainWindow::clearSecondStatsTable()
{
    auto activeInterfaces = networkSwitch_m.interfaces();
    networkSwitch_m.clearStats(activeInterfaces.second.networkInterface);
    refreshUi();
}

void MainWindow::resetMac()
{
    networkSwitch_m.resetMac();
    refreshUi();
}

void MainWindow::stopNetworkThread()
{
    networkSwitch_m.stopNetwork();
    networkSwitch_m.stopRest();
}

void MainWindow::stopRestThread()
{
    networkSwitch_m.stopRest();
}



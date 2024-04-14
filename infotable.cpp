#include "infotable.h"
#include "mainwindow.h"
#include "ui_infotable.h"
#include <chrono>

using std::chrono::duration_cast, std::chrono::seconds;

InfoTable::InfoTable(MainWindow & parent)
    : QMainWindow(&parent),
      ui_m(new Ui::InfoTable),
      networkSwitch_m(parent.networkSwitch_m),
      macModel_m{nullptr},
      sessionsModel_m{nullptr}
{
    ui_m->setupUi(this);
    connect(ui_m->acceptTimeout, &QAbstractButton::released, this, &InfoTable::onApplyTimeout);
    connect(ui_m->clearMac, &QAbstractButton::released, this, &InfoTable::onMacsClear);
    connect(ui_m->clearSessions, &QAbstractButton::released, this, &InfoTable::onSessionsClear);
    macModel_m.reset(new MacModel(parent.networkSwitch_m.getStorage(), this));
    sessionsModel_m.reset(new SessionsModel(parent.networkSwitch_m.getStorage(), this));
    ui_m->macTable->setModel(macModel_m.get());
    ui_m->sessionsTable->setModel(sessionsModel_m.get());
}

void InfoTable::refreshUi()
{
    if (networkSwitch_m.state() == NetworkSwitch::SwitchState::Idle)
    {
        ui_m->timeoutValue->setEnabled(false);
        ui_m->acceptTimeout->setEnabled(false);
    }
    else
    {
        ui_m->timeoutValue->setEnabled(true);
        ui_m->acceptTimeout->setEnabled(true);

        auto guard = networkSwitch_m.getStorage().guard();
        ui_m->currentTimeout->setText(
            QString("%1")
            .arg(duration_cast<seconds>(guard->deviceInfo.defaultMacTimeout).count())
        );
    }
}

void InfoTable::onApplyTimeout()
{
    networkSwitch_m.setMacTimeout(ui_m->timeoutValue->value());
}

void InfoTable::onMacsClear()
{
    networkSwitch_m.clearMac();
}

void InfoTable::onSessionsClear()
{
    networkSwitch_m.clearSessions();
}

InfoTable::~InfoTable()
{
    delete ui_m;
}

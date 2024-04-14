#pragma once

#include "macmodel.h"
#include "sessionsmodel.h"
#include "network_switch.h"
#include <QMainWindow>

namespace Ui
{
class InfoTable;
}

class MainWindow;

class InfoTable : public QMainWindow
{
    Q_OBJECT

public:
    explicit InfoTable(MainWindow & parent);
    ~InfoTable();

public slots:
    void refreshUi();

private slots:
    void onApplyTimeout();
    void onMacsClear();
    void onSessionsClear();

private:
    Ui::InfoTable *ui_m;
    NetworkSwitch & networkSwitch_m;
    unique_ptr<MacModel> macModel_m;
    unique_ptr<SessionsModel> sessionsModel_m;
};

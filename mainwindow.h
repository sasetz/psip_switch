#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "infotable.h"
#include "network_switch.h"
#include "statisticsmodel.h"
#include <QMainWindow>
#include <QTimer>

#include <tins/network_interface.h>

#include <QTableWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

using std::vector;
using Tins::NetworkInterface;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    friend class InfoTable;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartStopButtonClicked();
    void onRestStartStopButtonClicked();
    void refreshUi();

    void clearMacTable();
    void clearStatsTable();
    void clearFirstStatsTable();
    void clearSecondStatsTable();
    void resetMac();

    void updateMac();
    void updatePackets();
    void updateSessions();

private:
    void updateInterfaces();
    void startNetworkThread();
    void startRestThread();
    void stopThreads();
    void stopNetworkThread();
    void stopRestThread();

private:
    Ui::MainWindow *ui_m;
    InfoTable *info_m;

    vector<NetworkInterface> interfaces_m;
    QTimer configurationTimer_m, threadTimer_m, macTimer_m, packetsTimer_m, sessionTimer_m;
    NetworkSwitch networkSwitch_m;
    unique_ptr<StatisticsModel> firstModel, secondModel;
};
#endif // MAINWINDOW_H

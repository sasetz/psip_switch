#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "network_switch.h"
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

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartStopButtonClicked();
    void refreshUi();

    void clearMacTable();
    void clearStatsTable();
    void resetMac();
    void applyMac();

    void updateMac();
    void updatePackets();

private:
    void updateInterfaces();
    void startThread();
    void stopThread();

private:
    Ui::MainWindow *ui_m;

    vector<NetworkInterface> interfaces_m;
    QTimer configurationTimer_m, threadTimer_m, macTimer_m, packetsTimer_m;
    NetworkSwitch networkSwitch_m;
}; 
#endif // MAINWINDOW_H

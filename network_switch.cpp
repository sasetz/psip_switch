
#include "network_switch.h"
#include "network_handle.h"
#include <qlogging.h>
#include <tins/network_interface.h>

NetworkSwitch::NetworkSwitch()
    : storage_m{},
      storageMutex_m{},
      interface1_m(nullptr),
      interface2_m(nullptr),
      state_m(SwitchState::Idle)
{
}

void NetworkSwitch::startNetwork(string interface1, string interface2)
{
    if (state() != SwitchState::Idle)
    {
        qDebug("Network threads are already running!");
        return;
    }

    interface1_m.reset(new NetworkThreadHandle(getStorage(), Tins::NetworkInterface(interface1)));
    interface2_m.reset(new NetworkThreadHandle(getStorage(), Tins::NetworkInterface(interface2)));

    {
        lock_guard<mutex> lock(storageMutex_m);
        storage_m.reset();
    }

    interface1_m->start(1);
    interface2_m->start(2);
}

void NetworkSwitch::stopNetwork()
{
    interface1_m->signalStop();
    interface2_m->signalStop();
}

void NetworkSwitch::clearMac()
{
    lock_guard<mutex> lock(storageMutex_m);
    storage_m.macTable.clear();
}

void NetworkSwitch::clearStats()
{
    lock_guard<mutex> lock(storageMutex_m);
    storage_m.statisticsTable.clear();
}

NetworkSwitch::SwitchState NetworkSwitch::state() const
{
    lock_guard<mutex> lock(storageMutex_m);

    if (storage_m.interfaceThread1.running && !storage_m.interfaceThread1.finished &&
        storage_m.interfaceThread2.running && !storage_m.interfaceThread2.finished)
    {
        return SwitchState::RunningNetwork;
    }

    return SwitchState::Idle;
}

pair<string, string> NetworkSwitch::interfaces()
{
    // TODO: test this
    if (interface1_m.get() == nullptr || interface2_m.get() == nullptr)
    {
        qDebug("Interface names are being requested, but the threads are down");
        return {"", ""};
    }

    return {interface1_m->interfaceName(), interface2_m->interfaceName()};
}

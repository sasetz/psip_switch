
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

    interface1_m->start();
    interface2_m->start();
}

void NetworkSwitch::stopNetwork()
{
    interface1_m->signalStop();
    interface2_m->signalStop();
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

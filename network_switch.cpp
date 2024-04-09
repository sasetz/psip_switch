
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

NetworkSwitch::~NetworkSwitch()
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

void NetworkSwitch::resetMac()
{
    lock_guard<mutex> lock(storageMutex_m);
    for (auto & entry : storage_m.macTable)
    {
        entry.second.expiration.reset();
    }
}

void NetworkSwitch::applyMac(milliseconds newTimeout)
{
    lock_guard<mutex> lock(storageMutex_m);
    storage_m.deviceInfo.defaultMacTimeout = newTimeout;
}

NetworkSwitch::SwitchState NetworkSwitch::state() const
{
    lock_guard<mutex> lock(storageMutex_m);

    for (const auto & interface : storage_m.interfaces)
    {
        if (interface.second.control.running == false &&
        interface.second.control.finished == false)
        {
            return SwitchState::StoppingNetwork;
        }

        if (interface.second.control.running == true &&
        interface.second.control.finished == false)
        {
            return SwitchState::RunningNetwork;
        }
    }

    return SwitchState::Idle;
}

pair<string, string> NetworkSwitch::interfaces()
{
    if (interface1_m.get() == nullptr || interface2_m.get() == nullptr)
    {
        qDebug("Interface names are being requested, but the threads are down");
        return {"", ""};
    }

    return {interface1_m->interfaceName(), interface2_m->interfaceName()};
}

void NetworkSwitch::updateMac()
{
    lock_guard guard(storageMutex_m);
    for (auto it = storage_m.macTable.begin(); it != storage_m.macTable.end();)
    {
        if (it->second.expiration.expired())
        {
            it = storage_m.macTable.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void NetworkSwitch::updatePackets()
{
    lock_guard guard(storageMutex_m);
    for (auto it = storage_m.sentPackets.begin(); it != storage_m.sentPackets.end();)
    {
        if (it->expiration.expired())
        {
            it = storage_m.sentPackets.erase(it);
        }
        else
        {
            it++;
        }
    }
}



#include "network_switch.h"
#include "network_handle.h"
#include "shared_storage.h"
#include <qlogging.h>
#include <tins/network_interface.h>

using std::chrono::duration_cast, std::chrono::seconds;

NetworkSwitch::NetworkSwitch()
    : storage_m{},
      storageMutex_m{},
      interface1_m(nullptr),
      interface2_m(nullptr),
      restThread_m(nullptr),
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

void NetworkSwitch::startRest(int16_t port)
{
    if (state() != SwitchState::RunningNetwork)
    {
        qDebug("Network threads are not running!");
        return;
    }
    restThread_m.reset(new RestThreadHandle(getStorage(), port));
    restThread_m->start();
}

void NetworkSwitch::stopNetwork()
{
    interface1_m->signalStop();
    interface2_m->signalStop();
}

void NetworkSwitch::stopRest()
{
    if (restThread_m)
    {
        restThread_m->signalStop();
    }
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

void NetworkSwitch::clearStats(interface requiredInterface)
{
    lock_guard<mutex> lock(storageMutex_m);
    for (auto it = storage_m.statisticsTable.begin(); it != storage_m.statisticsTable.end();)
    {
        if (it->first.target != requiredInterface)
        {
            it++;
            continue;
        }

        it = storage_m.statisticsTable.erase(it);
    }
}

void NetworkSwitch::clearSessions()
{
    lock_guard<mutex> lock(storageMutex_m);
    storage_m.sessions.clear();
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

    if (storage_m.restThread.running == true)
    {
        return SwitchState::RunningRest;
    }

    for (const auto & interface : storage_m.interfaces)
    {
        if (interface.second.control.running == false && interface.second.control.finished == false)
        {
            return SwitchState::Stopping;
        }

        if (interface.second.control.running == true && interface.second.control.finished == false)
        {
            return SwitchState::RunningNetwork;
        }
    }

    if (restThread_m.get() != nullptr && storage_m.restThread.running == false &&
        storage_m.restThread.finished == false)
    {
        return SwitchState::Stopping;
    }

    return SwitchState::Idle;
}

pair<NetworkSwitch::InterfaceData, NetworkSwitch::InterfaceData> NetworkSwitch::interfaces()
{
    if (interface1_m.get() == nullptr || interface2_m.get() == nullptr)
    {
        qDebug("Interface names are being requested, but the threads are down");
        return {{}, {}};
    }
    lock_guard<mutex> lock(storageMutex_m);

    return {
        {
            storage_m.interfaces[interface1_m->getInterface()].name,
            interface1_m->interfaceName(),
            interface1_m->id(),
            storage_m.interfaces[interface1_m->getInterface()].up,
            interface1_m->getInterface(),
        },
        {
            storage_m.interfaces[interface2_m->getInterface()].name,
            interface2_m->interfaceName(),
            interface2_m->id(),
            storage_m.interfaces[interface2_m->getInterface()].up,
            interface2_m->getInterface(),
        },
    };
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

void NetworkSwitch::updateSessions()
{
    lock_guard guard(storageMutex_m);
    for (auto it = storage_m.sessions.begin(); it != storage_m.sessions.end();)
    {
        if (it->expiration.expired())
        {
            it = storage_m.sessions.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void NetworkSwitch::setMacTimeout(int32_t newTimeout)
{
    lock_guard guard(storageMutex_m);
    storage_m.deviceInfo.defaultMacTimeout = duration_cast<milliseconds>(
        seconds{newTimeout}
    );
}

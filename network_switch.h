#pragma once

#include "network_handle.h"
#include "shared_storage.h"
#include "shared_storage_handle.h"
#include <memory>
#include <mutex>
#include <string>
#include <utility>

using std::string, std::pair, std::mutex, std::lock_guard, std::unique_ptr;

struct NetworkSwitch
{
public:
    enum class SwitchState
    {
        Idle,
        RunningNetwork,
        StoppingNetwork
    };
public:
    NetworkSwitch();
    NetworkSwitch(NetworkSwitch &&) = delete;
    NetworkSwitch(const NetworkSwitch &) = delete;
    NetworkSwitch & operator=(NetworkSwitch &&) = delete;
    NetworkSwitch & operator=(const NetworkSwitch &) = delete;
    ~NetworkSwitch();

public:
    void startNetwork(string interface1, string interface2);
    void stopNetwork();
    void updateMac();

public:
    void clearMac();
    void clearStats();

    SharedStorageHandle getStorage();
    pair<string, string> interfaces();
    SwitchState state() const;

private:
    SharedStorage storage_m;
    mutable mutex storageMutex_m;
    unique_ptr<NetworkThreadHandle> interface1_m, interface2_m;

    SwitchState state_m;
};

inline SharedStorageHandle NetworkSwitch::getStorage()
{
    return SharedStorageHandle(storageMutex_m, storage_m);
}


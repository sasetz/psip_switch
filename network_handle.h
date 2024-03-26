#pragma once

#include "shared_storage_handle.h"
#include <thread>
#include <tins/hw_address.h>
#include <tins/network_interface.h>
#include <tins/pdu.h>

using interface = Tins::NetworkInterface;
using mac_address = Tins::HWAddress<6>;

// this class contains code to affect the running underlying thread
// the only method that runs in the separate thread is run()
// the other methods remain inside the main thread
struct NetworkThreadHandle
{
public:
    NetworkThreadHandle(SharedStorageHandle storageHandle, interface acceptingInterface);
    NetworkThreadHandle(NetworkThreadHandle &&) = default;
    NetworkThreadHandle(const NetworkThreadHandle &) = delete;
    NetworkThreadHandle & operator=(NetworkThreadHandle &&) = delete;
    NetworkThreadHandle & operator=(const NetworkThreadHandle &) = delete;
    ~NetworkThreadHandle();

public:
    void start();      // non-blocking
    void signalStop(); // doesn't *actually* stop the thread

public:
    string interfaceName() const;

private:
    void thread(); // blocking!
    void inputStatistics(Tins::PDU & packet, interface net, storage_guard & guard);
    void outputStatistics(Tins::PDU & packet, interface net, storage_guard & guard);
    void updateMac(mac_address mac, storage_guard & guard);
    void send(Tins::PDU & packet, interface destination, storage_guard & guard);
    void broadcast(Tins::PDU & packet, storage_guard & guard);

private:
    std::thread thread_m;
    SharedStorageHandle storageHandle_m;
    interface acceptingInterface_m;
};

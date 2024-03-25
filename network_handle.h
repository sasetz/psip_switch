#pragma once

#include "shared_storage_handle.h"
#include <thread>
#include <tins/network_interface.h>

using interface = Tins::NetworkInterface;

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
    // TODO: make start() set running variable to true
    // TODO: make start() choose the id of the thread
    void start(int id);      // non-blocking
    void signalStop(); // doesn't *actually* stop the thread

public:
    string interfaceName() const;

private:
    void thread(); // blocking!

private:
    std::thread thread_m;
    SharedStorageHandle storageHandle_m;
    interface acceptingInterface_m;

    bool running_m;
    int id_m;
};

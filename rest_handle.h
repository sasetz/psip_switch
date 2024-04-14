#pragma once

#include "shared_storage_handle.h"
#include <random>
#include <thread>
#include <tins/hw_address.h>
#include <tins/network_interface.h>
#include <tins/pdu.h>

using interface = Tins::NetworkInterface;
using mac_address = Tins::HWAddress<6>;
using std::unique_ptr;

// this class contains code to affect the running underlying thread
// the only method that runs in the separate thread is thread()
// the other methods remain inside the main thread
struct RestThreadHandle
{
public:
    RestThreadHandle(SharedStorageHandle storageHandle, int16_t port);
    RestThreadHandle(RestThreadHandle &&) = default;
    RestThreadHandle(const RestThreadHandle &) = delete;
    RestThreadHandle & operator=(RestThreadHandle &&) = delete;
    RestThreadHandle & operator=(const RestThreadHandle &) = delete;
    ~RestThreadHandle();

public:
    void start();      // non-blocking
    void signalStop(); // doesn't *actually* stop the thread

public:
    int16_t port() const;

private:
    void thread(); // blocking!
    string encodeJsonObject(map<string, string> data) const;
    string encodeJsonList(vector<string> data) const;
    string encodeJson(int data) const;
    string encodeJson(long data) const;
    string encodeJson(string data) const;
    string encodeJson(const char * data) const;
    string encodeJson(bool data) const;

private:
    std::thread thread_m;
    SharedStorageHandle storageHandle_m;
    int16_t port_m;
};

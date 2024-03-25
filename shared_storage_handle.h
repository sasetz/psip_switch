#pragma once

#include "shared_storage.h"
#include <mutex>

// a container for accessing the storage, while also keeping the lock guard
struct storage_guard
{
    std::lock_guard<std::mutex> lock;
    SharedStorage & storage;
};

// a copyable handle for accessing the shared storage
struct SharedStorageHandle
{
public:
    SharedStorageHandle(std::mutex & mutex, SharedStorage & storage);
    SharedStorageHandle() = delete;
    SharedStorageHandle(const SharedStorageHandle &) = default;
    SharedStorageHandle(SharedStorageHandle &&) = default;
    SharedStorageHandle & operator=(SharedStorageHandle &&) = delete;

public:
    storage_guard guard();

private:
    std::mutex & access_m;
    SharedStorage & storage_m;
};


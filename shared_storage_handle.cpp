#include "shared_storage_handle.h"
#include "shared_storage.h"
#include <mutex>

storage_guard SharedStorageHandle::guard()
{
    return {
        std::lock_guard<std::mutex>(access_m),
        storage_m
    };
}

SharedStorageHandle::SharedStorageHandle(std::mutex & mutex, SharedStorage & storage)
    : access_m(mutex),
    storage_m(storage)
{
}


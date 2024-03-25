#include "network_handle.h"
#include "shared_storage_handle.h"
#include <qlogging.h>

NetworkThreadHandle::NetworkThreadHandle(SharedStorageHandle storageHandle, interface acceptingInterface)
    : storageHandle_m(storageHandle),
      acceptingInterface_m(acceptingInterface),
      running_m(false),
      id_m(0)
{
}

string NetworkThreadHandle::interfaceName() const
{
    return acceptingInterface_m.name();
}

NetworkThreadHandle::~NetworkThreadHandle()
{
    running_m = false;
    if (thread_m.joinable())
    {
        qDebug("Joining the network thread...");
        thread_m.join();
    }
}

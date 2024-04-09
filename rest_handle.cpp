#include "rest_handle.h"
#include <lithium_http_server.hh>

void RestThreadHandle::thread()
{
    li::http_api api;
    api.get("/hello_world") = [&](li::http_request & request, li::http_response & response) {
        response.write("Hello World!");
    };

    li::http_serve(api, port_m);
    {
        auto guard = storageHandle_m.guard();
        guard.storage.restThread.finished = true;
    }
}

void RestThreadHandle::start()
{
    li::quit_signal_catched = 0;

    {
        auto guard = storageHandle_m.guard();
        guard.storage.restThread.running = true;
        guard.storage.restThread.finished = false;
    }

    // the actual start
    thread_m = std::thread(&RestThreadHandle::thread, this);
}

void RestThreadHandle::signalStop()
{
    auto guard = storageHandle_m.guard();
    guard.storage.restThread.running = false;
    li::quit_signal_catched = 1;
}

RestThreadHandle::RestThreadHandle(SharedStorageHandle storageHandle, int16_t port)
    : storageHandle_m{storageHandle},
      port_m{port}
{
}

RestThreadHandle::~RestThreadHandle()
{
    if (thread_m.joinable())
    {
        qDebug("Joining the network thread...");
        thread_m.join();
    }
}

int16_t RestThreadHandle::port() const
{
    return port_m;
}


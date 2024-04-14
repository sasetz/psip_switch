#include "rest_handle.h"
#include "settings.h"
#include "symbols.hh"
#include <chrono>
#include <lithium_http_server.hh>
#include <lithium_json.hh>
#include <string>

using std::map, std::string, std::optional;
using std::chrono::duration_cast, std::chrono::seconds;

void RestThreadHandle::thread()
{
    li::http_api api;
    api.get("/") = [&](li::http_request & request, li::http_response & response) {
        map<string, string> test = {
            {"hello", "world"}
        };

        auto json_str = "{\"hello\": \"world\"}";
        response.set_header("Content-Type", "application/json");
        response.write(encodeJsonObject({
            {"hello", encodeJson("world")                             },
            {"test",  (encodeJsonList({encodeJson(1), encodeJson(2)}))}
        }));
    };

    api.post("/login") = [&](li::http_request & request, li::http_response & response) {
        auto params = request.post_parameters(s::username = string(), s::password = string());
        response.set_header("Content-Type", "application/json");
        if (params.username != REST_USERNAME || params.password != REST_PASSWORD)
        {
            throw li::http_error::forbidden("Invalid username or password.");
        }
        auto guard = storageHandle_m.guard();
        guard->sessions.push_back({});

        response.write(encodeJsonObject({
            {"token", encodeJson(guard->sessions.back().token)}
        }));
    };

    api.get("/auth") = [&](li::http_request & request, li::http_response & response) {
        string_view bearerToken = request.header("Authorization");
        if (bearerToken.substr(0, 6) != "Bearer")
        {
            throw li::http_error::forbidden("Invalid auth token.");
        }
        string_view token = bearerToken.substr(7, string::npos);
        qInfo("Auth check on token: %s", string{token}.c_str());
        response.set_header("Content-Type", "application/json");
        {
            auto guard = storageHandle_m.guard();
            for (auto it = guard->sessions.begin(); it != guard->sessions.end(); it++)
            {
                if (it->getToken() == token)
                {
                    response.write(
                        encodeJsonObject({
                            { "auth", encodeJson(true) }
                        })
                    );
                    return;
                }
            }
            response.write(
                encodeJsonObject({
                    { "auth", encodeJson(false) }
                })
            );
        }
    };

    api.post("/logout") = [&](li::http_request & request, li::http_response & response) {
        string_view bearerToken = request.header("Authorization");
        if (bearerToken.substr(0, 6) != "Bearer")
        {
            throw li::http_error::forbidden("Invalid auth token.");
        }
        string_view token = bearerToken.substr(7, string::npos);
        response.set_header("Content-Type", "application/json");
        {
            auto guard = storageHandle_m.guard();
            for (auto it = guard->sessions.begin(); it != guard->sessions.end(); it++)
            {
                if (it->getToken() == token)
                {
                    guard->sessions.erase(it);
                    response.write(
                        encodeJsonObject({})
                    );
                    return;
                }
            }
        }
        throw li::http_error::forbidden("Invalid auth token.");
    };

    api.get("/interface") = [&](li::http_request & request, li::http_response & response) {
        string_view bearerToken = request.header("Authorization");
        if (bearerToken.substr(0, 6) != "Bearer")
        {
            throw li::http_error::forbidden("Invalid auth token.");
        }
        string_view token = bearerToken.substr(7, string::npos);
        response.set_header("Content-Type", "application/json");
        bool auth = false;
        {
            auto guard = storageHandle_m.guard();
            for (auto it = guard->sessions.begin(); it != guard->sessions.end(); it++)
            {
                if (it->getToken() == token)
                {
                    auth = true;
                    break;
                }
            }
            if (!auth)
            {
                throw li::http_error::forbidden("Invalid auth token.");
            }

            vector<string> interfaces;
            for (auto it = guard->interfaces.begin(); it != guard->interfaces.end(); it++)
            {
                interfaces.push_back(encodeJsonObject({
                    { "id", encodeJson(static_cast<int>(it->first.id())) },
                    { "name", encodeJson(it->second.name) },
                    { "up", encodeJson(it->second.up) },
                    { "address", encodeJson(it->first.hw_address().to_string()) }
                }));
            }

            response.write(
                encodeJsonObject({
                    { "interfaces", encodeJsonList(interfaces) }
                })
            );
        }
    };

    api.get("/interface/{{id}}") = [&](li::http_request & request, li::http_response & response) {
        string_view bearerToken = request.header("Authorization");
        if (bearerToken.substr(0, 6) != "Bearer")
        {
            throw li::http_error::forbidden("Invalid auth token.");
        }
        string_view token = bearerToken.substr(7, string::npos);
        response.set_header("Content-Type", "application/json");
        bool auth = false;
        {
            auto guard = storageHandle_m.guard();
            for (auto it = guard->sessions.begin(); it != guard->sessions.end(); it++)
            {
                if (it->getToken() == token)
                {
                    auth = true;
                    break;
                }
            }
            if (!auth)
            {
                throw li::http_error::forbidden("Invalid auth token.");
            }
            auto params = request.url_parameters(s::id = Tins::NetworkInterface::id_type());
            for (auto it = guard->interfaces.begin(); it != guard->interfaces.end(); it++)
            {
                if (it->first.id() == params.id)
                {
                    response.write(encodeJsonObject({
                        { "id", encodeJson(static_cast<int>(it->first.id())) },
                        { "name", encodeJson(it->second.name) },
                        { "up", encodeJson(it->second.up) },
                        { "address", encodeJson(it->first.hw_address().to_string()) }
                    }));
                    return;
                }
            }
            throw li::http_error::not_found("No such interface.");
        }
    };

    api.put("/interface/{{id}}/edit") = [&](li::http_request & request, li::http_response & response) {
        string_view bearerToken = request.header("Authorization");
        if (bearerToken.substr(0, 6) != "Bearer")
        {
            throw li::http_error::forbidden("Invalid auth token.");
        }
        string_view token = bearerToken.substr(7, string::npos);
        response.set_header("Content-Type", "application/json");
        bool auth = false;
        {
            auto guard = storageHandle_m.guard();
            for (auto it = guard->sessions.begin(); it != guard->sessions.end(); it++)
            {
                if (it->getToken() == token)
                {
                    auth = true;
                    break;
                }
            }
            if (!auth)
            {
                throw li::http_error::forbidden("Invalid auth token.");
            }
            auto params = request.url_parameters(s::id = Tins::NetworkInterface::id_type());
            auto config = request.post_parameters(s::name = optional<string>(), s::up = optional<int>());
            for (auto it = guard->interfaces.begin(); it != guard->interfaces.end(); it++)
            {
                if (it->first.id() == params.id)
                {
                    if (config.name.has_value())
                    {
                        it->second.name = *config.name;
                    }
                    qInfo("interface configuration, up:\nhas value?: %d\ncurrent value: %d\nnext value: %d", config.up.has_value(), it->second.up, config.up.has_value() ? *(config.up) : it->second.up);
                    if (config.up.has_value())
                    {
                        it->second.up = *config.up;
                    }

                    response.write(encodeJsonObject({
                        { "id", encodeJson(static_cast<int>(it->first.id())) },
                        { "name", encodeJson(it->second.name) },
                        { "up", encodeJson(it->second.up) },
                        { "address", encodeJson(it->first.hw_address().to_string()) }
                    }));
                    return;
                }
            }
            throw li::http_error::not_found("No such interface.");
        }
    };

    api.get("/device") = [&](li::http_request & request, li::http_response & response) {
        string_view bearerToken = request.header("Authorization");
        if (bearerToken.substr(0, 6) != "Bearer")
        {
            throw li::http_error::forbidden("Invalid auth token.");
        }
        string_view token = bearerToken.substr(7, string::npos);
        response.set_header("Content-Type", "application/json");
        bool auth = false;
        {
            auto guard = storageHandle_m.guard();
            for (auto it = guard->sessions.begin(); it != guard->sessions.end(); it++)
            {
                if (it->getToken() == token)
                {
                    auth = true;
                    break;
                }
            }
            if (!auth)
            {
                throw li::http_error::forbidden("Invalid auth token.");
            }

            response.write(
                encodeJsonObject({
                    { "hostname", encodeJson(guard->deviceInfo.hostname) },
                    { "timeout", encodeJson(duration_cast<seconds>(guard->deviceInfo.defaultMacTimeout).count()) }
                })
            );
        }
    };

    api.put("/device/edit") = [&](li::http_request & request, li::http_response & response) {
        string_view bearerToken = request.header("Authorization");
        if (bearerToken.substr(0, 6) != "Bearer")
        {
            throw li::http_error::forbidden("Invalid auth token.");
        }
        string_view token = bearerToken.substr(7, string::npos);
        response.set_header("Content-Type", "application/json");
        bool auth = false;
        {
            auto guard = storageHandle_m.guard();
            for (auto it = guard->sessions.begin(); it != guard->sessions.end(); it++)
            {
                if (it->getToken() == token)
                {
                    auth = true;
                    break;
                }
            }
            if (!auth)
            {
                throw li::http_error::forbidden("Invalid auth token.");
            }

            auto config = request.post_parameters(s::hostname = optional<string>(), s::timeout = optional<seconds::rep>());

            if (config.hostname.has_value())
            {
                guard->deviceInfo.hostname = *config.hostname;
            }
            if (config.timeout)
            {
                guard->deviceInfo.defaultMacTimeout = duration_cast<milliseconds>(
                    seconds{*(config.timeout)}
                );
            }

            response.write(
                encodeJsonObject({
                    { "hostname", encodeJson(guard->deviceInfo.hostname) },
                    { "timeout", encodeJson(duration_cast<seconds>(guard->deviceInfo.defaultMacTimeout).count()) }
                })
            );
        }
    };

    li::http_serve(api, port_m);
    {
        auto guard = storageHandle_m.guard();
        guard->restThread.finished = true;
    }
}

string RestThreadHandle::encodeJsonObject(map<string, string> data) const
{
    string output = "{";

    for (auto entry : data)
    {
        output += "\"";
        output += entry.first;
        output += "\":";
        output += entry.second;
        output += ",";
    }
    if (output.back() == ',')
    {
        output.pop_back();
    }

    output += "}";
    return output;
}

string RestThreadHandle::encodeJsonList(vector<string> data) const
{
    string output = "[";

    for (auto entry : data)
    {
        output += entry;
        output += ",";
    }
    if (output.back() == ',')
    {
        output.pop_back();
    }

    output += "]";
    return output;
}

string RestThreadHandle::encodeJson(int data) const
{
    return std::to_string(data);
}

string RestThreadHandle::encodeJson(long data) const
{
    return std::to_string(data);
}

string RestThreadHandle::encodeJson(string data) const
{
    return "\"" + data + "\"";
}

string RestThreadHandle::encodeJson(const char *data) const
{
    return "\"" + string{data} + "\"";
}

string RestThreadHandle::encodeJson(bool data) const
{
    return data ? "true" : "false";
}

void RestThreadHandle::start()
{
    li::quit_signal_catched = 0;

    {
        auto guard = storageHandle_m.guard();
        guard->restThread.running = true;
        guard->restThread.finished = false;
    }

    // the actual start
    thread_m = std::thread(&RestThreadHandle::thread, this);
}

void RestThreadHandle::signalStop()
{
    auto guard = storageHandle_m.guard();
    guard->restThread.running = false;
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

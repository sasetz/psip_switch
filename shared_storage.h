#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <tins/tins.h>
#include <unordered_set>

using mac_address = Tins::HWAddress<6>;
using interface = Tins::NetworkInterface;
using session_token = char[32];

using std::chrono::time_point, std::chrono::steady_clock, std::chrono::milliseconds;
using namespace std::chrono_literals;
using std::vector, std::map, std::string, std::string_view;

// ============================================================================
// = Statistics ===============================================================
// ============================================================================

struct Statistic
{
    int32_t input;
    int32_t output;
};

enum class Protocol
{
    // TODO: add all protocols from the specification
};
using StatisticsTable = map<Protocol, Statistic>;

// ============================================================================
// = Sessions =================================================================
// ============================================================================

struct timeout
{
    timeout();
    time_point<steady_clock> start;
    milliseconds duration;

public:
    bool expired() const;
    void reset();
    void setDuration(milliseconds newTimeout);

    static constexpr milliseconds DEFAULT_DURATION = 5s;
};

struct Session
{
    session_token token;
    timeout expiration;

public:
    string_view getToken() const;
};

// ============================================================================
// = MAC Table ================================================================
// ============================================================================

struct MacEntry
{
    interface interface;
    timeout expiration;
};

using MacTable = map<mac_address, interface>;

// ============================================================================
// = Device Info ==============================================================
// ============================================================================

struct DeviceInfo
{
    DeviceInfo();
    string hostname;
    milliseconds defaultMacTimeout;

    static constexpr milliseconds DEFAULT_TIMEOUT = 30s;
    static constexpr string_view DEFAULT_HOSTNAME = "Switch";
};

// ============================================================================
// = Thread Control ===========================================================
// ============================================================================

struct ThreadControl
{
    bool running;
    bool finished;
};

// ============================================================================
// = Interface Status =========================================================
// ============================================================================

struct InterfaceStatus
{
    bool up;
    interface interfaceAddress;
};

// ============================================================================
// = Hashable packet ==========================================================
// ============================================================================

struct Packet
{
    Packet() = delete;
    Packet(const Packet &) = default;
    Packet(Packet &&) = default;

    Packet(vector<uint8_t> && data);
    Packet(Tins::PDU & pdu);
    Packet(Tins::PDU *pdu);

    vector<uint8_t> data;
    timeout expiration;

    bool operator==(const Packet &) const;

    struct Hash
    {
        std::size_t operator()(const Packet &) const noexcept;
    };
};

using PacketTable = std::unordered_set<Packet, Packet::Hash>;

// ============================================================================
// = Shared Storage Definition ================================================
// ============================================================================

struct SharedStorage
{
    MacTable macTable;
    StatisticsTable statisticsTable;
    vector<Session> sessions;
    DeviceInfo deviceInfo;
    ThreadControl interfaceThread1, interfaceThread2, restThread;
    PacketTable sentPackets;

    void reset();
};

// ============================================================================
// = Inline implementations ===================================================
// ============================================================================

inline DeviceInfo::DeviceInfo()
    : hostname(DEFAULT_HOSTNAME),
      defaultMacTimeout(DEFAULT_TIMEOUT)
{
}

inline void SharedStorage::reset()
{
    macTable.clear();
    statisticsTable.clear();
    sessions.clear();
    deviceInfo.hostname = DeviceInfo::DEFAULT_HOSTNAME;
    deviceInfo.defaultMacTimeout = DeviceInfo::DEFAULT_TIMEOUT;
    interfaceThread1.finished = false;
    interfaceThread1.running = false;
    interfaceThread2.finished = false;
    interfaceThread2.running = false;
    sentPackets.clear();
}

inline timeout::timeout()
    : start(steady_clock::now()),
      duration{5s}
{
}

inline bool timeout::expired() const
{
    return start + duration > steady_clock::now();
}

inline void timeout::reset()
{
    start = steady_clock::now();
}

inline void timeout::setDuration(milliseconds newTimeout)
{
    duration = newTimeout;
}

inline string_view Session::getToken() const
{
    return string_view{token};
}

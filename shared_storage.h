#pragma once

#include "settings.h"
#include <chrono>
#include <cstdint>
#include <map>
#include <qlogging.h>
#include <string>
#include <string_view>
#include <tins/tins.h>
#include <unordered_set>

using mac_address = Tins::HWAddress<6>;
using interface = Tins::NetworkInterface;
using session_token = char[32];

using std::chrono::duration_cast;
using std::chrono::time_point, std::chrono::steady_clock, std::chrono::milliseconds;
using namespace std::chrono_literals;
using std::vector, std::map, std::string, std::string_view;

enum class Protocol
{
    EthernetII,
    ARP,
    IP,
    TCP,
    UDP,
    ICMP,
    HTTP
};

// One statistic entry
struct StatisticEntry
{
    int32_t input;
    int32_t output;
};

struct StatisticKey
{
    Protocol protocol;
    interface target;

    struct StatisticKeyComparator
    {
        bool operator()(const StatisticKey & lhs, const StatisticKey & rhs) const
        {
            return lhs.value() < rhs.value();
        }
    };

    int32_t value() const
    {
        return target.id() + static_cast<int>(protocol);
    }
};

using StatisticsTable = map<StatisticKey, StatisticEntry, StatisticKey::StatisticKeyComparator>;

string protocolToString(Protocol protocol);

// ============================================================================
// = Sessions =================================================================
// ============================================================================

struct timeout
{
    timeout();
    timeout(milliseconds duration);
    time_point<steady_clock> start;
    milliseconds duration;

public:
    bool expired() const;
    void reset();
    void setDuration(milliseconds newTimeout);
    milliseconds timeLeft() const;

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
    MacEntry & operator=(const MacEntry &) = default;
    MacEntry & operator=(MacEntry &&) = default;

    interface interface;
    timeout expiration;
};

using MacTable = map<mac_address, MacEntry>;

// ============================================================================
// = Device Info ==============================================================
// ============================================================================

struct DeviceInfo
{
    DeviceInfo();
    string hostname;
    milliseconds defaultMacTimeout;
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

struct InterfaceEntry
{
public:
    ThreadControl control;
    bool up;
};

struct NetworkInterfaceComparator
{
    bool operator()(const interface & lhs, const interface & rhs) const
    {
        return lhs.id() < rhs.id();
    }
};

using InterfaceTable = map<interface, InterfaceEntry, NetworkInterfaceComparator>;

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
    SharedStorage();
    MacTable macTable;
    StatisticsTable statisticsTable;
    vector<Session> sessions;
    DeviceInfo deviceInfo;
    ThreadControl restThread;
    InterfaceTable interfaces;
    PacketTable sentPackets;

    void reset();
    InterfaceEntry & getInterface(mac_address address);
};

// ============================================================================
// = Inline implementations ===================================================
// ============================================================================

inline DeviceInfo::DeviceInfo()
    : hostname(DEFAULT_HOSTNAME),
      defaultMacTimeout(DEFAULT_MAC_TIMEOUT)
{
}

inline SharedStorage::SharedStorage()
    : macTable{},
      statisticsTable{},
      sessions{},
      deviceInfo{},
      restThread{},
      sentPackets{},
      interfaces{}
{
    reset();
}

inline void SharedStorage::reset()
{
    macTable.clear();
    statisticsTable.clear();
    sessions.clear();
    deviceInfo.hostname = DEFAULT_HOSTNAME;
    deviceInfo.defaultMacTimeout = DEFAULT_MAC_TIMEOUT;
    sentPackets.clear();
    interfaces.clear();
}

inline timeout::timeout()
    : start(steady_clock::now()),
      duration{5s}
{
}

inline timeout::timeout(milliseconds duration)
    : start(steady_clock::now()),
      duration{duration}
{
}

inline bool timeout::expired() const
{
    return start + duration < steady_clock::now();
}

inline void timeout::reset()
{
    start = steady_clock::now();
}

inline void timeout::setDuration(milliseconds newTimeout)
{
    duration = newTimeout;
}

inline milliseconds timeout::timeLeft() const
{
    return duration_cast<milliseconds>(duration + start - steady_clock::now());
}

inline string_view Session::getToken() const
{
    return string_view{token};
}

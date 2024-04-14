#include "shared_storage.h"
#include "settings.h"
#include <cstdint>
#include <random>

Packet::Packet(vector<uint8_t> && data)
    : data(std::move(data)),
      expiration(DEFAULT_SENT_PACKET_TIMEOUT)
{
}

Packet::Packet(Tins::PDU & pdu)
    : Packet(std::move(pdu.serialize()))
{
}

Packet::Packet(Tins::PDU *pdu)
    : Packet(std::move(pdu->serialize()))
{
}

bool Packet::operator==(const Packet & other) const
{
    return this->data == other.data;
}

std::size_t Packet::Hash::operator()(const Packet & packet) const noexcept
{
    std::size_t hash = packet.data.size();

    for (int i = 0; i < packet.data.size(); i++)
    {
        auto byte = packet.data[i];
        hash ^= byte + 73 + (hash >> 1) + (hash << 3);
    }

    return hash;
}

string protocolToString(Protocol protocol)
{
    switch (protocol)
    {
    case Protocol::EthernetII:
        return "EthernetII";
    case Protocol::ARP:
        return "ARP";
    case Protocol::IP:
        return "IP";
    case Protocol::TCP:
        return "TCP";
    case Protocol::UDP:
        return "UDP";
    case Protocol::ICMP:
        return "ICMP";
    case Protocol::HTTP:
        return "HTTP";
    }
}

InterfaceEntry & SharedStorage::getInterface(mac_address address)
{
    for (auto & interface : interfaces)
    {
        if (interface.first.hw_address() == address)
        {
            return interface.second;
        }
    }
    throw std::runtime_error("No interface with the following MAC: " + address.to_string());
}

Session::Session()
    : expiration(DEFAULT_SESSION_TIMEOUT),
      token()
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> distribution(0, 64);

    for (int i = 0; i < TOKEN_LENGTH; i++)
    {
        auto result = distribution(rng);
        if (result < 10)
        {
            this->token[i] = result + '0';
        }
        else if (result < 36)
        {
            this->token[i] = result - 10 + 'a';
        }
        else if (result < 62)
        {
            this->token[i] = result - 36 + 'A';
        }
        else
        {
            this->token[i] = result % 2 == 0 ? '-' : '_';
        }
    }
    this->token[TOKEN_LENGTH] = '\0';
}

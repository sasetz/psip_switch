#include "shared_storage.h"
#include <cstdint>

Packet::Packet(vector<uint8_t> && data)
    : data(std::move(data))
{}

Packet::Packet(Tins::PDU & pdu)
    : Packet(std::move(pdu.serialize()))
{}

Packet::Packet(Tins::PDU * pdu)
    : Packet(std::move(pdu->serialize()))
{}

bool Packet::operator==(const Packet & other) const
{
    return this->data == other.data;
}

std::size_t Packet::Hash::operator()(const Packet & packet) const noexcept
{
    std::size_t hash = packet.data.size();

    for (int i = packet.data.size() > 22 ? 21 : 0; i < packet.data.size(); i++)
    {
        auto byte = packet.data[i];
        hash ^= byte + 73 + (hash >> 1) + (hash << 3);
    }

    return hash;
}


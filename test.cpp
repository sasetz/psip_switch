
#include "network_switch.h"
#include "shared_storage.h"
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <tins/pdu.h>
#include <tins/rawpdu.h>
#include <tins/tcp.h>

using std::cout, std::unique_ptr;

unique_ptr<Tins::PDU> generatePDU()
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> distByte(0, 255);

    vector<uint8_t> payload;
    payload.reserve(distByte(rng) + 20);
    uint8_t size = distByte(rng);
    for (int i = 0; i < size; i++)
    {
        payload.push_back(distByte(rng));
    }
    unique_ptr<Tins::TCP> output(new Tins::TCP);
    output->sport(distByte(rng) * 2);
    output->dport(distByte(rng) * 2);
    output->inner_pdu(new Tins::RawPDU(payload));
    return output;
}

void printPayload(const vector<uint8_t> & payload)
{
    cout << std::hex;
    int i = 0;
    for (const auto & byte : payload)
    {
        if (i % 8 == 7)
        {
            cout << "\n";
            i = 0;
        }
        cout << std::setw(3) << (int)byte;
        i++;
    }
    cout << "\n";
    cout << std::dec;
}

#define HASH_COUNT 10

int main (int argc, char *argv[]) {
    cout << "Testing packet hashing...\n";

    vector<unique_ptr<Tins::PDU>> pdus;
    vector<Packet> hashes;
    pdus.reserve(HASH_COUNT);
    hashes.reserve(HASH_COUNT);
    for (int i = 0; i < HASH_COUNT; i++)
    {
        pdus.push_back(generatePDU());
        hashes.push_back({pdus[i].get()});
    }

    bool ok = true;
    for (int i = 0; i < HASH_COUNT; i++)
    {
        for (int j = 0; j < HASH_COUNT; j++)
        {
            if (i == j)
            {
                continue;
            }
            if (hashes[i] == hashes[j])
            {
                cout << "Critical! Packets are equal!\n";
                printPayload(hashes[i].data);
                printPayload(hashes[j].data);
                ok = false;
            }
            if (Packet::Hash()(hashes[i]) == Packet::Hash()(hashes[j]))
            {
                cout << "Critical! Hashes are colliding!\n";
                cout << "First:\n" << std::hex << Packet::Hash()(hashes[i]) << "\n";
                printPayload(hashes[i].data);
                cout << "Second:\n" << std::hex << Packet::Hash()(hashes[j]) << "\n";
                printPayload(hashes[j].data);
                ok = false;
            }
        }

        if (Packet::Hash()(hashes[i]) != Packet::Hash()(hashes[i]))
        {
            cout << "Critical! Hashes are not equal!\n";
            cout  << std::hex << Packet::Hash()(hashes[i]) << "\n";
            printPayload(hashes[i].data);
            ok = false;
        }
    }

    if (ok)
    {
        cout << "---TEST PASS---\n";
    }

    for (int i = 0; i < HASH_COUNT; i++)
    {
        cout << i + 1 << ". " << std::hex << Packet::Hash()(hashes[i]) << "\n";
        printPayload(hashes[i].data);
    }

    return 0;
}


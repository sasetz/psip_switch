#include "network_handle.h"
#include "shared_storage.h"
#include "shared_storage_handle.h"
#include <qlogging.h>
#include <thread>
#include <tins/ethernetII.h>
#include <tins/exceptions.h>
#include <tins/packet_sender.h>
#include <tins/pdu.h>

void NetworkThreadHandle::start()
{
    {
        auto guard = storageHandle_m.guard();
        guard.storage.interfaces[acceptingInterface_m].control.running = true;
        guard.storage.interfaces[acceptingInterface_m].up = true;
    }

    // the actual start
    thread_m = std::thread(&NetworkThreadHandle::thread, this);
}

void NetworkThreadHandle::signalStop()
{
    auto guard = storageHandle_m.guard();
    guard.storage.interfaces[acceptingInterface_m].control.running = false;
}

// the network thread function itself
void NetworkThreadHandle::thread()
{
    Tins::SnifferConfiguration config;
    config.set_promisc_mode(true);
    config.set_immediate_mode(true);
    config.set_timeout(500);
    Tins::Sniffer reader(acceptingInterface_m.name(), config);
    reader.sniff_loop([&](Tins::PDU & packet) {
        qInfo("Received a packet!");
        if (!packet.find_pdu<Tins::EthernetII>())
        {
            qInfo("Non-EthernetII packet");

            auto guard = storageHandle_m.guard();
            return guard.storage.interfaces[acceptingInterface_m].control.running;
        }
        auto guard = storageHandle_m.guard();
        auto eth = packet.rfind_pdu<Tins::EthernetII>();

        // is this interface up?
        if (!guard.storage.interfaces[acceptingInterface_m].up)
        {
            qDebug("The interface %s is down, skipping", acceptingInterface_m.hw_address().to_string().c_str());
            return guard.storage.interfaces[acceptingInterface_m].control.running;
        }

        // did we send this packet?
        if (guard.storage.sentPackets.count(packet) == 1)
        {
            qDebug("Found a duplicate packet on interface %s, skipping",
                   acceptingInterface_m.hw_address().to_string().c_str());
            return guard.storage.interfaces[acceptingInterface_m].control.running;
        }

        // record the packet as input
        inputStatistics(packet, acceptingInterface_m, guard);

        // update MAC table
        updateMac(eth.src_addr(), guard);

        // did they send this packet to us?
        if (eth.dst_addr() == acceptingInterface_m.hw_address())
        {
            qDebug("The packet on interface %s was meant for that interface, skipping",
                   acceptingInterface_m.hw_address().to_string().c_str());
            return guard.storage.interfaces[acceptingInterface_m].control.running;
        }

        // is the destination on this device?
        for (const auto & entry : guard.storage.interfaces)
        {
            if (eth.dst_addr() == entry.first.hw_address())
            {
                qInfo("Switching packet to local device on interface %s", entry.first.hw_address().to_string().c_str());
                send(packet, entry.first, guard);
                return guard.storage.interfaces[acceptingInterface_m].control.running;
            }
        }

        // is destination address known?
        if (guard.storage.macTable.count(eth.dst_addr()) == 1)
        {
            qInfo("Switching packet using MAC entry");
            send(packet, guard.storage.macTable[eth.dst_addr()].interface, guard);
            return guard.storage.interfaces[acceptingInterface_m].control.running;
        }

        // broadcasting
        broadcast(packet, guard);

        return guard.storage.interfaces[acceptingInterface_m].control.running;
    });

    auto guard = storageHandle_m.guard();
    guard.storage.interfaces[acceptingInterface_m].control.finished = true;
    qInfo("Thread %s is down", acceptingInterface_m.hw_address().to_string().c_str());
}

void NetworkThreadHandle::send(Tins::PDU & packet, interface destination, storage_guard & guard)
{
    Tins::PacketSender sender;
    outputStatistics(packet, destination, guard);
    guard.storage.sentPackets.insert(packet);
    if (packet.size() > 1500)
    {
        qDebug("Sending a big chungus!");
        auto eth = packet.rfind_pdu<Tins::EthernetII>();
        qDebug("%s -> %s", eth.src_addr().to_string().c_str(), eth.dst_addr().to_string().c_str());
        if (destination.name().find("wlo") != string::npos)
        {
            qDebug("Cannot send jumbo to wifi!");
            return;
        }
    }
    if (packet.rfind_pdu<Tins::EthernetII>().dst_addr().is_broadcast())
    {
        qDebug("Sending a broadcast packet!");
    }
    sender.send(packet, destination);
}

void NetworkThreadHandle::broadcast(Tins::PDU & packet, storage_guard & guard)
{
    Tins::PacketSender sender;
    guard.storage.sentPackets.insert(packet);
    for (const auto & entry : guard.storage.interfaces)
    {
        if (entry.first == acceptingInterface_m)
        {
            continue;
        }
        qInfo("Broadcasting the packet to interface %s", entry.first.hw_address().to_string().c_str());
        outputStatistics(packet, entry.first, guard);
        sender.send(packet, entry.first);
    }
}

void NetworkThreadHandle::inputStatistics(Tins::PDU & packet, interface net, storage_guard & guard)
{
    if (packet.find_pdu<Tins::EthernetII>())
    {
        guard.storage.statisticsTable[Protocol::EthernetII].table[net].input++;
    }
    if (packet.find_pdu<Tins::ARP>())
    {
        guard.storage.statisticsTable[Protocol::ARP].table[net].input++;
    }
    if (packet.find_pdu<Tins::IP>())
    {
        guard.storage.statisticsTable[Protocol::IP].table[net].input++;
    }
    if (packet.find_pdu<Tins::TCP>())
    {
        guard.storage.statisticsTable[Protocol::TCP].table[net].input++;
    }
    if (packet.find_pdu<Tins::UDP>())
    {
        guard.storage.statisticsTable[Protocol::UDP].table[net].input++;
    }
    if (packet.find_pdu<Tins::ICMP>())
    {
        guard.storage.statisticsTable[Protocol::ICMP].table[net].input++;
    }

    try
    {
        auto tcp = packet.rfind_pdu<Tins::TCP>();
        if (tcp.sport() == 80 || tcp.dport() == 80)
        {
            guard.storage.statisticsTable[Protocol::HTTP].table[net].input++;
        }
    }
    catch (Tins::pdu_not_found & e)
    {
    }
}

void NetworkThreadHandle::outputStatistics(Tins::PDU & packet, interface net, storage_guard & guard)
{
    if (packet.find_pdu<Tins::EthernetII>())
    {
        guard.storage.statisticsTable[Protocol::EthernetII].table[net].output++;
    }
    if (packet.find_pdu<Tins::ARP>())
    {
        guard.storage.statisticsTable[Protocol::ARP].table[net].output++;
    }
    if (packet.find_pdu<Tins::IP>())
    {
        guard.storage.statisticsTable[Protocol::IP].table[net].output++;
    }
    if (packet.find_pdu<Tins::TCP>())
    {
        guard.storage.statisticsTable[Protocol::TCP].table[net].output++;
    }
    if (packet.find_pdu<Tins::UDP>())
    {
        guard.storage.statisticsTable[Protocol::UDP].table[net].output++;
    }
    if (packet.find_pdu<Tins::ICMP>())
    {
        guard.storage.statisticsTable[Protocol::ICMP].table[net].output++;
    }

    try
    {
        auto tcp = packet.rfind_pdu<Tins::TCP>();
        if (tcp.sport() == 80 || tcp.dport() == 80)
        {
            guard.storage.statisticsTable[Protocol::HTTP].table[net].output++;
        }
    }
    catch (Tins::pdu_not_found & e)
    {
    }
}

void NetworkThreadHandle::updateMac(mac_address mac, storage_guard & guard)
{
    guard.storage.macTable[mac] = {acceptingInterface_m, guard.storage.deviceInfo.defaultMacTimeout};
}

NetworkThreadHandle::NetworkThreadHandle(SharedStorageHandle storageHandle, interface acceptingInterface)
    : storageHandle_m(storageHandle),
      acceptingInterface_m(acceptingInterface),
      thread_m{}
{
}

string NetworkThreadHandle::interfaceName() const
{
    return acceptingInterface_m.name();
}

NetworkThreadHandle::~NetworkThreadHandle()
{
    if (thread_m.joinable())
    {
        qDebug("Joining the network thread...");
        thread_m.join();
    }
}

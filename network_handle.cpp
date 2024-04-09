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
        guard.storage.interfaces[interface_m] = {};
        guard.storage.interfaces[interface_m].control.running = true;
        guard.storage.interfaces[interface_m].up = true;
    }

    // the actual start
    thread_m = std::thread(&NetworkThreadHandle::thread, this);
}

void NetworkThreadHandle::signalStop()
{
    auto guard = storageHandle_m.guard();
    guard.storage.interfaces[interface_m].control.running = false;
}

// the network thread function itself
void NetworkThreadHandle::thread()
{
    Tins::SnifferConfiguration config;
    config.set_promisc_mode(true);
    config.set_immediate_mode(true);
    config.set_timeout(500);
    Tins::Sniffer reader(interface_m.name(), config);
    reader.sniff_loop([&](Tins::PDU & packet) {
        qInfo("Received a packet!");
        if (!packet.find_pdu<Tins::EthernetII>())
        {
            qInfo("Non-EthernetII packet");

            auto guard = storageHandle_m.guard();
            return guard.storage.interfaces[interface_m].control.running;
        }
        auto guard = storageHandle_m.guard();
        SnifferHelper me(guard, interface_m);
        auto eth = packet.rfind_pdu<Tins::EthernetII>();

        // is this interface up?
        if (!me.up())
        {
            qDebug("The interface %s is down, skipping", interface_m.hw_address().to_string().c_str());
            return me.running();
        }

        // did we send this packet?
        if (guard.storage.sentPackets.count(packet) == 1)
        {
            qDebug("Found a duplicate packet on interface %s, skipping",
                   interface_m.hw_address().to_string().c_str());
            return me.running();
        }

        // record the packet as input
        inputStatistics(packet, interface_m, guard);

        // did our device send this?
        if (eth.src_addr() == interface_m.hw_address())
        {
            qDebug("The packet on interface %s was sent by that interface, skipping",
                   interface_m.hw_address().to_string().c_str());
            return me.running();
        }

        // update MAC table
        updateMac(eth.src_addr(), guard);

        // did they send this packet to us?
        if (eth.dst_addr() == interface_m.hw_address())
        {
            qDebug("The packet on interface %s was meant for that interface, skipping",
                   interface_m.hw_address().to_string().c_str());
            return me.running();
        }

        // is the destination on this device?
        for (const auto & entry : guard.storage.interfaces)
        {
            if (eth.dst_addr() == entry.first.hw_address())
            {
                qInfo("Switching packet to local device on interface %s", entry.first.hw_address().to_string().c_str());
                send(packet, entry.first, guard);
                return me.running();
            }
        }

        // is destination address known?
        if (me.macTable().count(eth.dst_addr()) == 1)
        {
            // did we get this packet on the same interface that we need to send
            // it to?
            if (me.macTable()[eth.dst_addr()].interface == interface_m)
            {
                qInfo("The recipient of the packet has already received it, skipping");
                return me.running();
            }
            qInfo("Switching packet using MAC entry");
            send(packet, me.macTable()[eth.dst_addr()].interface, guard);
            return me.running();
        }

        // broadcasting
        broadcast(packet, guard);

        return me.running();
    });

    auto guard = storageHandle_m.guard();
    guard.storage.interfaces[interface_m].control.finished = true;
    qInfo("Thread %s is down", interface_m.hw_address().to_string().c_str());
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
        if (entry.first == interface_m)
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
        guard.storage.statisticsTable[{Protocol::EthernetII, net}].input++;
    }
    if (packet.find_pdu<Tins::ARP>())
    {
        guard.storage.statisticsTable[{Protocol::ARP, net}].input++;
    }
    if (packet.find_pdu<Tins::IP>())
    {
        guard.storage.statisticsTable[{Protocol::IP, net}].input++;
    }
    if (packet.find_pdu<Tins::TCP>())
    {
        guard.storage.statisticsTable[{Protocol::TCP, net}].input++;
    }
    if (packet.find_pdu<Tins::UDP>())
    {
        guard.storage.statisticsTable[{Protocol::UDP, net}].input++;
    }
    if (packet.find_pdu<Tins::ICMP>())
    {
        guard.storage.statisticsTable[{Protocol::ICMP, net}].input++;
    }

    try
    {
        auto tcp = packet.rfind_pdu<Tins::TCP>();
        if (tcp.sport() == 80 || tcp.dport() == 80 || tcp.sport() == 443 || tcp.dport() == 443)
        {
            guard.storage.statisticsTable[{Protocol::HTTP, net}].input++;
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
        guard.storage.statisticsTable[{Protocol::EthernetII, net}].output++;
    }
    if (packet.find_pdu<Tins::ARP>())
    {
        guard.storage.statisticsTable[{Protocol::ARP, net}].output++;
    }
    if (packet.find_pdu<Tins::IP>())
    {
        guard.storage.statisticsTable[{Protocol::IP, net}].output++;
    }
    if (packet.find_pdu<Tins::TCP>())
    {
        guard.storage.statisticsTable[{Protocol::TCP, net}].output++;
    }
    if (packet.find_pdu<Tins::UDP>())
    {
        guard.storage.statisticsTable[{Protocol::UDP, net}].output++;
    }
    if (packet.find_pdu<Tins::ICMP>())
    {
        guard.storage.statisticsTable[{Protocol::ICMP, net}].output++;
    }

    try
    {
        auto tcp = packet.rfind_pdu<Tins::TCP>();
        if (tcp.sport() == 80 || tcp.dport() == 80)
        {
            guard.storage.statisticsTable[{Protocol::HTTP, net}].output++;
        }
    }
    catch (Tins::pdu_not_found & e)
    {
    }
}

void NetworkThreadHandle::updateMac(mac_address mac, storage_guard & guard)
{
    guard.storage.macTable[mac] = {interface_m, guard.storage.deviceInfo.defaultMacTimeout};
}

NetworkThreadHandle::NetworkThreadHandle(SharedStorageHandle storageHandle, interface acceptingInterface)
    : storageHandle_m(storageHandle),
      interface_m(acceptingInterface),
      thread_m{}
{
}

string NetworkThreadHandle::interfaceName() const
{
    return interface_m.name();
}

NetworkThreadHandle::~NetworkThreadHandle()
{
    if (thread_m.joinable())
    {
        qDebug("Joining the network thread...");
        thread_m.join();
    }
}

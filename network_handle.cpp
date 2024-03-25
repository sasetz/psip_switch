#include "network_handle.h"
#include "shared_storage.h"
#include "shared_storage_handle.h"
#include <qlogging.h>
#include <thread>
#include <tins/ethernetII.h>
#include <tins/exceptions.h>
#include <tins/pdu.h>

void NetworkThreadHandle::start(int id)
{
    if (id_m != 1 && id_m != 2)
    {
        qDebug("Warning: thread id can only be 1 or 2");
    }
    id_m = id;
    running_m = true;
    {
        auto guard = storageHandle_m.guard();
        guard.storage.getInterface(id_m).running = true;
    }

    // the actual start
    thread_m = std::thread(&NetworkThreadHandle::thread, this);
}

// the network thread function itself
void NetworkThreadHandle::thread()
{
    Tins::SnifferConfiguration config;
    config.set_promisc_mode(true);
    config.set_immediate_mode(true);
    config.set_timeout(500);
    Tins::Sniffer reader(acceptingInterface_m.name(), config);
    Tins::PacketSender sender;
    reader.sniff_loop([&](Tins::PDU & packet) {
        qInfo("Received a packet!");
        if (!packet.find_pdu<Tins::EthernetII>())
        {
            qInfo("Non-EthernetII packet");

            auto guard = storageHandle_m.guard();
            return guard.storage.getInterface(id_m).running;
        }
        auto guard = storageHandle_m.guard();
        auto eth = packet.rfind_pdu<Tins::EthernetII>();

        // is this interface up?
        if (!guard.storage.getStatus(id_m).up)
        {
            qDebug("The interface %d is down, skipping", id_m);
            return guard.storage.getInterface(id_m).running;
        }

        // did we send this packet?
        if (guard.storage.sentPackets.count(packet) == 1)
        {
            qDebug("Found a duplicate packet on interface %d, skipping", id_m);
            return guard.storage.getInterface(id_m).running;
        }

        // record the packet as input
        inputStatistics(packet, guard);

        // update MAC table
        updateMac(eth.src_addr(), guard);

        // did they send this packet to us?
        if (eth.dst_addr() == acceptingInterface_m.hw_address())
        {
            qDebug("The packet on interface %d was meant for that interface, skipping", id_m);
            return guard.storage.getInterface(id_m).running;
        }

        // is the destination on this device?
        if (eth.dst_addr() == guard.storage.interfaceStatus1.interfaceAddress.hw_address())
        {
            qInfo("Switching packet to local device on interface 1");
            outputStatistics(packet, guard);
            sender.send(packet, guard.storage.interfaceStatus1.interfaceAddress);
            return guard.storage.getInterface(id_m).running;
        }
        if (eth.dst_addr() == guard.storage.interfaceStatus2.interfaceAddress.hw_address())
        {
            qInfo("Switching packet to local device on interface 2");
            outputStatistics(packet, guard);
            sender.send(packet, guard.storage.interfaceStatus2.interfaceAddress);
            return guard.storage.getInterface(id_m).running;
        }

        // is destination address known?
        if (guard.storage.macTable.count(eth.dst_addr()) == 1)
        {
            qInfo("Switching packet using MAC entry");
            outputStatistics(packet, guard);
            sender.send(packet, guard.storage.macTable[eth.dst_addr()].interface);
            return guard.storage.getInterface(id_m).running;
        }

        // broadcasting
        if (id_m == 1)
        {
            qInfo("Broadcasting the packet to interface 2");
            sender.send(packet, guard.storage.interfaceStatus2.interfaceAddress);
        }
        else
        {
            qInfo("Broadcasting the packet to interface 1");
            sender.send(packet, guard.storage.interfaceStatus1.interfaceAddress);
        }

        return guard.storage.getInterface(id_m).running;
    });

    auto guard = storageHandle_m.guard();
    guard.storage.getInterface(id_m).finished = true;
}

void NetworkThreadHandle::inputStatistics(Tins::PDU & packet, storage_guard & guard)
{
    if (packet.find_pdu<Tins::EthernetII>())
    {
        guard.storage.statisticsTable[Protocol::EthernetII].input++;
    }
    if (packet.find_pdu<Tins::ARP>())
    {
        guard.storage.statisticsTable[Protocol::ARP].input++;
    }
    if (packet.find_pdu<Tins::IP>())
    {
        guard.storage.statisticsTable[Protocol::IP].input++;
    }
    if (packet.find_pdu<Tins::TCP>())
    {
        guard.storage.statisticsTable[Protocol::TCP].input++;
    }
    if (packet.find_pdu<Tins::UDP>())
    {
        guard.storage.statisticsTable[Protocol::UDP].input++;
    }
    if (packet.find_pdu<Tins::ICMP>())
    {
        guard.storage.statisticsTable[Protocol::ICMP].input++;
    }

    try
    {
        auto tcp = packet.rfind_pdu<Tins::TCP>();
        if (tcp.sport() == 80 || tcp.dport() == 80)
        {
            guard.storage.statisticsTable[Protocol::HTTP].input++;
        }
    }
    catch (Tins::pdu_not_found & e)
    {
    }
}

void NetworkThreadHandle::outputStatistics(Tins::PDU & packet, storage_guard & guard)
{
    if (packet.find_pdu<Tins::EthernetII>())
    {
        guard.storage.statisticsTable[Protocol::EthernetII].output++;
    }
    if (packet.find_pdu<Tins::ARP>())
    {
        guard.storage.statisticsTable[Protocol::ARP].output++;
    }
    if (packet.find_pdu<Tins::IP>())
    {
        guard.storage.statisticsTable[Protocol::IP].output++;
    }
    if (packet.find_pdu<Tins::TCP>())
    {
        guard.storage.statisticsTable[Protocol::TCP].output++;
    }
    if (packet.find_pdu<Tins::UDP>())
    {
        guard.storage.statisticsTable[Protocol::UDP].output++;
    }
    if (packet.find_pdu<Tins::ICMP>())
    {
        guard.storage.statisticsTable[Protocol::ICMP].output++;
    }

    try
    {
        auto tcp = packet.rfind_pdu<Tins::TCP>();
        if (tcp.sport() == 80 || tcp.dport() == 80)
        {
            guard.storage.statisticsTable[Protocol::HTTP].output++;
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

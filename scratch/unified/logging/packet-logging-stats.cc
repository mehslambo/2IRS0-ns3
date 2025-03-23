#include "packet-logging-stats.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include "ns3/simulator.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-mac.h"

PacketLoggingStats::PacketLoggingStats(const std::string& scenarioName,
                                       const NodeContainer& staNodes,
                                       const NodeContainer& apNodes)
    : m_scenarioName(scenarioName),
      m_staNodes(staNodes),
      m_apNodes(apNodes)
{
}

void PacketLoggingStats::EnableLogging()
{
    // Used for throughput calculations
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxBegin",
                    MakeCallback(&PacketLoggingStats::TxBeginCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxEnd",
                    MakeCallback(&PacketLoggingStats::RxEndCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDropWithReason",
                    MakeCallback(&PacketLoggingStats::RxDropCallback, this));

    // Used for signal strength
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx",
                    MakeCallback(&PacketLoggingStats::MonitorSnifferRxCallback, this));
}

MacAddresses PacketLoggingStats::ExtractMacAddresses(const WifiMacHeader& header)
{
    MacAddresses macs;
    macs.receiver = header.GetAddr1();
    macs.transmitter = header.GetAddr2();

    bool toDS = header.IsToDs();
    bool fromDS = header.IsFromDs();
    if (!toDS && !fromDS) {
        // Ad-hoc: addr3 is BSSID.
        macs.destination = header.GetAddr1();
        macs.source = header.GetAddr2();
        macs.bssid = header.GetAddr3();
    } else if (!toDS && fromDS) {
        // From AP to STA.
        macs.destination = header.GetAddr1();
        macs.bssid = header.GetAddr2();
        macs.source = header.GetAddr3();
    } else if (toDS && !fromDS) {
        // From STA to AP.
        macs.bssid = header.GetAddr1();
        macs.source = header.GetAddr2();
        macs.destination = header.GetAddr3();
    } else {
        // WDS: addr4 exists.
        macs.bssid = Mac48Address();
        macs.destination = header.GetAddr3();
        macs.source = header.GetAddr4();
    }
    return macs;
}

uint32_t PacketLoggingStats::GetNodeIdFromMacAddress(const Mac48Address& addr)
{
    // Search STA nodes.
    for (uint32_t i = 0; i < m_staNodes.GetN(); i++) {
        Ptr<WifiNetDevice> dev = m_staNodes.Get(i)->GetDevice(0)->GetObject<WifiNetDevice>();
        if (dev && dev->GetMac()->GetAddress() == addr) {
            return m_staNodes.Get(i)->GetId();
        }
    }
    // Search AP nodes.
    for (uint32_t i = 0; i < m_apNodes.GetN(); i++) {
        Ptr<WifiNetDevice> dev = m_apNodes.Get(i)->GetDevice(0)->GetObject<WifiNetDevice>();
        if (dev && dev->GetMac()->GetAddress() == addr) {
            return m_apNodes.Get(i)->GetId();
        }
    }
    return (uint32_t)-1;
}

std::string PacketLoggingStats::GetNodeType(uint32_t nodeId)
{
    Ptr<Node> node = nullptr;
    for (uint32_t i = 0; i < m_staNodes.GetN(); i++) {
        if (m_staNodes.Get(i)->GetId() == nodeId) {
            node = m_staNodes.Get(i);
            break;
        }
    }
    if (!node) {
        for (uint32_t i = 0; i < m_apNodes.GetN(); i++) {
            if (m_apNodes.Get(i)->GetId() == nodeId) {
                node = m_apNodes.Get(i);
                break;
            }
        }
    }
    if (!node)
        return "Unknown";
    std::string nodeType = "STA";
    for (uint32_t i = 0; i < node->GetNDevices(); i++) {
        Ptr<WifiNetDevice> wifiDev = node->GetDevice(i)->GetObject<WifiNetDevice>();
        if (wifiDev) {
            Ptr<ApWifiMac> apMac = wifiDev->GetMac()->GetObject<ApWifiMac>();
            if (apMac) {
                nodeType = "AP";
                break;
            }
        }
    }
    return nodeType;
}

Vector PacketLoggingStats::GetNodeCoordinates(uint32_t nodeId)
{
    Ptr<Node> node = nullptr;
    for (uint32_t i = 0; i < m_staNodes.GetN(); i++) {
        if (m_staNodes.Get(i)->GetId() == nodeId) {
            node = m_staNodes.Get(i);
            break;
        }
    }
    if (!node) {
        for (uint32_t i = 0; i < m_apNodes.GetN(); i++) {
            if (m_apNodes.Get(i)->GetId() == nodeId) {
                node = m_apNodes.Get(i);
                break;
            }
        }
    }
    if (!node) return Vector(0,0,0);
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    return mobility ? mobility->GetPosition() : Vector(0,0,0);
}

void PacketLoggingStats::TxBeginCallback(std::string context, Ptr<const Packet> packet)
{
    // Extract the sniffer node ID from context.
    std::string::size_type pos = context.find("/NodeList/");
    std::string nodeStr = context.substr(pos);
    uint32_t snifferNodeId;
    sscanf(nodeStr.c_str(), "/NodeList/%u/", &snifferNodeId);

    WifiMacHeader header;
    packet->PeekHeader(header);
    MacAddresses macs = ExtractMacAddresses(header);

    // Check if the sniffer node matches either the source or destination node.
    if (snifferNodeId != GetNodeIdFromMacAddress(macs.destination) &&
        snifferNodeId != GetNodeIdFromMacAddress(macs.source))
    {
        return;
    }

    uint32_t packetId = packet->GetUid();
    // Look up (or create) the record for this packet.
    PacketRecord& record = m_packetRecords[packetId];
    if (record.packetId == 0) {
        record.packetId = packetId;
        record.packetSize = packet->GetSize();
        record.macs = macs;
        record.receiverNodeId = GetNodeIdFromMacAddress(macs.receiver);
        record.transmitterNodeId = GetNodeIdFromMacAddress(macs.transmitter);
        record.destinationNodeId = GetNodeIdFromMacAddress(macs.destination);
        record.sourceNodeId = GetNodeIdFromMacAddress(macs.source);
        record.receiverNodeType = GetNodeType(record.receiverNodeId);
        record.transmitterNodeType = GetNodeType(record.transmitterNodeId);
        record.destinationNodeType = GetNodeType(record.destinationNodeId);
        record.sourceNodeType = GetNodeType(record.sourceNodeId);
    }
    
    if (record.txBeginCount == 0) {
        record.txBeginFirstSeen = Simulator::Now();
    }
    record.txBeginCount++;
}

void PacketLoggingStats::RxEndCallback(std::string context, Ptr<const Packet> packet)
{
    // Extract the sniffer node ID from context.
    std::string::size_type pos = context.find("/NodeList/");
    std::string nodeStr = context.substr(pos);
    uint32_t snifferNodeId;
    sscanf(nodeStr.c_str(), "/NodeList/%u/", &snifferNodeId);

    WifiMacHeader header;
    packet->PeekHeader(header);
    MacAddresses macs = ExtractMacAddresses(header);

    // Check if the sniffer node matches either the source or destination node.
    if (snifferNodeId != GetNodeIdFromMacAddress(macs.destination) &&
        snifferNodeId != GetNodeIdFromMacAddress(macs.source))
    {
        return;
    }

    uint32_t packetId = packet->GetUid();
    PacketRecord& record = m_packetRecords[packetId];
    if (record.packetId == 0) {
        record.packetId = packetId;
        record.packetSize = packet->GetSize();
        record.macs = macs;
        record.receiverNodeId = GetNodeIdFromMacAddress(macs.receiver);
        record.transmitterNodeId = GetNodeIdFromMacAddress(macs.transmitter);
        record.destinationNodeId = GetNodeIdFromMacAddress(macs.destination);
        record.sourceNodeId = GetNodeIdFromMacAddress(macs.source);
        record.receiverNodeType = GetNodeType(record.receiverNodeId);
        record.transmitterNodeType = GetNodeType(record.transmitterNodeId);
        record.destinationNodeType = GetNodeType(record.destinationNodeId);
        record.sourceNodeType = GetNodeType(record.sourceNodeId);
    }
    record.rxEndLastSeen = Simulator::Now();
    record.rxEndCount++;
    // Update the source and destination coordinates at the time of this RX event.
    record.srcCoordinates = GetNodeCoordinates(record.sourceNodeId);
    record.dstCoordinates = GetNodeCoordinates(record.destinationNodeId);
}

void PacketLoggingStats::RxDropCallback(std::string context, Ptr<const Packet> packet, DropReason reason)
{
    // Extract the sniffer node ID from context.
    std::string::size_type pos = context.find("/NodeList/");
    std::string nodeStr = context.substr(pos);
    uint32_t snifferNodeId;
    sscanf(nodeStr.c_str(), "/NodeList/%u/", &snifferNodeId);

    WifiMacHeader header;
    packet->PeekHeader(header);
    MacAddresses macs = ExtractMacAddresses(header);

    // Check if the sniffer node matches either the source or destination node.
    if (snifferNodeId != GetNodeIdFromMacAddress(macs.destination) &&
        snifferNodeId != GetNodeIdFromMacAddress(macs.source))
    {
        return;
    }

    uint32_t packetId = packet->GetUid();
    PacketRecord& record = m_packetRecords[packetId];
    if (record.packetId == 0) {
        record.packetId = packetId;
        record.packetSize = packet->GetSize();
        record.macs = macs;
        record.receiverNodeId = GetNodeIdFromMacAddress(macs.receiver);
        record.transmitterNodeId = GetNodeIdFromMacAddress(macs.transmitter);
        record.destinationNodeId = GetNodeIdFromMacAddress(macs.destination);
        record.sourceNodeId = GetNodeIdFromMacAddress(macs.source);
        record.receiverNodeType = GetNodeType(record.receiverNodeId);
        record.transmitterNodeType = GetNodeType(record.transmitterNodeId);
        record.destinationNodeType = GetNodeType(record.destinationNodeId);
        record.sourceNodeType = GetNodeType(record.sourceNodeId);
    }
    record.rxDropCount++;
}

void PacketLoggingStats::MonitorSnifferRxCallback(std::string context, Ptr<const Packet> packet, 
    uint16_t channelFreqMhz, uint16_t channelNumber, 
    uint32_t rate, bool isShortPreamble, 
    WifiTxVector txVector,
    double signalDbm, double noiseDbm)
{
    // Extract the sniffer node ID from context
    std::string::size_type pos = context.find("/NodeList/");
    std::string nodeStr = context.substr(pos);
    uint32_t snifferNodeId;
    sscanf(nodeStr.c_str(), "/NodeList/%u/", &snifferNodeId);

    WifiMacHeader header;
    packet->PeekHeader(header);
    MacAddresses macs = ExtractMacAddresses(header);

    // Check if the sniffer node matches either the source or destination node
    if (snifferNodeId != GetNodeIdFromMacAddress(macs.destination) &&
    snifferNodeId != GetNodeIdFromMacAddress(macs.source))
    {
        return;
    }

    uint32_t packetId = packet->GetUid();
    PacketRecord& record = m_packetRecords[packetId];

    record.lastRxSignal = signalDbm;
}

void PacketLoggingStats::DumpPacketRecords() const
{
    std::cout << "=== Packet Logging Summary ===" << std::endl;
    for (std::map<uint32_t, PacketRecord>::const_iterator it = m_packetRecords.begin();
         it != m_packetRecords.end(); ++it) {
        const PacketRecord& record = it->second;
        std::cout << "Packet ID: " << record.packetId
                  << ", Size: " << record.packetSize
                  << ", TX Start First Seen: " << record.txBeginFirstSeen.GetSeconds() << " s"
                  << ", RX End Last Seen: " << record.rxEndLastSeen.GetSeconds() << " s"
                  << ", TX Start Count: " << record.txBeginCount
                  << ", RX End Count: " << record.rxEndCount
                  << ", RX Drop Count: " << record.rxDropCount
                  << "\n    Src Node (" << record.sourceNodeId << " - " << record.sourceNodeType << ") at ("
                  << record.srcCoordinates.x << ", " << record.srcCoordinates.y << ", " << record.srcCoordinates.z << ")"
                  << "\n    Dst Node (" << record.destinationNodeId << " - " << record.destinationNodeType << ") at ("
                  << record.dstCoordinates.x << ", " << record.dstCoordinates.y << ", " << record.dstCoordinates.z << ")"
                  << std::endl;
    }
}

void PacketLoggingStats::DumpPacketRecordsToCsv() const
{
    std::string baseLogPath = "postprocessing/logs/" + m_scenarioName;
    // Create logging directory if it doesn't exist
    std::string cmd = "mkdir -p " + baseLogPath;
    int result = system(cmd.c_str());
    if (result != 0) {
        std::cout << "Failed to create logging directory: " << baseLogPath << std::endl;
    }
    
    std::string filename = baseLogPath + "/PacketLoggingStats.csv";
    std::ofstream csvFile(filename.c_str());
    if (!csvFile.is_open()) {
        std::cerr << "Failed to open CSV file for writing: " << filename << std::endl;
        return;
    }

    // Write CSV header.
    csvFile << "PacketId;PacketSize;"
            << "ReceiverMac;ReceiverNodeId;ReceiverNodeType;"
            << "TransmitterMac;TransmitterNodeId;TransmitterNodeType;"
            << "DestinationMac;DestinationNodeId;DestinationNodeType;"
            << "SourceMac;SourceNodeId;SourceNodeType;"
            << "TxBeginFirstSeen;RxEndLastSeen;TxBeginCount;RxEndCount;RxDropCount;"
            << "SourceNodeX;SourceNodeY;SourceNodeZ;"
            << "DestinationNodeX;DestinationNodeY;DestinationNodeZ;"
            << "LastRxSignal" << std::endl;

    // Write each packet record.
    for (std::map<uint32_t, PacketRecord>::const_iterator it = m_packetRecords.begin();
         it != m_packetRecords.end(); ++it) {
        const PacketRecord& record = it->second;
        csvFile << record.packetId << ";"
                << record.packetSize << ";"
                << record.macs.receiver << ";"
                << record.receiverNodeId << ";"
                << record.receiverNodeType << ";"
                << record.macs.transmitter << ";"
                << record.transmitterNodeId << ";"
                << record.transmitterNodeType << ";"
                << record.macs.destination << ";"
                << record.destinationNodeId << ";"
                << record.destinationNodeType << ";"
                << record.macs.source << ";"
                << record.sourceNodeId << ";"
                << record.sourceNodeType << ";"
                << record.txBeginFirstSeen << ";"
                << record.rxEndLastSeen << ";"
                << record.txBeginCount << ";"
                << record.rxEndCount << ";"
                << record.rxDropCount << ";"
                << record.srcCoordinates.x << ";"
                << record.srcCoordinates.y << ";"
                << record.srcCoordinates.z << ";"
                << record.dstCoordinates.x << ";"
                << record.dstCoordinates.y << ";"
                << record.dstCoordinates.z << ";"
                << record.lastRxSignal
                << std::endl;
    }

    csvFile.close();
}

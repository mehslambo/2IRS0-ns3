#include "mac-stats.h"
#include <sys/stat.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <iostream>

using namespace ns3;

MacStats::MacStats(const std::string &scenarioName, const NodeContainer &staNodes)
    : m_scenarioName(scenarioName), m_staNodes(staNodes)
{
    std::string baseLogPath = "postprocessing/logs/" + scenarioName;
    std::string cmd = "mkdir -p " + baseLogPath;
    int result = system(cmd.c_str());
    if (result != 0)
    {
        std::cout << "Failed to create logging directory: " << baseLogPath << std::endl;
    }
    m_csvFilePath = baseLogPath + "/MacStats.csv";

    // Initialize stats for each STA node.
    for (uint32_t i = 0; i < m_staNodes.GetN(); i++) {
        Ptr<Node> node = m_staNodes.Get(i);
        uint32_t nodeId = node->GetId();
        m_nodeStats[nodeId].nodeType = "STA";
    }
}

bool MacStats::GetNodeIdFromContext(const std::string &context, uint32_t &nodeId)
{
    std::string::size_type pos = context.find("/NodeList/");
    if (pos == std::string::npos) {
        return false;
    }
    std::string nodeStr = context.substr(pos);
    if (sscanf(nodeStr.c_str(), "/NodeList/%u/", &nodeId) != 1) {
        std::cout << "Failed to parse node ID from context: " << context << std::endl;
        return false;
    }
    return true;
}

void MacStats::EnableLogging()
{
    // Connect each STA node's MAC trace sources.
    for (uint32_t i = 0; i < m_staNodes.GetN(); i++) {
        uint32_t nodeId = m_staNodes.Get(i)->GetId();
        std::string basePath = "/NodeList/" + std::to_string(nodeId);

        Config::Connect(basePath + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/Assoc",
                        MakeCallback(&MacStats::SetAssociation, this));
        Config::Connect(basePath + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/DeAssoc",
                        MakeCallback(&MacStats::UnsetAssociation, this));
        Config::Connect(basePath + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/NrOfTransmissionsDuringRAWSlot",
                        MakeCallback(&MacStats::OnNrOfTransmissionsDuringRAWSlotChanged, this));
        Config::Connect(basePath + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/PacketDropped",
                        MakeCallback(&MacStats::OnMacPacketDropped, this));
        Config::Connect(basePath + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/Collision",
                        MakeCallback(&MacStats::OnCollision, this));
        Config::Connect(basePath + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/TransmissionWillCrossRAWBoundary",
                        MakeCallback(&MacStats::OnTransmissionWillCrossRAWBoundary, this));
    }
}

void MacStats::SetAssociation(std::string context, Mac48Address address)
{
    uint32_t nodeId;
    if (!GetNodeIdFromContext(context, nodeId)) {
        return;
    }
    auto it = m_nodeStats.find(nodeId);
    if (it == m_nodeStats.end()) {
        std::cout << "Node ID " << nodeId << " not found in MAC stats map." << std::endl;
        return;
    }
    // Increment the number of association attempts.
    it->second.associationAttempts++;
}

void MacStats::UnsetAssociation(std::string context, Mac48Address address)
{
    uint32_t nodeId;
    if (!GetNodeIdFromContext(context, nodeId)) {
        return;
    }
    auto it = m_nodeStats.find(nodeId);
    if (it == m_nodeStats.end()) {
        std::cout << "Node ID " << nodeId << " not found in MAC stats map." << std::endl;
        return;
    }
    // Increment the number of deassociation attempts.
    it->second.deassociationAttempts++;
}

void MacStats::OnNrOfTransmissionsDuringRAWSlotChanged(std::string context, uint16_t oldValue, uint16_t newValue)
{
    uint32_t nodeId;
    if (!GetNodeIdFromContext(context, nodeId)) {
        return;
    }
    auto it = m_nodeStats.find(nodeId);
    if (it == m_nodeStats.end()) {
        std::cout << "Node ID " << nodeId << " not found in MAC stats map." << std::endl;
        return;
    }
    // Increment by the difference (if any) between the new and old values.
    if (newValue > oldValue) {
        it->second.totalTransmissionsDuringRAWSlot += (newValue - oldValue);
    }
}

void MacStats::OnMacPacketDropped(std::string context, Ptr<const Packet> packet, DropReason reason)
{
    uint32_t nodeId;
    if (!GetNodeIdFromContext(context, nodeId)) {
        return;
    }
    auto it = m_nodeStats.find(nodeId);
    if (it == m_nodeStats.end()) {
        std::cout << "Node ID " << nodeId << " not found in MAC stats map." << std::endl;
        return;
    }
    it->second.packetDroppedCount++;
}

void MacStats::OnCollision(std::string context, uint32_t nrOfBackoffSlots)
{
    uint32_t nodeId;
    if (!GetNodeIdFromContext(context, nodeId)) {
        return;
    }
    auto it = m_nodeStats.find(nodeId);
    if (it == m_nodeStats.end()) {
        std::cout << "Node ID " << nodeId << " not found in MAC stats map." << std::endl;
        return;
    }
    it->second.collisionBackoffSlots += nrOfBackoffSlots;
}

void MacStats::OnTransmissionWillCrossRAWBoundary(std::string context, Time txDuration, Time remainingTimeInRawSlot)
{
    uint32_t nodeId;
    if (!GetNodeIdFromContext(context, nodeId)) {
        return;
    }
    auto it = m_nodeStats.find(nodeId);
    if (it == m_nodeStats.end()) {
        std::cout << "Node ID " << nodeId << " not found in MAC stats map." << std::endl;
        return;
    }
    it->second.crossingCount++;
    it->second.totalTxDurationCrossingBoundary += txDuration;
}

void MacStats::DumpMacRecordsToCsv()
{
    m_csvFile.open(m_csvFilePath, std::ios::out | std::ios::trunc);
    if (!m_csvFile.is_open())
    {
        std::cout << "Failed to open CSV file: " << m_csvFilePath << std::endl;
        return;
    }

    // CSV header.
    m_csvFile << "NodeId;NodeType;AssociationAttempts;DeassociationAttempts;TotalTransmissionsDuringRAWSlot;PacketDroppedCount;CollisionBackoffSlots;CrossingCount;TotalTxDurationCrossingBoundary" << std::endl;

    // Write statistics for each node.
    for (const auto &entry : m_nodeStats)
    {
        uint32_t nodeId = entry.first;
        const MacStateStats &stats = entry.second;
        m_csvFile << nodeId << ";"
                  << stats.nodeType << ";"
                  << stats.associationAttempts << ";"
                  << stats.deassociationAttempts << ";"
                  << stats.totalTransmissionsDuringRAWSlot << ";"
                  << stats.packetDroppedCount << ";"
                  << stats.collisionBackoffSlots << ";"
                  << stats.crossingCount << ";"
                  << stats.totalTxDurationCrossingBoundary.GetSeconds()
                  << std::endl;
    }
    m_csvFile.close();
}

#include "power-logging-stats.h"
#include <sys/stat.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>

using namespace ns3;

PowerLoggingStats::PowerLoggingStats(const std::string &scenarioName,
                                     const NodeContainer &staNodes,
                                     const NodeContainer &apNodes)
    : m_scenarioName(scenarioName),
      m_staNodes(staNodes),
      m_apNodes(apNodes)
{
    std::string baseLogPath = "postprocessing/logs/" + scenarioName;
    std::string cmd = "mkdir -p " + baseLogPath;
    int result = system(cmd.c_str());
    if (result != 0)
    {
        std::cout << "Failed to create logging directory: " << baseLogPath << std::endl;
    }
    m_csvFilePath = baseLogPath + "/PowerStateStats.csv";

    for (uint32_t i = 0; i < m_staNodes.GetN(); i++) {
        Ptr<Node> node = m_staNodes.Get(i);
        uint32_t nodeId = node->GetId();
        m_nodeStats[nodeId].nodeType = "STA";
    }
    for (uint32_t i = 0; i < m_apNodes.GetN(); i++) {
        Ptr<Node> node = m_apNodes.Get(i);
        uint32_t nodeId = node->GetId();
        m_nodeStats[nodeId].nodeType = "AP";
    }
}

void PowerLoggingStats::EnableLogging()
{
    Config::Connect(
        "/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Phy/State/State",
        MakeCallback(&PowerLoggingStats::PhyStateChangeCallback, this));
}

void PowerLoggingStats::PhyStateChangeCallback(std::string context, const Time start,
                                               const Time duration, const WifiPhy::State state)
{
    std::string::size_type pos = context.find("/NodeList/");
    if (pos == std::string::npos)
    {
        // Context string not in expected format, ignore callback.
        return;
    }
    std::string nodeStr = context.substr(pos);
    uint32_t nodeId = 0;
    if (sscanf(nodeStr.c_str(), "/NodeList/%u/", &nodeId) != 1)
    {
        std::cout << "Failed to parse node ID from context: " << context << std::endl;
        return;
    }

    auto it = m_nodeStats.find(nodeId);
    if (it == m_nodeStats.end())
    {
        std::cout << "Node ID " << nodeId << " not found in the stats map." << std::endl;
        return;
    }

    // Update the corresponding state's duration for the node.
    switch (state)
    {
    case WifiPhy::State::SLEEP:
        it->second.sleep += duration;
        break;
    case WifiPhy::State::IDLE:
        it->second.idle += duration;
        break;
    case WifiPhy::State::TX:
        it->second.tx += duration;
        break;
    case WifiPhy::State::RX:
        it->second.rx += duration;
        break;
    case WifiPhy::State::CCA_BUSY:
        it->second.ccaBusy += duration;
        break;
    case WifiPhy::State::SWITCHING:
        it->second.switching += duration;
        break;
    default:
        break;
    }
}

void PowerLoggingStats::DumpPowerRecordsToCsv()
{
    // Open the CSV file for writing.
    m_csvFile.open(m_csvFilePath, std::ios::out | std::ios::trunc);
    if (!m_csvFile.is_open())
    {
        std::cout << "Failed to open CSV file: " << m_csvFilePath << std::endl;
        return;
    }

    // Write CSV header.
    m_csvFile << "NodeId;NodeType;SLEEPTime;IDLETime;TXTime;RXTime;CCA_BUSYTime;SWITCHINGTime" << std::endl;

    // Write out the collected statistics for each node.
    for (const auto &entry : m_nodeStats)
    {
        uint32_t nodeId = entry.first;
        const StateStats &stats = entry.second;
        m_csvFile << nodeId << ";"
                  << stats.nodeType << ";"
                  << stats.sleep << ";"
                  << stats.idle << ";"
                  << stats.tx << ";"
                  << stats.rx << ";"
                  << stats.ccaBusy << ";"
                  << stats.switching << std::endl;
    }
    m_csvFile.close();
}

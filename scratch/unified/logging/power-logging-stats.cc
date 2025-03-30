#include "power-logging-stats.h"
#include <sys/stat.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>
#include "ns3/energy-module.h"

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

    // Initialize stats for STA nodes.
    for (uint32_t i = 0; i < m_staNodes.GetN(); i++)
    {
        Ptr<Node> node = m_staNodes.Get(i);
        uint32_t nodeId = node->GetId();
        m_nodeStats[nodeId].nodeType = "STA";
        Ptr<NetDevice> device = node->GetDevice(0);
        Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice>(device);
        if (wifiDevice)
        {
            // The energy model is attached to the node, not the device.
            Ptr<EnergySourceContainer> sourceContainer = node->GetObject<EnergySourceContainer>();
            Ptr<WifiRadioEnergyModel> energyModel = nullptr;
            if (sourceContainer && sourceContainer->GetN() > 0)
            {
                Ptr<EnergySource> source = sourceContainer->Get(0);
                DeviceEnergyModelContainer deviceModels = source->FindDeviceEnergyModels("ns3::WifiRadioEnergyModel");
                if (deviceModels.GetN() > 0)
                {
                    energyModel = DynamicCast<WifiRadioEnergyModel>(deviceModels.Get(0));
                }
            }
            if (energyModel)
            {
                m_nodeStats[nodeId].energyModel = energyModel;
                m_nodeStats[nodeId].lastEnergy = energyModel->GetTotalEnergyConsumption();
            }
            else
            {
                std::cout << "Energy model not found for node " << nodeId << std::endl;
            }
        }
        else
        {
            std::cout << "WifiNetDevice not found for node " << nodeId << std::endl;
        }
    }

    // Initialize stats for AP nodes.
    for (uint32_t i = 0; i < m_apNodes.GetN(); i++)
    {
        Ptr<Node> node = m_apNodes.Get(i);
        uint32_t nodeId = node->GetId();
        m_nodeStats[nodeId].nodeType = "AP";
        Ptr<NetDevice> device = node->GetDevice(0);
        Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice>(device);
        if (wifiDevice)
        {
            Ptr<EnergySourceContainer> sourceContainer = node->GetObject<EnergySourceContainer>();
            Ptr<WifiRadioEnergyModel> energyModel = nullptr;
            if (sourceContainer && sourceContainer->GetN() > 0)
            {
                Ptr<EnergySource> source = sourceContainer->Get(0);
                DeviceEnergyModelContainer deviceModels = source->FindDeviceEnergyModels("ns3::WifiRadioEnergyModel");
                if (deviceModels.GetN() > 0)
                {
                    energyModel = DynamicCast<WifiRadioEnergyModel>(deviceModels.Get(0));
                }
            }
            if (energyModel)
            {
                m_nodeStats[nodeId].energyModel = energyModel;
                m_nodeStats[nodeId].lastEnergy = energyModel->GetTotalEnergyConsumption();
            }
            else
            {
                std::cout << "[PowerLoggingStats] Energy model not found for node " << nodeId << std::endl;
            }
        }
        else
        {
            std::cout << "[PowerLoggingStats] WifiNetDevice not found for node " << nodeId << std::endl;
        }
    }
}

void PowerLoggingStats::EnableLogging()
{
    // Connect the PHY state change callback to update time durations.
    Config::Connect(
        "/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Phy/State/State",
        MakeCallback(&PowerLoggingStats::PhyStateChangeCallback, this));

    for (auto &entry : m_nodeStats)
    {
        uint32_t nodeId = entry.first;
        Ptr<WifiRadioEnergyModel> energyModel = entry.second.energyModel;
        if (energyModel)
        {
            energyModel->TraceConnect("TotalEnergyConsumption", std::to_string(nodeId),
                MakeCallback(&PowerLoggingStats::TotalEnergyConsumptionCallback, this));
        }
    }
}

void PowerLoggingStats::PhyStateChangeCallback(std::string context, const Time start,
                                               const Time duration, const WifiPhy::State state)
{
    // Parse the node ID from the context string.
    std::string::size_type pos = context.find("/NodeList/");
    if (pos == std::string::npos)
    {
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

    // Update the time duration for the given state.
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

    // Record the current state so that the energy callback can know which energy field to update.
    it->second.currentState = state;
}

void PowerLoggingStats::TotalEnergyConsumptionCallback(std::string context,
                                                       double oldValue,
                                                       double newTotalEnergy)
{
    // Convert context (an int as a string) to a uint32_t node ID.
    uint32_t nodeId = 0;
    if (sscanf(context.c_str(), "%u", &nodeId) != 1)
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

    double energyDelta = newTotalEnergy - oldValue;
    switch (it->second.currentState)
    {
    case WifiPhy::State::SLEEP:
        it->second.sleepEnergy += energyDelta;
        break;
    case WifiPhy::State::IDLE:
        it->second.idleEnergy += energyDelta;
        break;
    case WifiPhy::State::TX:
        it->second.txEnergy += energyDelta;
        break;
    case WifiPhy::State::RX:
        it->second.rxEnergy += energyDelta;
        break;
    case WifiPhy::State::CCA_BUSY:
        it->second.ccaBusyEnergy += energyDelta;
        break;
    case WifiPhy::State::SWITCHING:
        it->second.switchingEnergy += energyDelta;
        break;
    default:
        break;
    }
    // Update the last energy reading.
    it->second.lastEnergy = newTotalEnergy;
}

void PowerLoggingStats::DumpPowerRecordsToCsv()
{
    m_csvFile.open(m_csvFilePath, std::ios::out | std::ios::trunc);
    if (!m_csvFile.is_open())
    {
        std::cout << "Failed to open CSV file: " << m_csvFilePath << std::endl;
        return;
    }

    m_csvFile << "NodeId;NodeType;SLEEPTime;IDLETime;TXTime;RXTime;CCA_BUSYTime;SWITCHINGTime;"
              << "SLEEPEnergy;IDLEEnergy;TXEnergy;RXEnergy;CCA_BUSYEnergy;SWITCHINGEnergy;TotalEnergy" << std::endl;

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
                  << stats.switching << ";"
                  << stats.sleepEnergy << ";"
                  << stats.idleEnergy << ";"
                  << stats.txEnergy << ";"
                  << stats.rxEnergy << ";"
                  << stats.ccaBusyEnergy << ";"
                  << stats.switchingEnergy << ";"
                  << stats.lastEnergy << std::endl;
    }
    m_csvFile.close();
}

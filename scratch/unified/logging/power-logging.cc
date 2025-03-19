#include "power-logging.h"
#include <sys/stat.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>

using namespace ns3;

PowerLogging::PowerLogging(const std::string& scenarioName, 
    const NodeContainer& staNodes, 
    const NodeContainer& apNodes)
: m_scenarioName(scenarioName)
, m_staNodes(staNodes)
, m_apNodes(apNodes)
{  
    
    // Create path with timestamp
    std::string baseLogPath = "postprocessing/logs/" + scenarioName;

    // Create logging directory if it doesn't exist
    std::string cmd = "mkdir -p " + baseLogPath;
    int result = system(cmd.c_str());
    if (result != 0) {
        std::cout<<"Failed to create logging directory: " << baseLogPath<<std::endl;
    }

    m_logFilePath = baseLogPath + "/PowerState.csv";
    
    // Open log file
    m_logFile.open(m_logFilePath, std::ios::out | std::ios::trunc);
    if (!m_logFile.is_open()) {
        std::cout<<"Failed to open log file: " << m_logFilePath<<std::endl;
    }

    WriteHeader();
}

void PowerLogging::EnableLogging()
{
    Config::Connect(
        "/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Phy/State/State",
        MakeCallback(&PowerLogging::PhyStateChangeCallback, this)
    );
}

std::string PowerLogging::GetNodePos(uint32_t nodeId) {
    // If nodeId is invalid, return placeholder values
    if (nodeId == (uint32_t)-1) {
        return "?;?;?;?";
    }

    // Find the node
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
    
    if (!node) {
        return std::to_string(nodeId) + "?;?;?;?";
    }

    // Get position information
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    Vector position = mobility ? mobility->GetPosition() : Vector(0, 0, 0);
    
    // Determine if node is AP or STA
    std::string nodeType = "STA";  // Default to STA
    for (uint32_t i = 0; i < node->GetNDevices(); ++i) {
        Ptr<WifiNetDevice> wifiDev = node->GetDevice(i)->GetObject<WifiNetDevice>();
        if (wifiDev) {
            Ptr<ApWifiMac> apMac = wifiDev->GetMac()->GetObject<ApWifiMac>();
            if (apMac) {
                nodeType = "AP";
                break;
            }
        }
    }
    
    std::stringstream ss;
    ss << position.x << ";"
       << position.y << ";"
       << position.z << ";"
       << nodeType;
    
    return ss.str();
}


void PowerLogging::PhyStateChangeCallback(std::string context, const Time start, const Time duration, const WifiPhy::State state)
{
    // Get node ID from context
	std::string::size_type pos = context.find("/NodeList/");
    std::string nodeStr = context.substr(pos);
    uint32_t nodeId;
    sscanf(nodeStr.c_str(), "/NodeList/%u/", &nodeId);

    std::string stateStr;
    switch (state) {
        case WifiPhy::State::SLEEP:
            stateStr = "SLEEP";
            break;
        case WifiPhy::State::IDLE:
            stateStr = "IDLE";
            break;
        case WifiPhy::State::TX:
            stateStr = "TX";
            break;
        case WifiPhy::State::RX:
            stateStr = "RX";
            break;
        case WifiPhy::State::CCA_BUSY:
            stateStr = "CCA_BUSY";
            break;
        case WifiPhy::State::SWITCHING:
            stateStr = "SWITCHING";
            break;
    }

    if(!m_logFile.is_open()) {
        std::cout << "Log file for PhyStateChangeCallback is not open" << std::endl;
        return;
    }

    m_logFile 
        << nodeId << ";"
        << GetNodePos(nodeId) << ";"
        << start << ";"
        << duration << ";"
        << stateStr << std::endl;
}

void PowerLogging::WriteHeader()
{
    if (m_logFile.is_open()) {
        m_logFile << "NodeId;NodeX;NodeY;NodeZ;NodeType;Start;Duration;State" << std::endl;
    }
}

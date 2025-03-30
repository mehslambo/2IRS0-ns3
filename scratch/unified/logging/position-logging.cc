#include "position-logging.h"
#include <sys/stat.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <cmath>

using namespace ns3;

PositionLogging::PositionLogging(const std::string& scenarioName, Time interval)
    : m_scenarioName(scenarioName),
      m_loggingInterval(interval)
{
    
}

void PositionLogging::EnableLogging()
{
    // Create path with timestamp
    std::string baseLogPath = "postprocessing/logs/" + m_scenarioName;

    // Create logging directory if it doesn't exist
    std::string cmd = "mkdir -p " + baseLogPath;
    int result = system(cmd.c_str());
    if (result != 0) {
        std::cout << "Failed to create logging directory: " << baseLogPath << std::endl;
    }

    m_logFilePath = baseLogPath + "/CourseChange.csv";
    
    // Open log file
    m_logFile.open(m_logFilePath, std::ios::out | std::ios::trunc);
    if (!m_logFile.is_open()) {
        std::cout << "Failed to open log file: " << m_logFilePath << std::endl;
    }

    WriteHeader();

    // Start the periodic position check
    Simulator::Schedule(Seconds(0), &PositionLogging::PeriodicPositionCheck, this);
}

void PositionLogging::PeriodicPositionCheck()
{
    // Iterate through all nodes in the simulation
    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {
        Ptr<Node> node = *i;
        uint32_t nodeId = node->GetId();
        
        Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
        if (mobility) {
            Vector position = mobility->GetPosition();
            
            // Determine if this node is an AP
            bool isAp = false;
            for (uint32_t j = 0; j < node->GetNDevices(); ++j) {
                Ptr<WifiNetDevice> wifiDev = node->GetDevice(j)->GetObject<WifiNetDevice>();
                if (wifiDev && wifiDev->GetMac()->GetObject<ApWifiMac>()) {
                    isAp = true;
                    break;
                }
            }
            
            // Check if position has changed since last log
            bool positionChanged = false;
            
            if (m_lastPositions.find(nodeId) == m_lastPositions.end()) {
                // First time seeing this node, log it
                positionChanged = true;
            } else {
                // Get the last position
                auto lastPos = m_lastPositions[nodeId];
                double lastX = std::get<0>(lastPos);
                double lastY = std::get<1>(lastPos);
                double lastZ = std::get<2>(lastPos);
                
                // Check if position has changed (with small epsilon to account for floating-point precision)
                const double epsilon = 0.0001; // 0.1mm precision
                if (std::abs(position.x - lastX) > epsilon || 
                    std::abs(position.y - lastY) > epsilon ||
                    std::abs(position.z - lastZ) > epsilon) {
                    positionChanged = true;
                }
            }
            
            // Log position if it changed
            if (positionChanged) {
                LogNodePosition(node, isAp, position);
                
                // Update the last position
                m_lastPositions[nodeId] = std::make_tuple(position.x, position.y, position.z);
            }
        }
    }
    
    // Schedule the next check
    Simulator::Schedule(m_loggingInterval, &PositionLogging::PeriodicPositionCheck, this);
}

void PositionLogging::LogNodePosition(Ptr<Node> node, bool isAp, const Vector& position)
{
    std::string nodeType = isAp ? "AP" : "STA";

    m_logFile << Simulator::Now() << ";"
              << nodeType << ";"
              << node->GetId() << ";"
              << position.x << ";"
              << position.y << ";"
              << position.z << std::endl;
}

void PositionLogging::WriteHeader()
{
    if (m_logFile.is_open()) {
        m_logFile << "Time;NodeType;NodeId;X;Y;Z" << std::endl;
    }
}
#include "position-logging.h"
#include <sys/stat.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>

using namespace ns3;

PositionLogging::PositionLogging(const std::string& scenarioName)
    : m_scenarioName(scenarioName)
{
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d_%H:%M");
    
    // Create path with timestamp
    std::string baseLogPath = "postprocessing/logs/" + scenarioName + "/" + ss.str();

    // Create logging directory if it doesn't exist
    std::string cmd = "mkdir -p " + baseLogPath;
    int result = system(cmd.c_str());
    if (result != 0) {
        std::cout<<"Failed to create logging directory: " << baseLogPath<<std::endl;
    }

    m_logFilePath = baseLogPath + "/CourseChange.csv";
    
    // Open log file
    m_logFile.open(m_logFilePath, std::ios::app);
    if (!m_logFile.is_open()) {
        std::cout<<"Failed to open log file: " << m_logFilePath<<std::endl;
    }

    WriteHeader();
}

void PositionLogging::EnableLogging()
{
    Config::Connect(
        "/NodeList/*/$ns3::MobilityModel/CourseChange",
        MakeCallback(&PositionLogging::CourseChangeCallback, this)
    );
}

void PositionLogging::CourseChangeCallback(std::string context, Ptr<const MobilityModel> mobility)
{
    Vector position = mobility->GetPosition();
    
    // Identify if this node is a station or AP
    Ptr<Node> node = mobility->GetObject<Node>();
    bool isAp = false;

    // Look through all devices on the node
    for (uint32_t i = 0; i < node->GetNDevices(); ++i) {
        Ptr<WifiNetDevice> wifiDev = node->GetDevice(i)->GetObject<WifiNetDevice>();
        if (wifiDev) {
            Ptr<ApWifiMac> apMac = wifiDev->GetMac()->GetObject<ApWifiMac>();
            if (apMac) {
                isAp = true;
                break;
            }
        }
    }

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

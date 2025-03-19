#ifndef POSITION_LOGGING_H
#define POSITION_LOGGING_H

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include <string>
#include <fstream>
#include <map>
#include <tuple>

using namespace ns3;

class PositionLogging {
public:
    PositionLogging(const std::string& scenarioName, Time interval = Seconds(1.0));
    void EnableLogging();
    
private:
    std::string m_scenarioName;
    std::string m_logFilePath;
    std::ofstream m_logFile;
    Time m_loggingInterval;
    
    // Map to store the last logged position for each node: nodeId -> (x,y,z)
    std::map<uint32_t, std::tuple<double, double, double>> m_lastPositions;
    
    void PeriodicPositionCheck();
    void LogNodePosition(Ptr<Node> node, bool isAp, const Vector& position);
    std::string GetLogFilePath() const;
    void WriteHeader();
};

#endif // POSITION_LOGGING_H
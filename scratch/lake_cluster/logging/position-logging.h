#ifndef POSITION_LOGGING_H
#define POSITION_LOGGING_H

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include <string>
#include <fstream>

using namespace ns3;

class PositionLogging {
public:
    PositionLogging(const std::string& scenarioName);
    void EnableLogging();
    
private:
    std::string m_scenarioName;
    std::string m_logFilePath;
    std::ofstream m_logFile;

    void CourseChangeCallback(std::string context, Ptr<const MobilityModel> mobility);
    std::string GetLogFilePath() const;
    void WriteHeader();
};

#endif // POSITION_LOGGING_H

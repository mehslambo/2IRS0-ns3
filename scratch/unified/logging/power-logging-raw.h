#ifndef POWER_LOGGING_RAW_H
#define POWER_LOGGING_RAW_H

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include <string>
#include <fstream>

using namespace ns3;

class PowerLoggingRaw {
public:
    PowerLoggingRaw(const std::string& scenarioName, const NodeContainer& staNodes, const NodeContainer& apNodes);
    void EnableLogging();
    
private:
    std::string m_scenarioName;
    std::string m_logFilePath;
    std::ofstream m_logFile;
    NodeContainer m_staNodes;
    NodeContainer m_apNodes;

    void PhyStateChangeCallback(std::string context, const Time start,	const Time duration, const WifiPhy::State state);
    std::string GetLogFilePath() const;
    std::string GetNodePos(uint32_t nodeId);
    void WriteHeader();
};

#endif // POWER_LOGGING_RAW_H

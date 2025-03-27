#ifndef POWER_LOGGING_STATS_H
#define POWER_LOGGING_STATS_H

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include <string>
#include <fstream>
#include <map>

using namespace ns3;

class PowerLoggingStats {
public:
  PowerLoggingStats(const std::string& scenarioName,
                    const NodeContainer& staNodes,
                    const NodeContainer& apNodes);

  void EnableLogging();

  void DumpPowerRecordsToCsv();

private:
  std::string m_scenarioName;
  std::string m_csvFilePath;
  std::ofstream m_csvFile;
  NodeContainer m_staNodes;
  NodeContainer m_apNodes;

  // Structure to store time durations for each PHY state.
  struct StateStats {
    std::string nodeType; // AP/STA
    Time sleep;
    Time idle;
    Time tx;
    Time rx;
    Time ccaBusy;
    Time switching;
    StateStats() 
      : nodeType("?"),
        sleep(Seconds(0)),
        idle(Seconds(0)),
        tx(Seconds(0)),
        rx(Seconds(0)),
        ccaBusy(Seconds(0)),
        switching(Seconds(0))
    {}
  };

  // Map node Id -> state statistics.
  std::map<uint32_t, StateStats> m_nodeStats;

  void PhyStateChangeCallback(std::string context, const Time start,
                              const Time duration, const WifiPhy::State state);
};

#endif // POWER_LOGGING_STATS_H

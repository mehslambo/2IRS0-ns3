#ifndef POWER_LOGGING_STATS_H
#define POWER_LOGGING_STATS_H

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/wifi-radio-energy-model.h"
#include <string>
#include <fstream>
#include <map>

using namespace ns3;

class PowerLoggingAgg {
public:
  PowerLoggingAgg(const std::string& scenarioName,
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
    // Energy consumption (Joules) breakdown for each state.
    double sleepEnergy;
    double idleEnergy;
    double txEnergy;
    double rxEnergy;
    double ccaBusyEnergy;
    double switchingEnergy;
    // The last total energy reading (Joules).
    double lastEnergy;
    // The current PHY state of the node.
    WifiPhy::State currentState;
    // Pointer to the energy model associated with the node.
    Ptr<WifiRadioEnergyModel> energyModel;

    StateStats() 
      : nodeType("?"),
        sleep(Seconds(0)),
        idle(Seconds(0)),
        tx(Seconds(0)),
        rx(Seconds(0)),
        ccaBusy(Seconds(0)),
        switching(Seconds(0)),
        sleepEnergy(0.0),
        idleEnergy(0.0),
        txEnergy(0.0),
        rxEnergy(0.0),
        ccaBusyEnergy(0.0),
        switchingEnergy(0.0),
        lastEnergy(0.0),
        currentState(WifiPhy::State::SLEEP),
        energyModel(0)
    {}
  };

  // Map: node Id -> state statistics.
  std::map<uint32_t, StateStats> m_nodeStats;

  // Callback to update time durations and current state.
  void PhyStateChangeCallback(std::string context, const Time start,
                              const Time duration, const WifiPhy::State state);

  void TotalEnergyConsumptionCallback(std::string context, double oldValue, double newTotalEnergy);
};

#endif // POWER_LOGGING_STATS_H

#ifndef MAC_STATS_H
#define MAC_STATS_H

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include <string>
#include <fstream>
#include <map>

using namespace ns3;

class MacStats {
public:
  MacStats(const std::string &scenarioName, const NodeContainer &staNodes);

  void EnableLogging();

  // Dump collected MAC statistics to CSV.
  void DumpMacRecordsToCsv();

  // Callback handlers for NS‑3 traces.
  void SetAssociation(std::string context, Mac48Address address);
  void UnsetAssociation(std::string context, Mac48Address address);
  void OnNrOfTransmissionsDuringRAWSlotChanged(std::string context, uint16_t oldValue, uint16_t newValue);
  void OnMacPacketDropped(std::string context, Ptr<const Packet> packet, DropReason reason);
  void OnCollision(std::string context, uint32_t nrOfBackoffSlots);
  void OnTransmissionWillCrossRAWBoundary(std::string context, Time txDuration, Time remainingTimeInRawSlot);

private:
  std::string m_scenarioName;
  std::string m_csvFilePath;
  std::ofstream m_csvFile;
  NodeContainer m_staNodes;

  struct MacStateStats {
    std::string nodeType; // STA/AP
    uint32_t associationAttempts;
    uint32_t deassociationAttempts;
    uint32_t totalTransmissionsDuringRAWSlot;
    uint32_t packetDroppedCount;
    uint32_t collisionBackoffSlots;
    uint32_t crossingCount;
    Time totalTxDurationCrossingBoundary;
    MacStateStats()
      : nodeType("?"),
        associationAttempts(0),
        deassociationAttempts(0),
        totalTransmissionsDuringRAWSlot(0),
        packetDroppedCount(0),
        collisionBackoffSlots(0),
        crossingCount(0),
        totalTxDurationCrossingBoundary(Seconds(0))
    {}
  };

  // Map node ID -> MAC statistics.
  std::map<uint32_t, MacStateStats> m_nodeStats;

  // Helper to extract node ID from the context string.
  bool GetNodeIdFromContext(const std::string &context, uint32_t &nodeId);
};

#endif // MAC_STATS_H

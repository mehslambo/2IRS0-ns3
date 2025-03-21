#ifndef PACKET_LOGGING_STATS_H
#define PACKET_LOGGING_STATS_H

#include <map>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

// A simple struct to hold MAC addresses extracted from a Wifi header.
struct MacAddresses {
    Mac48Address receiver;
    Mac48Address transmitter;
    Mac48Address destination;
    Mac48Address source;
    Mac48Address bssid;
};

struct PacketRecord {
    uint32_t packetId;
    uint32_t packetSize;
    MacAddresses macs;
    uint32_t receiverNodeId;
    std::string receiverNodeType;
    uint32_t transmitterNodeId;
    std::string transmitterNodeType;
    uint32_t destinationNodeId;
    std::string destinationNodeType;
    uint32_t sourceNodeId;
    std::string sourceNodeType;
    Time txBeginFirstSeen;  // time first seen in TX Begin callback
    Time rxEndLastSeen;   // time last seen in RX End callback
    uint32_t txBeginCount;  // number of times TX ended
    uint32_t rxEndCount;  // number of times RX ended
    uint32_t rxDropCount; // number of times RX dropped
    Vector srcCoordinates; // Source node coordinates at last RX event
    Vector dstCoordinates; // Destination node coordinates at last RX event
    double lastRxSignal; // Last received signal strength in dBm

    PacketRecord()
      : packetId(0), packetSize(0),
        receiverNodeId(0), transmitterNodeId(0),
        destinationNodeId(0), sourceNodeId(0),
        txBeginCount(0), rxEndCount(0), rxDropCount(0),
        srcCoordinates(0,0,0), dstCoordinates(0,0,0), lastRxSignal(-1000.0)
    {}
};

class PacketLoggingStats {
public:
    PacketLoggingStats(const std::string& scenarioName, const NodeContainer& staNodes, const NodeContainer& apNodes);
    void EnableLogging();

    // Dump the packet records summary to the console.
    void DumpPacketRecords() const;
    // Dump the packet records to a CSV file.
    void DumpPacketRecordsToCsv() const;

private:
    std::string m_scenarioName;
    NodeContainer m_staNodes;
    NodeContainer m_apNodes;
    std::map<uint32_t, PacketRecord> m_packetRecords;

    // Callbacks that update the per‐packet records.
    void TxBeginCallback(std::string context, Ptr<const Packet> packet);
    void RxEndCallback(std::string context, Ptr<const Packet> packet);
    void RxDropCallback(std::string context, Ptr<const Packet> packet, DropReason reason);
    void MonitorSnifferRxCallback(std::string context, Ptr<const Packet> packet, 
        uint16_t channelFreqMhz, uint16_t channelNumber, 
        uint32_t rate, bool isShortPreamble, 
        WifiTxVector txVector,
        double signalDbm, double noiseDbm);

    // Helper methods.
    MacAddresses ExtractMacAddresses(const WifiMacHeader& header);
    uint32_t GetNodeIdFromMacAddress(const Mac48Address& addr);
    std::string GetNodeType(uint32_t nodeId);
    Vector GetNodeCoordinates(uint32_t nodeId);
};

#endif // PACKET_LOGGING_STATS_H

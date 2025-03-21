#ifndef PACKET_LOGGING_H
#define PACKET_LOGGING_H

#include <sys/stat.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include <map>
#include <string>
#include <fstream>

using namespace ns3;

struct MacAddresses {
    Mac48Address receiver;
    Mac48Address transmitter;
    Mac48Address destination;
    Mac48Address source;
    Mac48Address bssid;
};

class PacketLoggingRaw {
public:
    PacketLoggingRaw(const std::string& scenarioName, const NodeContainer& staNodes, const NodeContainer& apNodes);
    MacAddresses ExtractMacAddresses(const WifiMacHeader& header);
    
    void EnableLogging();
    
private:
    std::string m_scenarioName;
    std::string m_baseLogPath;
    NodeContainer m_staNodes;
    NodeContainer m_apNodes;
    std::map<std::string, std::ofstream> m_logFiles;  // Store open file streams

    void MonitorSnifferRxCallback(std::string context, Ptr<const Packet> packet, 
        uint16_t channelFreqMhz, uint16_t channelNumber, 
        uint32_t rate, bool isShortPreamble, 
        WifiTxVector txVector,
        double signalDbm, double noiseDbm);

    void MonitorSnifferTxCallback(std::string context, Ptr<const Packet> packet, 
        uint16_t channelFreqMhz, uint16_t channelNumber,
        uint32_t rate, bool isShortPreamble,
        WifiTxVector txVector);

    void PhyTxRxBeginEndCallback(std::string context, Ptr<const Packet> packet);

    void PhyTxRxDropCallback(std::string context, Ptr<const Packet> packet, DropReason reason);

    uint32_t GetNodeIdFromMacAddress(const Mac48Address& addr);
    std::string GetNodePos(uint32_t nodeId);
    std::string PacketToCsv(Ptr<const Packet> packet);
    std::string GetLogFilePath(const std::string& eventType) const;

    void WriteHeaders();
    static const std::map<std::string, std::string> CSV_HEADERS;
};


#endif // PACKET_LOGGING_H
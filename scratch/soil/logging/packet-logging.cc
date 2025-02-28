#include "packet-logging.h"
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

const std::map<std::string, std::string> PacketLogging::CSV_HEADERS = {
    {"MonitorSnifferRx", "Time;SnifferNodeId;PacketId;PacketSize;ReceiverMac;ReceiverNodeId;TransmitterMac;TransmitterNodeId;BSSID;BSSIDNodeId;DestinationMac;DestinationNodeId;SourceMac;SourceNodeId;IsRetry;ChannelFreqMHz;ChannelNumber;Rate;IsShortPreamble;Mode;Retries;Ness;Nss;IsShortGuardInterval;IsStbc;TxPowerLevel;NoiseDbm;SignalDbm"},
    {"MonitorSnifferTx", "Time;SnifferNodeId;PacketId;PacketSize;ReceiverMac;ReceiverNodeId;TransmitterMac;TransmitterNodeId;BSSID;BSSIDNodeId;DestinationMac;DestinationNodeId;SourceMac;SourceNodeId;IsRetry;ChannelFreqMHz;ChannelNumber;Rate;IsShortPreamble;Mode;Retries;Ness;Nss;IsShortGuardInterval;IsStbc;TxPowerLevel"},
    {"PhyTxBegin", "Time;SnifferNodeId;PacketId;PacketSize;ReceiverMac;ReceiverNodeId;TransmitterMac;TransmitterNodeId;BSSID;BSSIDNodeId;DestinationMac;DestinationNodeId;SourceMac;SourceNodeId;IsRetry"},
    {"PhyTxEnd", "Time;SnifferNodeId;PacketId;PacketSize;ReceiverMac;ReceiverNodeId;TransmitterMac;TransmitterNodeId;BSSID;BSSIDNodeId;DestinationMac;DestinationNodeId;SourceMac;SourceNodeId;IsRetry"},
    {"PhyTxDrop", "Time;SnifferNodeId;PacketId;PacketSize;ReceiverMac;ReceiverNodeId;TransmitterMac;TransmitterNodeId;BSSID;BSSIDNodeId;DestinationMac;DestinationNodeId;SourceMac;SourceNodeId;IsRetry"},
    {"PhyRxBegin", "Time;SnifferNodeId;PacketId;PacketSize;ReceiverMac;ReceiverNodeId;TransmitterMac;TransmitterNodeId;BSSID;BSSIDNodeId;DestinationMac;DestinationNodeId;SourceMac;SourceNodeId;IsRetry"},
    {"PhyRxEnd", "Time;SnifferNodeId;PacketId;PacketSize;ReceiverMac;ReceiverNodeId;TransmitterMac;TransmitterNodeId;BSSID;BSSIDNodeId;DestinationMac;DestinationNodeId;SourceMac;SourceNodeId;IsRetry"},
    {"PhyRxDrop", "Time;SnifferNodeId;PacketId;PacketSize;ReceiverMac;ReceiverNodeId;TransmitterMac;TransmitterNodeId;BSSID;BSSIDNodeId;DestinationMac;DestinationNodeId;SourceMac;SourceNodeId;IsRetry"}
};

PacketLogging::PacketLogging(const std::string& scenarioName, 
                           const NodeContainer& staNodes, 
                           const NodeContainer& apNodes)
    : m_scenarioName(scenarioName)
    , m_staNodes(staNodes)
    , m_apNodes(apNodes)
{
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d_%H:%M");
    
    // Create path with timestamp
    m_baseLogPath = "postprocessing/logs/" + scenarioName + "/" + ss.str();

    // Create logging directory if it doesn't exist
    std::string cmd = "mkdir -p " + m_baseLogPath;
    int result = system(cmd.c_str());
    if (result != 0) {
        std::cout << "Failed to create logging directory: " << m_baseLogPath << std::endl;
    }

    // Open all required log files
    std::vector<std::string> eventTypes = {
        "MonitorSnifferRx", "MonitorSnifferTx", 
        "PhyTxBegin", "PhyTxEnd", "PhyTxDrop",
        "PhyRxBegin", "PhyRxEnd", "PhyRxDrop"
    };

    for (const auto& eventType : eventTypes) {
        std::string filePath = GetLogFilePath(eventType);
        m_logFiles[eventType].open(filePath, std::ios::app);
        if (!m_logFiles[eventType].is_open()) {
            std::cout << "Failed to open log file: " << filePath << std::endl;
        }
    }

    // Write headers to all files after opening them
    WriteHeaders();
}

void PacketLogging::EnableLogging()
{
    // Connect all logging callbacks
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxBegin",
        MakeCallback(&PacketLogging::PhyTxRxBeginDropEndCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxEnd",
        MakeCallback(&PacketLogging::PhyTxRxBeginDropEndCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop",
        MakeCallback(&PacketLogging::PhyTxRxBeginDropEndCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxBegin",
        MakeCallback(&PacketLogging::PhyTxRxBeginDropEndCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxEnd",
        MakeCallback(&PacketLogging::PhyTxRxBeginDropEndCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop",
        MakeCallback(&PacketLogging::PhyTxRxBeginDropEndCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferTx",
        MakeCallback(&PacketLogging::MonitorSnifferTxCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx",
        MakeCallback(&PacketLogging::MonitorSnifferRxCallback, this));
}

std::string PacketLogging::GetLogFilePath(const std::string& eventType) const
{
    return m_baseLogPath + "/" + eventType + ".csv";
}

uint32_t PacketLogging::GetNodeIdFromMacAddress(const Mac48Address& addr) {
    // Check station nodes
    for (uint32_t i = 0; i < m_staNodes.GetN(); i++) {
        Ptr<WifiNetDevice> dev = m_staNodes.Get(i)->GetDevice(0)->GetObject<WifiNetDevice>();
        if (dev && dev->GetMac()->GetAddress() == addr) {
            return m_staNodes.Get(i)->GetId();
        }
    }
    
    for (uint32_t i = 0; i < m_apNodes.GetN(); i++) {
        Ptr<WifiNetDevice> dev = m_apNodes.Get(i)->GetDevice(0)->GetObject<WifiNetDevice>();
        if (dev && dev->GetMac()->GetAddress() == addr) {
            return m_apNodes.Get(i)->GetId();
        }
    }
    
    return -1; // Return -1 if not found
}

std::string PacketLogging::PacketToCsv(Ptr<const Packet> packet){
	// Extract MAC header for source/destination info
	WifiMacHeader header;
	packet->PeekHeader(header);
	
	// Get MAC addresses based on ToDS and FromDS bits
	bool toDS = header.IsToDs();
	bool fromDS = header.IsFromDs();
	
	Mac48Address receiverMac = header.GetAddr1(); // Always the receiver
	Mac48Address transmitterMac = header.GetAddr2(); // Always the transmitter
	Mac48Address destinationMac;
	Mac48Address sourceMac;
	Mac48Address bssid;
	
	if (!toDS && !fromDS) {
		// Ad-hoc: addr3 is BSSID
		destinationMac = header.GetAddr1();
		sourceMac = header.GetAddr2(); 
		bssid = header.GetAddr3();
	}
	else if (!toDS && fromDS) {
		// From AP to STA
		destinationMac = header.GetAddr1();
		bssid = header.GetAddr2();
		sourceMac = header.GetAddr3();
	}
	else if (toDS && !fromDS) {
		// From STA to AP
		bssid = header.GetAddr1();
		sourceMac = header.GetAddr2();
		destinationMac = header.GetAddr3();
	}
	else {
		// WDS: addr4 exists
		bssid = Mac48Address(); // No BSSID
		destinationMac = header.GetAddr3();
		sourceMac = header.GetAddr4();
	}
	
	std::stringstream ss;

	ss << packet->GetUid() << ";"
	   << packet->GetSize() << ";" 
	   << receiverMac << ";"
	   << (GetNodeIdFromMacAddress(receiverMac) == ((uint32_t) -1) ? "?" : std::to_string(GetNodeIdFromMacAddress(receiverMac))) << ";"
	   << transmitterMac << ";"
	   << (GetNodeIdFromMacAddress(transmitterMac) == ((uint32_t) -1) ? "?" : std::to_string(GetNodeIdFromMacAddress(transmitterMac))) << ";"
	   << bssid << ";"
	   << (GetNodeIdFromMacAddress(bssid) == ((uint32_t) -1) ? "?" : std::to_string(GetNodeIdFromMacAddress(bssid))) << ";"
	   << destinationMac << ";"
	   << (GetNodeIdFromMacAddress(destinationMac) == ((uint32_t) -1) ? "?" : std::to_string(GetNodeIdFromMacAddress(destinationMac))) << ";"
	   << sourceMac << ";"
	   << (GetNodeIdFromMacAddress(sourceMac) == ((uint32_t) -1) ? "?" : std::to_string(GetNodeIdFromMacAddress(sourceMac))) << ";"
       << (header.IsRetry() ? "true" : "false");

	return ss.str();
}



void PacketLogging::MonitorSnifferRxCallback(std::string context, Ptr<const Packet> packet, 
    uint16_t channelFreqMhz, uint16_t channelNumber, 
    uint32_t rate, bool isShortPreamble, 
    WifiTxVector txVector,
    double signalDbm, double noiseDbm)
{
	// Get interceptor node ID from context
	std::string::size_type pos = context.find("/NodeList/");
    std::string nodeStr = context.substr(pos);
    uint32_t rxSnifferNodeId;
    sscanf(nodeStr.c_str(), "/NodeList/%u/", &rxSnifferNodeId);

	// If interceptor != intended recipient, skip logging
	//if (rxSnifferNodeId != GetNodeIdFromMacAddress(destinationMac)) 
	//    return;

    // Cast values to int to avoid null bytes
    int retries = static_cast<int>(txVector.GetRetries());
    int ness = static_cast<int>(txVector.GetNess());
    int nss = static_cast<int>(txVector.GetNss());
    int txPowerLevel = static_cast<int>(txVector.GetTxPowerLevel());

    auto& logFile = m_logFiles["MonitorSnifferRx"];
    if(!logFile.is_open()) {
        std::cout << "Log file for MonitorSnifferRx is not open" << std::endl;
        return;
    }

    // Write CSV line with all packet information
    logFile << Simulator::Now() << ";"
            << rxSnifferNodeId << ";"
            << PacketToCsv(packet) << ";" 
            << channelFreqMhz << ";"
            << channelNumber << ";"
            << rate << ";"
            << (isShortPreamble ? "true" : "false") << ";"
			<< txVector.GetMode().GetUniqueName() << ";"
            << retries << ";"
            << ness << ";"
            << nss << ";"
            << (txVector.IsShortGuardInterval() ? "true" : "false") << ";"
            << (txVector.IsStbc() ? "true" : "false") << ";"
            << txPowerLevel << ";"
            << noiseDbm << ";"
            << signalDbm << std::endl;
}

void PacketLogging::MonitorSnifferTxCallback(std::string context, Ptr<const Packet> packet, 
    uint16_t channelFreqMhz, uint16_t channelNumber,
    uint32_t rate, bool isShortPreamble,
    WifiTxVector txVector)
{

    // Get transmission sniffer node ID from context
    std::string::size_type pos = context.find("/NodeList/");
    std::string nodeStr = context.substr(pos);
    uint32_t txSnifferNodeId;
    sscanf(nodeStr.c_str(), "/NodeList/%u/", &txSnifferNodeId);

    // Cast values to int to avoid null bytes
    int retries = static_cast<int>(txVector.GetRetries());
    int ness = static_cast<int>(txVector.GetNess());
    int nss = static_cast<int>(txVector.GetNss());
    int txPowerLevel = static_cast<int>(txVector.GetTxPowerLevel());

    auto& logFile = m_logFiles["MonitorSnifferTx"];
    if(!logFile.is_open()) {
        std::cout << "Log file for MonitorSnifferTx is not open" << std::endl;
        return;
    }

    // Write CSV line with all packet information
    logFile << Simulator::Now() << ";"
            << txSnifferNodeId << ";"
			<< PacketToCsv(packet) << ";"
			<< channelFreqMhz << ";"
            << channelNumber << ";"
            << rate << ";"
            << (isShortPreamble ? "true" : "false") << ";"
            << txVector.GetMode().GetUniqueName() << ";"
            << retries << ";"
            << ness << ";"
            << nss << ";"
            << (txVector.IsShortGuardInterval() ? "true" : "false") << ";"
            << (txVector.IsStbc() ? "true" : "false") << ";"
            << txPowerLevel << std::endl;
}

void PacketLogging::PhyTxRxBeginDropEndCallback(std::string context, Ptr<const Packet> packet){
    std::string::size_type traceSourcePos = context.find_last_of("/");
    std::string traceSource = context.substr(traceSourcePos + 1);

    // Get sniffer node ID from context
    std::string::size_type pos = context.find("/NodeList/");
    std::string nodeStr = context.substr(pos);
    uint32_t snifferNodeId;
    sscanf(nodeStr.c_str(), "/NodeList/%u/", &snifferNodeId);

    auto& logFile = m_logFiles[traceSource];
    if(!logFile.is_open()) {
        std::cout << "Log file for " << traceSource << " is not open" << std::endl;
        return;
    }

	logFile << Simulator::Now() << ";"
            << snifferNodeId << ";"
			<< PacketToCsv(packet) << std::endl;
}

void PacketLogging::WriteHeaders()
{
    for (auto it = m_logFiles.begin(); it != m_logFiles.end(); ++it) {
        const std::string& eventType = it->first;
        std::ofstream& logFile = it->second;
        if (logFile.is_open()) {
            logFile << CSV_HEADERS.at(eventType) << std::endl;
        }
    }
}

#include "phy-sniffers-raw.h"
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

const std::map<std::string, std::string> PhySniffersRaw::CSV_HEADERS = {
    {"MonitorSnifferRx", "Time;SnifferNodeId;SnifferNodeX;SnifferNodeY;SnifferNodeZ;SnifferNodeType;PacketId;PacketSize;ReceiverMac;ReceiverNodeId;ReceiverNodeX;ReceiverNodeY;ReceiverNodeZ;ReceiverNodeType;TransmitterMac;TransmitterNodeId;TransmitterNodeX;TransmitterNodeY;TransmitterNodeZ;TransmitterNodeType;BSSID;BSSIDNodeId;BSSIDNodeX;BSSIDNodeY;BSSIDNodeZ;BSSIDNodeType;DestinationMac;DestinationNodeId;DestinationNodeX;DestinationNodeY;DestinationNodeZ;DestinationNodeType;SourceMac;SourceNodeId;SourceNodeX;SourceNodeY;SourceNodeZ;SourceNodeType;IsRetry;ChannelFreqMHz;ChannelNumber;Rate;IsShortPreamble;Mode;Retries;Ness;Nss;IsShortGuardInterval;IsStbc;TxPowerLevel;NoiseDbm;SignalDbm"},
    {"MonitorSnifferTx", "Time;SnifferNodeId;SnifferNodeX;SnifferNodeY;SnifferNodeZ;SnifferNodeType;PacketId;PacketSize;ReceiverMac;ReceiverNodeId;ReceiverNodeX;ReceiverNodeY;ReceiverNodeZ;ReceiverNodeType;TransmitterMac;TransmitterNodeId;TransmitterNodeX;TransmitterNodeY;TransmitterNodeZ;TransmitterNodeType;BSSID;BSSIDNodeId;BSSIDNodeX;BSSIDNodeY;BSSIDNodeZ;BSSIDNodeType;DestinationMac;DestinationNodeId;DestinationNodeX;DestinationNodeY;DestinationNodeZ;DestinationNodeType;SourceMac;SourceNodeId;SourceNodeX;SourceNodeY;SourceNodeZ;SourceNodeType;IsRetry;ChannelFreqMHz;ChannelNumber;Rate;IsShortPreamble;Mode;Retries;Ness;Nss;IsShortGuardInterval;IsStbc;TxPowerLevel"},
    {"PhyTxBegin", "Time;SnifferNodeId;SnifferNodeX;SnifferNodeY;SnifferNodeZ;SnifferNodeType;PacketId;PacketSize;ReceiverMac;ReceiverNodeId;ReceiverNodeX;ReceiverNodeY;ReceiverNodeZ;ReceiverNodeType;TransmitterMac;TransmitterNodeId;TransmitterNodeX;TransmitterNodeY;TransmitterNodeZ;TransmitterNodeType;BSSID;BSSIDNodeId;BSSIDNodeX;BSSIDNodeY;BSSIDNodeZ;BSSIDNodeType;DestinationMac;DestinationNodeId;DestinationNodeX;DestinationNodeY;DestinationNodeZ;DestinationNodeType;SourceMac;SourceNodeId;SourceNodeX;SourceNodeY;SourceNodeZ;SourceNodeType;IsRetry"},
    {"PhyTxEnd", "Time;SnifferNodeId;SnifferNodeX;SnifferNodeY;SnifferNodeZ;SnifferNodeType;PacketId;PacketSize;ReceiverMac;ReceiverNodeId;ReceiverNodeX;ReceiverNodeY;ReceiverNodeZ;ReceiverNodeType;TransmitterMac;TransmitterNodeId;TransmitterNodeX;TransmitterNodeY;TransmitterNodeZ;TransmitterNodeType;BSSID;BSSIDNodeId;BSSIDNodeX;BSSIDNodeY;BSSIDNodeZ;BSSIDNodeType;DestinationMac;DestinationNodeId;DestinationNodeX;DestinationNodeY;DestinationNodeZ;DestinationNodeType;SourceMac;SourceNodeId;SourceNodeX;SourceNodeY;SourceNodeZ;SourceNodeType;IsRetry"},
    {"PhyTxDropWithReason", "Time;SnifferNodeId;SnifferNodeX;SnifferNodeY;SnifferNodeZ;SnifferNodeType;PacketId;PacketSize;ReceiverMac;ReceiverNodeId;ReceiverNodeX;ReceiverNodeY;ReceiverNodeZ;ReceiverNodeType;TransmitterMac;TransmitterNodeId;TransmitterNodeX;TransmitterNodeY;TransmitterNodeZ;TransmitterNodeType;BSSID;BSSIDNodeId;BSSIDNodeX;BSSIDNodeY;BSSIDNodeZ;BSSIDNodeType;DestinationMac;DestinationNodeId;DestinationNodeX;DestinationNodeY;DestinationNodeZ;DestinationNodeType;SourceMac;SourceNodeId;SourceNodeX;SourceNodeY;SourceNodeZ;SourceNodeType;IsRetry;DropReason"},
    {"PhyRxBegin", "Time;SnifferNodeId;SnifferNodeX;SnifferNodeY;SnifferNodeZ;SnifferNodeType;PacketId;PacketSize;ReceiverMac;ReceiverNodeId;ReceiverNodeX;ReceiverNodeY;ReceiverNodeZ;ReceiverNodeType;TransmitterMac;TransmitterNodeId;TransmitterNodeX;TransmitterNodeY;TransmitterNodeZ;TransmitterNodeType;BSSID;BSSIDNodeId;BSSIDNodeX;BSSIDNodeY;BSSIDNodeZ;BSSIDNodeType;DestinationMac;DestinationNodeId;DestinationNodeX;DestinationNodeY;DestinationNodeZ;DestinationNodeType;SourceMac;SourceNodeId;SourceNodeX;SourceNodeY;SourceNodeZ;SourceNodeType;IsRetry"},
    {"PhyRxEnd", "Time;SnifferNodeId;SnifferNodeX;SnifferNodeY;SnifferNodeZ;SnifferNodeType;PacketId;PacketSize;ReceiverMac;ReceiverNodeId;ReceiverNodeX;ReceiverNodeY;ReceiverNodeZ;ReceiverNodeType;TransmitterMac;TransmitterNodeId;TransmitterNodeX;TransmitterNodeY;TransmitterNodeZ;TransmitterNodeType;BSSID;BSSIDNodeId;BSSIDNodeX;BSSIDNodeY;BSSIDNodeZ;BSSIDNodeType;DestinationMac;DestinationNodeId;DestinationNodeX;DestinationNodeY;DestinationNodeZ;DestinationNodeType;SourceMac;SourceNodeId;SourceNodeX;SourceNodeY;SourceNodeZ;SourceNodeType;IsRetry"},
    {"PhyRxDropWithReason", "Time;SnifferNodeId;SnifferNodeX;SnifferNodeY;SnifferNodeZ;SnifferNodeType;PacketId;PacketSize;ReceiverMac;ReceiverNodeId;ReceiverNodeX;ReceiverNodeY;ReceiverNodeZ;ReceiverNodeType;TransmitterMac;TransmitterNodeId;TransmitterNodeX;TransmitterNodeY;TransmitterNodeZ;TransmitterNodeType;BSSID;BSSIDNodeId;BSSIDNodeX;BSSIDNodeY;BSSIDNodeZ;BSSIDNodeType;DestinationMac;DestinationNodeId;DestinationNodeX;DestinationNodeY;DestinationNodeZ;DestinationNodeType;SourceMac;SourceNodeId;SourceNodeX;SourceNodeY;SourceNodeZ;SourceNodeType;IsRetry;DropReason"}
};

PhySniffersRaw::PhySniffersRaw(const std::string& scenarioName, 
                           const NodeContainer& staNodes, 
                           const NodeContainer& apNodes)
    : m_scenarioName(scenarioName)
    , m_staNodes(staNodes)
    , m_apNodes(apNodes)
{   
    
}

void PhySniffersRaw::EnableLogging()
{
    // Create path with timestamp
    m_baseLogPath = "postprocessing/logs/" + m_scenarioName;

    // Create logging directory if it doesn't exist
    std::string cmd = "mkdir -p " + m_baseLogPath;
    int result = system(cmd.c_str());
    if (result != 0) {
        std::cout << "Failed to create logging directory: " << m_baseLogPath << std::endl;
    }
    // Open all required log files
    std::vector<std::string> eventTypes = {
        "MonitorSnifferRx", "MonitorSnifferTx", 
        "PhyTxBegin", "PhyTxEnd", "PhyTxDropWithReason",
        "PhyRxBegin", "PhyRxEnd", "PhyRxDropWithReason"
    };

    for (const auto& eventType : eventTypes) {
        std::string filePath = GetLogFilePath(eventType);
        m_logFiles[eventType].open(filePath, std::ios::out | std::ios::trunc);
        if (!m_logFiles[eventType].is_open()) {
            std::cout << "Failed to open log file: " << filePath << std::endl;
        }
    }

    // Write headers to all files after opening them
    WriteHeaders();

    // Connect all logging callbacks
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxBegin",
        MakeCallback(&PhySniffersRaw::PhyTxRxBeginEndCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxEnd",
        MakeCallback(&PhySniffersRaw::PhyTxRxBeginEndCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDropWithReason",
        MakeCallback(&PhySniffersRaw::PhyTxRxDropCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxBegin",
        MakeCallback(&PhySniffersRaw::PhyTxRxBeginEndCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxEnd",
        MakeCallback(&PhySniffersRaw::PhyTxRxBeginEndCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDropWithReason",
        MakeCallback(&PhySniffersRaw::PhyTxRxDropCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferTx",
        MakeCallback(&PhySniffersRaw::MonitorSnifferTxCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx",
        MakeCallback(&PhySniffersRaw::MonitorSnifferRxCallback, this));
}

std::string PhySniffersRaw::GetLogFilePath(const std::string& eventType) const
{
    return m_baseLogPath + "/" + eventType + ".csv";
}

uint32_t PhySniffersRaw::GetNodeIdFromMacAddress(const Mac48Address& addr) {
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

std::string PhySniffersRaw::GetNodePos(uint32_t nodeId) {
    // If nodeId is invalid, return placeholder values
    if (nodeId == (uint32_t)-1) {
        return "?;?;?;?";
    }

    // Find the node
    Ptr<Node> node = nullptr;
    for (uint32_t i = 0; i < m_staNodes.GetN(); i++) {
        if (m_staNodes.Get(i)->GetId() == nodeId) {
            node = m_staNodes.Get(i);
            break;
        }
    }
    
    if (!node) {
        for (uint32_t i = 0; i < m_apNodes.GetN(); i++) {
            if (m_apNodes.Get(i)->GetId() == nodeId) {
                node = m_apNodes.Get(i);
                break;
            }
        }
    }
    
    if (!node) {
        return std::to_string(nodeId) + "?;?;?;?";
    }

    // Get position information
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    Vector position = mobility ? mobility->GetPosition() : Vector(0, 0, 0);
    
    // Determine if node is AP or STA
    std::string nodeType = "STA";  // Default to STA
    for (uint32_t i = 0; i < node->GetNDevices(); ++i) {
        Ptr<WifiNetDevice> wifiDev = node->GetDevice(i)->GetObject<WifiNetDevice>();
        if (wifiDev) {
            Ptr<ApWifiMac> apMac = wifiDev->GetMac()->GetObject<ApWifiMac>();
            if (apMac) {
                nodeType = "AP";
                break;
            }
        }
    }
    
    std::stringstream ss;
    ss << position.x << ";"
       << position.y << ";"
       << position.z << ";"
       << nodeType;
    
    return ss.str();
}

MacAddresses PhySniffersRaw::ExtractMacAddresses(const WifiMacHeader& header) {
    MacAddresses macs;

    macs.receiver = header.GetAddr1();     // Always the receiver
    macs.transmitter = header.GetAddr2();  // Always the transmitter
    
	// Get MAC addresses based on ToDS and FromDS bits
    bool toDS = header.IsToDs();
    bool fromDS = header.IsFromDs();
    
    if (!toDS && !fromDS) {
        // Ad-hoc: addr3 is BSSID
        macs.destination = header.GetAddr1();
        macs.source = header.GetAddr2();
        macs.bssid = header.GetAddr3();
    }
    else if (!toDS && fromDS) {
        // From AP to STA
        macs.destination = header.GetAddr1();
        macs.bssid = header.GetAddr2();
        macs.source = header.GetAddr3();
    }
    else if (toDS && !fromDS) {
        // From STA to AP
        macs.bssid = header.GetAddr1();
        macs.source = header.GetAddr2();
        macs.destination = header.GetAddr3();
    }
    else {
        // WDS: addr4 exists
        macs.bssid = Mac48Address();
        macs.destination = header.GetAddr3();
        macs.source = header.GetAddr4();
    }
    
    return macs;
}

std::string PhySniffersRaw::PacketToCsv(Ptr<const Packet> packet){
	// Extract MAC header for source/destination info
	WifiMacHeader header;
	packet->PeekHeader(header);
	
	MacAddresses macs = ExtractMacAddresses(header);
	
	std::stringstream ss;

	ss << packet->GetUid() << ";"
	   << packet->GetSize() << ";" 
	   << macs.receiver << ";"
	   << (GetNodeIdFromMacAddress(macs.receiver) == ((uint32_t) -1) ? "?" : std::to_string(GetNodeIdFromMacAddress(macs.receiver))) << ";"
       << GetNodePos(GetNodeIdFromMacAddress(macs.receiver)) << ";"
	   << macs.transmitter << ";"
	   << (GetNodeIdFromMacAddress(macs.transmitter) == ((uint32_t) -1) ? "?" : std::to_string(GetNodeIdFromMacAddress(macs.transmitter))) << ";"
       << GetNodePos(GetNodeIdFromMacAddress(macs.transmitter)) << ";"
       << macs.bssid << ";"
	   << (GetNodeIdFromMacAddress(macs.bssid) == ((uint32_t) -1) ? "?" : std::to_string(GetNodeIdFromMacAddress(macs.bssid))) << ";"
       << GetNodePos(GetNodeIdFromMacAddress(macs.bssid)) << ";"
       << macs.destination << ";"
	   << (GetNodeIdFromMacAddress(macs.destination) == ((uint32_t) -1) ? "?" : std::to_string(GetNodeIdFromMacAddress(macs.destination))) << ";"
       << GetNodePos(GetNodeIdFromMacAddress(macs.destination)) << ";"
       << macs.source << ";"
	   << (GetNodeIdFromMacAddress(macs.source) == ((uint32_t) -1) ? "?" : std::to_string(GetNodeIdFromMacAddress(macs.source))) << ";"
       << GetNodePos(GetNodeIdFromMacAddress(macs.source)) << ";"
       << (header.IsRetry() ? "true" : "false");

	return ss.str();
}

void PhySniffersRaw::MonitorSnifferRxCallback(std::string context, Ptr<const Packet> packet, 
    uint16_t channelFreqMhz, uint16_t channelNumber, 
    uint32_t rate, bool isShortPreamble, 
    WifiTxVector txVector,
    double signalDbm, double noiseDbm)
{
	// Get interceptor node ID from context
	std::string::size_type pos = context.find("/NodeList/");
    std::string nodeStr = context.substr(pos);
    uint32_t snifferNodeId;
    sscanf(nodeStr.c_str(), "/NodeList/%u/", &snifferNodeId);

    // Extract MAC header for source/destination info
    WifiMacHeader header;
    packet->PeekHeader(header);

	// Check if sniffer node ID matches source or destination node ID
    MacAddresses macs = ExtractMacAddresses(header);
    if (snifferNodeId != GetNodeIdFromMacAddress(macs.destination) && snifferNodeId != GetNodeIdFromMacAddress(macs.source))
        return;

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
            << snifferNodeId << ";"
            << GetNodePos(snifferNodeId) << ";"
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

void PhySniffersRaw::MonitorSnifferTxCallback(std::string context, Ptr<const Packet> packet, 
    uint16_t channelFreqMhz, uint16_t channelNumber,
    uint32_t rate, bool isShortPreamble,
    WifiTxVector txVector)
{

    // Get transmission sniffer node ID from context
    std::string::size_type pos = context.find("/NodeList/");
    std::string nodeStr = context.substr(pos);
    uint32_t snifferNodeId;
    sscanf(nodeStr.c_str(), "/NodeList/%u/", &snifferNodeId);

    // Extract MAC header for source/destination info
    WifiMacHeader header;
    packet->PeekHeader(header);
    
    // Check if sniffer node ID matches source or destination node ID
    MacAddresses macs = ExtractMacAddresses(header);
    if (snifferNodeId != GetNodeIdFromMacAddress(macs.destination) && snifferNodeId != GetNodeIdFromMacAddress(macs.source))
        return;

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
            << snifferNodeId << ";"
            << GetNodePos(snifferNodeId) << ";"
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

void PhySniffersRaw::PhyTxRxBeginEndCallback(std::string context, Ptr<const Packet> packet){
    std::string::size_type traceSourcePos = context.find_last_of("/");
    std::string traceSource = context.substr(traceSourcePos + 1);

    // Get sniffer node ID from context
    std::string::size_type pos = context.find("/NodeList/");
    std::string nodeStr = context.substr(pos);
    uint32_t snifferNodeId;
    sscanf(nodeStr.c_str(), "/NodeList/%u/", &snifferNodeId);

    // Extract MAC header for source/destination info
    WifiMacHeader header;
    packet->PeekHeader(header);
    
    // Check if sniffer node ID matches source or destination node ID
    MacAddresses macs = ExtractMacAddresses(header);
    if (snifferNodeId != GetNodeIdFromMacAddress(macs.destination) && snifferNodeId != GetNodeIdFromMacAddress(macs.source))
        return;

    auto& logFile = m_logFiles[traceSource];
    if(!logFile.is_open()) {
        std::cout << "Log file for " << traceSource << " is not open" << std::endl;
        return;
    }

	logFile << Simulator::Now() << ";"
            << snifferNodeId << ";"
            << GetNodePos(snifferNodeId) << ";"
			<< PacketToCsv(packet) << std::endl;
}

void PhySniffersRaw::PhyTxRxDropCallback(std::string context, Ptr<const Packet> packet, DropReason reason){
    std::string::size_type traceSourcePos = context.find_last_of("/");
    std::string traceSource = context.substr(traceSourcePos + 1);

    // Get sniffer node ID from context
    std::string::size_type pos = context.find("/NodeList/");
    std::string nodeStr = context.substr(pos);
    uint32_t snifferNodeId;
    sscanf(nodeStr.c_str(), "/NodeList/%u/", &snifferNodeId);

    // Extract MAC header for source/destination info
    WifiMacHeader header;
    packet->PeekHeader(header);
    
    // Check if sniffer node ID matches source or destination node ID
    MacAddresses macs = ExtractMacAddresses(header);
    if (snifferNodeId != GetNodeIdFromMacAddress(macs.destination) && snifferNodeId != GetNodeIdFromMacAddress(macs.source))
        return;

    auto& logFile = m_logFiles[traceSource];
    if(!logFile.is_open()) {
        std::cout << "Log file for " << traceSource << " is not open" << std::endl;
        return;
    }

    std::string reasonString;
    switch (reason) {
        case DropReason::Unknown:
            reasonString = "Unknown";
            break;
        case DropReason::PhyInSleepMode:
            reasonString = "PhyInSleepMode";
            break;
        case DropReason::PhyNotEnoughSignalPower:
            reasonString = "PhyNotEnoughSignalPower";
            break;
        case DropReason::PhyUnsupportedMode:
            reasonString = "PhyUnsupportedMode";
            break;
        case DropReason::PhyPreampleHeaderReceptionFailed:
            reasonString = "PhyPreampleHeaderReceptionFailed";
            break;
        case DropReason::PhyRxDuringChannelSwitching:
            reasonString = "PhyRxDuringChannelSwitching";
            break;
        case DropReason::PhyAlreadyReceiving:
            reasonString = "PhyAlreadyReceiving";
            break;
        case DropReason::PhyAlreadyTransmitting:
            reasonString = "PhyAlreadyTransmitting";
            break;
        case DropReason::PhyPlcpReceptionFailed:
            reasonString = "PhyPlcpReceptionFailed";
            break;
        case DropReason::MacNotForAP:
            reasonString = "MacNotForAP";
            break;
        case DropReason::MacAPToAPFrame:
            reasonString = "MacAPToAPFrame";
            break;
        case DropReason::MacQueueDelayExceeded:
            reasonString = "MacQueueDelayExceeded";
            break;
        case DropReason::MacQueueSizeExceeded:
            reasonString = "MacQueueSizeExceeded";
            break;
        case DropReason::TCPTxBufferExceeded:
            reasonString = "TCPTxBufferExceeded";
            break;
        default:
            reasonString = "UnknownReason";
            break;
    }

    logFile << Simulator::Now() << ";"
            << snifferNodeId << ";"
            << GetNodePos(snifferNodeId) << ";"
            << PacketToCsv(packet) << ";"
            << reasonString << std::endl;
}

void PhySniffersRaw::WriteHeaders()
{
    for (auto it = m_logFiles.begin(); it != m_logFiles.end(); ++it) {
        const std::string& eventType = it->first;
        std::ofstream& logFile = it->second;
        if (logFile.is_open()) {
            logFile << CSV_HEADERS.at(eventType) << std::endl;
        }
    }
}

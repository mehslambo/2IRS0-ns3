/*
 * TCPSensorServer.cc
 *
 *  Created on: Aug 9, 2016
 *      Author: dwight
 */

#include "TCPSensorServer.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TCPSensorServer");
NS_OBJECT_ENSURE_REGISTERED(TCPSensorServer);


TCPSensorServer::TCPSensorServer() {

}

TCPSensorServer::~TCPSensorServer() {

}

ns3::TypeId TCPSensorServer::GetTypeId(void) {
	static ns3::TypeId tid = ns3::TypeId("TCPSensorServer")
			.SetParent<TcpServer>()
			.AddConstructor<TCPSensorServer>()
	;
	return tid;
}

void TCPSensorServer::OnDataReceived(ns3::Address from) {
    std::string msg;
    do {
        msg = ReadString(from, 8192);
        partialBytesReceived[from] += msg;
        
        // Process all complete messages (terminated by > bytes)
        size_t endPos;
        while ((endPos = partialBytesReceived[from].find('>')) != std::string::npos) {
            std::string completeMessage = partialBytesReceived[from].substr(0, endPos + 1);
            
            // Find and exclude start bytes until '<'
            size_t startPos = completeMessage.find('<');
            if (startPos != std::string::npos) {
                completeMessage = completeMessage.substr(startPos);
            }
            
            std::cout << "Received from client " << from << " following msg: " << completeMessage << std::endl;
            
            // Remove processed message and its terminator
            partialBytesReceived[from] = partialBytesReceived[from].substr(endPos + 1);
        }
    } while (msg != "");
}



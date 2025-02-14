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
	std::cout << "[";
      std::cout << Simulator::Now().GetMilliSeconds();
      std::cout << "] " ;
      std::cout << from;
      std::cout << " Received" << std::endl;


	std::string msg;
	do {
		 msg = ReadString(from, 4096);
	if(partialBytesReceived.find(from) == partialBytesReceived.end())
		partialBytesReceived[from] = "";
	else
		partialBytesReceived[from] += msg;
	} while(msg != "");

	NS_LOG_INFO("received from client " << from << ", current size: " << partialBytesReceived[from].size());
}



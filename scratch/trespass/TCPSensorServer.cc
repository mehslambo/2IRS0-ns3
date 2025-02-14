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
		 msg = ReadString(from, 4096);
		 if(partialBytesReceived.find(from) == partialBytesReceived.end())
		   partialBytesReceived[from] = "";
		 partialBytesReceived[from] += msg;
	} while(msg != "");

	size_t st = partialBytesReceived[from].size();
	NS_LOG_INFO("received from client " << from << ", current size: " << st);
	std::cout << "[";
	std::cout << Simulator::Now().GetMilliSeconds();
	std::cout << "] " ;
	std::cout << from;
	std::cout << " Received " << st << std::endl;
	

	//if (partialBytesReceived[from].size()>33600) {
	//  std::cout << "Payload: " << partialBytesReceived[from] << std::endl;
	//}
}


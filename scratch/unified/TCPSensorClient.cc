/*
 * TCPSensorClient.cc
 *
 *  Created on: Aug 9, 2016
 *      Author: dwight
 */

#include "TCPSensorClient.h"

using namespace ns3;


NS_LOG_COMPONENT_DEFINE("TCPSensorClient");
NS_OBJECT_ENSURE_REGISTERED(TCPSensorClient);


TCPSensorClient::TCPSensorClient() {

}

TCPSensorClient::~TCPSensorClient() {

}

ns3::TypeId TCPSensorClient::GetTypeId(void) {
	static ns3::TypeId tid = ns3::TypeId("TCPSensorClient")
			.SetParent<TcpClient>()
			.AddConstructor<TCPSensorClient>()

			.AddAttribute ("Interval",
			                   "The time to wait between packets",
			                   TimeValue (Seconds (1000)),
			                   MakeTimeAccessor (&TCPSensorClient::m_interval),
			                   MakeTimeChecker ())

		   .AddAttribute ("MeasurementSize",
						   "The size of a measurement",
						   UintegerValue(10),
						   MakeUintegerAccessor(&TCPSensorClient::measurementSize),
						   MakeUintegerChecker<uint32_t>())
			.AddAttribute ("id",
						   "id",
						   UintegerValue(0),
						   MakeUintegerAccessor(&TCPSensorClient::id),
						   MakeUintegerChecker<uint16_t>())

	;
	return tid;
}


void TCPSensorClient::StartApplication(void) {
	ns3::TcpClient::StartApplication();

#ifdef TRESPASS
	if (id % 4 != 3)
    {
      actionEvent = ns3::Simulator::Schedule(m_interval, &TCPSensorClient::Action, this);
    }
#else
	actionEvent = ns3::Simulator::Schedule(m_interval, &TCPSensorClient::Action, this);
#endif
}




void TCPSensorClient::StopApplication(void) {
	ns3::TcpClient::StopApplication();
	ns3::Simulator::Cancel(actionEvent);
} 





void TCPSensorClient::Action() {
		std::cout << "[";
      std::cout << Simulator::Now().GetMilliSeconds();
      std::cout << "] " ;
	  std::cout << id;
      std::cout << " Sending" << std::endl;

	NS_LOG_INFO("Sending measurement to server");

	char* buf = new char[measurementSize];
	for(int i = 0; i < measurementSize; i++)
		buf[i] = 'A' + (i % 20);//(i % 255)+1;
	Write(buf, measurementSize);
	delete buf;

	Flush();
}




void TCPSensorClient::OnDataReceived() {

	std::string reply = ReadString(4096);
	std::cout << "Reply from TCP Server: '" << reply << "'" << std::endl;
}

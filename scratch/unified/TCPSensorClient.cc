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
							TimeValue (Seconds (1.0)),
							MakeTimeAccessor (&TCPSensorClient::m_interval),
							MakeTimeChecker ())
 
			.AddAttribute ("MeasurementSize",
							"The size of a measurement",
							UintegerValue(10),
							MakeUintegerAccessor(&TCPSensorClient::measurementSize),
							MakeUintegerChecker<uint16_t>())
 
	 ;
	 return tid;
 }
 
 void TCPSensorClient::StartApplication(void) {
	 ns3::TcpClient::StartApplication();
 
	 actionEvent = ns3::Simulator::Schedule(m_interval, &TCPSensorClient::Action, this);
 }
 
 void TCPSensorClient::StopApplication(void) {
	 ns3::TcpClient::StopApplication();
	 ns3::Simulator::Cancel(actionEvent);
 }
 
 void TCPSensorClient::Action() {
    NS_LOG_INFO("Sending measurement of exact size " << measurementSize);
    
    // Create a buffer of exactly measurementSize bytes
    char* buf = new char[measurementSize];
    
    // Fill the buffer with random data
    for(int i = 0; i < measurementSize; i++)
        buf[i] = 'A' + (i % 26); // Use letters A-Z
    
    // Write exactly measurementSize bytes
    Write(buf, measurementSize);
    delete buf;
    
    // Force the packet to be sent immediately
    Flush();
    
    // Schedule next transmission
    actionEvent = Simulator::Schedule(MilliSeconds(20), &TCPSensorClient::Action, this);
}
 
 
 void TCPSensorClient::OnDataReceived() {
 
	 std::string reply = ReadString(4096);
	 std::cout << "Reply from TCP Server: '" << reply << "'" << std::endl;
 }
 
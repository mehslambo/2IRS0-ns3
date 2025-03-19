/*
 * TCPSensorClient.cc
 *
 *  Created on: March 15, 2025
 *  	Author: m.t.visoiu@student.tue.nl
 */

 #include "TCPSensorClient.h"
 #include <sstream>
 
 using namespace ns3;
 
 NS_LOG_COMPONENT_DEFINE("TCPSensorClient");
 NS_OBJECT_ENSURE_REGISTERED(TCPSensorClient);
 
 TCPSensorClient::TCPSensorClient() : m_connected(false) {
 }
 
 TCPSensorClient::~TCPSensorClient() {
 }
 
 ns3::TypeId TCPSensorClient::GetTypeId(void) {
	 static ns3::TypeId tid = ns3::TypeId("TCPSensorClient")
			 .SetParent<TcpClient>()
			 .AddConstructor<TCPSensorClient>()
			 .AddAttribute ("Interval",
							"The time between sensor measurements",
							TimeValue (Seconds (1.0)),
							MakeTimeAccessor (&TCPSensorClient::m_interval),
							MakeTimeChecker ());
	 return tid;
 }
 
 void TCPSensorClient::StartApplication(void) {
	 ns3::TcpClient::StartApplication();
 
	 // Register connection callbacks to update our m_connected flag
	 if (m_socket != 0) {
		 // When the connection is successfully established, ConnectionSucceeded is called.
		 m_socket->SetConnectCallback(MakeCallback(&TCPSensorClient::ConnectionSucceeded, this),
									  MakeNullCallback<void, Ptr<Socket>>());
		 // When the connection is closed, ConnectionClosed is called.
		 m_socket->SetCloseCallbacks(MakeCallback(&TCPSensorClient::ConnectionClosed, this),
									MakeCallback(&TCPSensorClient::ConnectionClosed, this));
	 }
 
	 actionEvent = ns3::Simulator::Schedule(m_interval, &TCPSensorClient::Action, this);
 }
 
 void TCPSensorClient::StopApplication(void) {
	 ns3::TcpClient::StopApplication();
	 ns3::Simulator::Cancel(actionEvent);
 }
 
 void TCPSensorClient::ConnectionSucceeded(Ptr<Socket> socket) {
	 NS_LOG_INFO("TCP connection established.");
	 m_connected = true;
	 // Once connected, try flushing any queued messages.
	 FlushQueuedMessages();
 }
 
 void TCPSensorClient::ConnectionClosed(Ptr<Socket> socket) {
	 NS_LOG_INFO("TCP connection closed.");
	 m_connected = false;
 }
 
 bool TCPSensorClient::IsConnected() const {
	 return m_connected;
 }
 
 void TCPSensorClient::FlushQueuedMessages() {
	 while (!m_messageQueue.empty() && IsConnected()) {
		 std::string queuedMessage = m_messageQueue.front();
		 Write((char*)queuedMessage.c_str(), queuedMessage.length());
		 Flush();
		 m_messageQueue.pop();
		 NS_LOG_INFO("Flushed a queued message.");
	 }
 }
 
 void TCPSensorClient::Action() {
	 NS_LOG_INFO("Sending measurement");
 
	 // Create a message string with the current simulation time
	 uint64_t currentTimeNs = Simulator::Now().GetNanoSeconds();
	 std::stringstream ss;
	 ss << "SENSOR TIME: " << currentTimeNs << "ns";
	 std::string message = ss.str();
 
	 // Add null terminator to the message
	 message += '\0';
 
	 if (IsConnected()) {
		 FlushQueuedMessages();
		 Write((char*)message.c_str(), message.length());
		 Flush();
		 NS_LOG_INFO("Message sent directly.");
		 std::cout << "Sent message directly: '" << message << "'" << std::endl;
	 } else {
		 m_messageQueue.push(message);
		 NS_LOG_INFO("TCP not connected. Message queued.");
		 std::cout << "Queued message: '" << message << "'" << std::endl;
	 }
 
	 // Schedule the next measurement action
	 actionEvent = Simulator::Schedule(m_interval, &TCPSensorClient::Action, this);
 }
 
 void TCPSensorClient::OnDataReceived() {
	 std::string reply = ReadString(4096);
	 std::cout << "Reply from TCP Server: '" << reply << "'" << std::endl;
 }
 
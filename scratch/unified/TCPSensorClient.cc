/*
 * TCPSensorClient.cc
 *
 *  Created on: March 15, 2025
 *  	Author: m.t.visoiu@student.tue.nl
 */

#include "TCPSensorClient.h"
#include <cstring>
#include <sstream>
#include <queue>
#include "ns3/simulator.h"
#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TCPSensorClient");
NS_OBJECT_ENSURE_REGISTERED(TCPSensorClient);

bool IsAssoc(uint16_t staid);

TCPSensorClient::TCPSensorClient() {
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
                   MakeTimeChecker ())
    // New: station id attribute
    .AddAttribute ("id",
                   "Station ID of the TCP Sensor Client",
                   UintegerValue(0),
                   MakeUintegerAccessor(&TCPSensorClient::m_id),
                   MakeUintegerChecker<uint32_t>())
    ;
  return tid;
}

void TCPSensorClient::StartApplication(void) {
  TcpClient::StartApplication();
  actionEvent = Simulator::Schedule(m_interval, &TCPSensorClient::Action, this);
}

void TCPSensorClient::StopApplication(void) {
  TcpClient::StopApplication();
  Simulator::Cancel(actionEvent);
}

void TCPSensorClient::Action() {
  NS_LOG_INFO("Sending measurement");

  // Create a string with the current time
  std::stringstream ss;
  ss << "<" << "Client " << m_id <<" @ " << Simulator::Now().GetNanoSeconds() << "ns has TEMP=FAKEDATA*C>";
  std::string message = ss.str();

  // Check if this node is associated using its id (global IsAssoc function)
  if (IsAssoc(m_id)) {
    // If associated, first send any queued messages
    while (!m_queue.empty()) {
		std::string queuedMsg = m_queue.front();
		std::cout<<"Client " << m_id << " sending QUEUED message: " << queuedMsg << std::endl;
		Write((char*)queuedMsg.c_str(), queuedMsg.length());
		Flush();
		m_queue.pop();
    }
	// Then send the current message
	std::cout<<"Client " << m_id << " sending REALTIME message: " << message << std::endl;
    Write((char*)message.c_str(), message.length());
    Flush();
  } else {
    // Otherwise, queue the message for later transmission
    m_queue.push(message);
    NS_LOG_INFO("Client " << m_id << " not associated; message QUEUED");
	std::cout << "Client " << m_id << " not associated; message QUEUED: " << message << std::endl;
  }

  // Schedule the next measurement
  actionEvent = Simulator::Schedule(m_interval, &TCPSensorClient::Action, this);
}

void TCPSensorClient::OnDataReceived() {
  std::string reply = ReadString(4096);
  std::cout << "Reply from TCP Server: '" << reply << "'" << std::endl;
}

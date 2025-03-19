#ifndef SCRATCH_APPLICATIONS_PINGPONG_TCPSensorClient_H_
#define SCRATCH_APPLICATIONS_PINGPONG_TCPSensorClient_H_

#include "ns3/tcp-client.h"
#include "ns3/application.h"
#include "ns3/core-module.h"
#include <queue>
#include <string>

class TCPSensorClient : public ns3::TcpClient {
public:
	TCPSensorClient();
	virtual ~TCPSensorClient();

	static ns3::TypeId GetTypeId (void);

protected:
	virtual void StartApplication(void);
	virtual void StopApplication(void);
	virtual void OnDataReceived();

private:
	void Action();
	void FlushQueuedMessages();
	bool IsConnected() const;

	bool m_connected;
	void ConnectionSucceeded(ns3::Ptr<ns3::Socket> socket);
	void ConnectionClosed(ns3::Ptr<ns3::Socket> socket);

	ns3::Time m_interval;
	ns3::EventId actionEvent;

	std::queue<std::string> m_messageQueue;
};

#endif /* SCRATCH_APPLICATIONS_PINGPONG_TCPSensorClient_H_ */

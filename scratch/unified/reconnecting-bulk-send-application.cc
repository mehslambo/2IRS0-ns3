#include "reconnecting-bulk-send-application.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ReconnectingBulkSendApplication");

TypeId
ReconnectingBulkSendApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ReconnectingBulkSendApplication")
    .SetParent<BulkSendApplication> ()
    .AddConstructor<ReconnectingBulkSendApplication> ();
  return tid;
}

ReconnectingBulkSendApplication::ReconnectingBulkSendApplication ()
{
  NS_LOG_FUNCTION (this);
}

ReconnectingBulkSendApplication::~ReconnectingBulkSendApplication ()
{
  NS_LOG_FUNCTION (this);
}

void
ReconnectingBulkSendApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_INFO ("Connection failed. Scheduling reconnect in 1 second.");
  // Schedule a reconnect attempt after a 1-second delay.
  Simulator::Schedule (Seconds (1.0), &ReconnectingBulkSendApplication::Reconnect, this);
}

void
ReconnectingBulkSendApplication::Reconnect (void)
{
  NS_LOG_FUNCTION (this);
  // Clean up any existing socket.
  if (m_socket != 0)
  {
    m_socket->Close ();
    m_socket = 0;
  }
  // Create a new socket with the same type.
  m_socket = Socket::CreateSocket (GetNode (), m_tid);
  if (!m_socket)
  {
    NS_FATAL_ERROR ("Failed to create socket during reconnect");
  }
  // Bind the new socket according to the type of remote address.
  if (Inet6SocketAddress::IsMatchingType (m_peer))
  {
    if (m_socket->Bind6 () == -1)
    {
      NS_FATAL_ERROR ("Failed to bind IPv6 socket during reconnect");
    }
  }
  else if (InetSocketAddress::IsMatchingType (m_peer))
  {
    if (m_socket->Bind () == -1)
    {
      NS_FATAL_ERROR ("Failed to bind IPv4 socket during reconnect");
    }
  }
  // Connect to the peer.
  m_socket->Connect (m_peer);
  // Since BulkSend only sends data, shut down the receive side.
  m_socket->ShutdownRecv ();
  // Re-bind the connection and send callbacks.
  m_socket->SetConnectCallback (
    MakeCallback (&BulkSendApplication::ConnectionSucceeded, this),
    MakeCallback (&ReconnectingBulkSendApplication::ConnectionFailed, this));
  m_socket->SetSendCallback (
    MakeCallback (&BulkSendApplication::DataSend, this));
  NS_LOG_INFO ("Reconnect attempt initiated.");
}

} // namespace ns3

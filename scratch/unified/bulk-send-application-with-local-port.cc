#include "bulk-send-application-with-local-port.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/internet-module.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BulkSendApplicationWithLocalPort");
NS_OBJECT_ENSURE_REGISTERED (BulkSendApplicationWithLocalPort);

TypeId
BulkSendApplicationWithLocalPort::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BulkSendApplicationWithLocalPort")
    .SetParent<BulkSendApplication> ()
    .SetGroupName("Applications")
    .AddConstructor<BulkSendApplicationWithLocalPort> ()
    .AddAttribute ("LocalPort",
                   "The custom local port number the application will bind to. "
                   "A value of 0 uses the default binding behavior.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&BulkSendApplicationWithLocalPort::m_localPort),
                   MakeUintegerChecker<uint16_t> (0, 65535));
  return tid;
}

BulkSendApplicationWithLocalPort::BulkSendApplicationWithLocalPort ()
  : BulkSendApplication (),
    m_localPort (0)
{
  NS_LOG_FUNCTION (this);
}

BulkSendApplicationWithLocalPort::~BulkSendApplicationWithLocalPort ()
{
  NS_LOG_FUNCTION (this);
}

void
BulkSendApplicationWithLocalPort::SetLocalPort (uint16_t localPort)
{
  NS_LOG_FUNCTION (this << localPort);
  m_localPort = localPort;
}

uint16_t
BulkSendApplicationWithLocalPort::GetLocalPort (void) const
{
  NS_LOG_FUNCTION (this);
  return m_localPort;
}

void
BulkSendApplicationWithLocalPort::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  // Only create the socket if it hasn't been created already.
  if (!GetSocket ())
    {
      // Create the socket using the protocol type inherited from the base class.
      Ptr<Socket> socket = Socket::CreateSocket (GetNode (), m_tid);

      // Bind the socket using a custom local port if provided.
      if (m_localPort != 0)
        {
          // For IPv4 addresses.
          if (InetSocketAddress::IsMatchingType (m_peer))
            {
              InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_localPort);
              socket->Bind (local);
              NS_LOG_INFO ("BulkSendApplicationWithLocalPort bound to local IPv4 port " << m_localPort);
            }
          else
            {
              NS_FATAL_ERROR ("Unsupported address type for binding custom local port");
            }
        }
      else
        {
          // If no custom local port is specified, use the default binding behavior.
          if (Inet6SocketAddress::IsMatchingType (m_peer))
            {
              socket->Bind6 ();
            }
          else if (InetSocketAddress::IsMatchingType (m_peer))
            {
              socket->Bind ();
            }
        }

      // Connect to the remote peer (set via the "Remote" attribute).
      socket->Connect (m_peer);
      socket->ShutdownRecv ();
      socket->SetConnectCallback (
        MakeCallback (&BulkSendApplication::ConnectionSucceeded, this),
        MakeCallback (&BulkSendApplication::ConnectionFailed, this));
      socket->SetSendCallback (
        MakeCallback (&BulkSendApplication::DataSend, this));

      // Save the socket in the base class.
      BulkSendApplication::m_socket = socket;
    }

  // If the connection is already established, start sending data.
  if (BulkSendApplication::m_connected)
    {
      SendData ();
    }
}

} // namespace ns3
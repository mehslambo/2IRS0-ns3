#ifndef RECONNECTING_BULK_SEND_APPLICATION_H
#define RECONNECTING_BULK_SEND_APPLICATION_H

#include "ns3/bulk-send-application.h"

namespace ns3 {

class ReconnectingBulkSendApplication : public BulkSendApplication
{
public:
  static TypeId GetTypeId (void);
  ReconnectingBulkSendApplication ();
  virtual ~ReconnectingBulkSendApplication ();

//protected:
  // Override the connection failure callback from BulkSendApplication.
  void ConnectionFailed (Ptr<Socket> socket);

//private:
  // Helper function to create a new socket and re-bind callbacks.
  void Reconnect (void);
};

} // namespace ns3

#endif // RECONNECTING_BULK_SEND_APPLICATION_H

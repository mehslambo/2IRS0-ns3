#ifndef BULK_SEND_APPLICATION_WITH_LOCAL_PORT_H
#define BULK_SEND_APPLICATION_WITH_LOCAL_PORT_H

#include "ns3/bulk-send-application.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"

namespace ns3 {

class BulkSendApplicationWithLocalPort : public BulkSendApplication
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  BulkSendApplicationWithLocalPort ();
  virtual ~BulkSendApplicationWithLocalPort ();

  /**
   * \brief Set the local port for binding.
   * \param localPort the local port number to bind
   */
  void SetLocalPort (uint16_t localPort);

  /**
   * \brief Get the local port used for binding.
   * \return local port number
   */
  uint16_t GetLocalPort (void) const;

protected:
  /**
   * \brief Start the application. Overrides the base class to allow custom local port binding.
   */
  virtual void StartApplication (void);

private:
  uint16_t m_localPort; //!< Custom local port, zero means default behavior.
};

} // namespace ns3

#endif /* BULK_SEND_APPLICATION_WITH_LOCAL_PORT_H */
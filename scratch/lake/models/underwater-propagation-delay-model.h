#ifndef UNDERWATER_PROPAGATION_DELAY_MODEL_H
#define UNDERWATER_PROPAGATION_DELAY_MODEL_H

#include "ns3/propagation-delay-model.h"
#include "ns3/vector.h"

namespace ns3 {

class UnderwaterPropagationDelayModel : public PropagationDelayModel
{
public:
  static TypeId GetTypeId (void);
  UnderwaterPropagationDelayModel ();
  virtual ~UnderwaterPropagationDelayModel ();

  /**
   * \brief Get the propagation delay (in seconds) between two mobility models.
   */
  virtual ns3::Time GetDelay (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;
private:
  // Frequency in Hz.
  double m_frequency;
  // Water temperature in Celsius.
  double m_temperature;
  // Salinity in parts per thousand (ppt).
  double m_salinity;
  // Relative permittivity (dimensionless).
  double m_relativePermittivity;

  virtual int64_t DoAssignStreams (int64_t stream);
};

} // namespace ns3

#endif /* UNDERWATER_PROPAGATION_DELAY_MODEL_H */

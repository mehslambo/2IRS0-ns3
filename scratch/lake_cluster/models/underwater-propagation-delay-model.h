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
  // The frequency (in Hz) at which the model is applied
  double m_frequency;
  // The temperature of water at a specific place
  double m_temp;
  // The salinity of water at a specific place
  double m_S;
  // The real relative permittivity of the water at high frequencies
  double m_epsilon_inf;

  virtual int64_t DoAssignStreams (int64_t stream);
};

} // namespace ns3

#endif /* UNDERWATER_PROPAGATION_DELAY_MODEL_H */

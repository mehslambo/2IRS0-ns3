#ifndef UNDERWATER_PROPAGATION_LOSS_MODEL_H
#define UNDERWATER_PROPAGATION_LOSS_MODEL_H

#include "ns3/propagation-loss-model.h"
#include "ns3/vector.h"

namespace ns3 {

class UnderwaterPropagationLossModel : public PropagationLossModel
{
public:
  static TypeId GetTypeId (void);
  UnderwaterPropagationLossModel ();
  virtual ~UnderwaterPropagationLossModel ();

  /**
   * \brief Calculate the received power (in dBm) given the transmitter power.
   */
  virtual double DoCalcRxPower (double txPowerDbm,
                                Ptr<MobilityModel> a,
                                Ptr<MobilityModel> b) const;
protected:
  /**
   * \brief Compute the loss (in dB) between two mobility models.
   */
  virtual double DoCalcLoss (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;
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

#endif /* UNDERWATER_PROPAGATION_LOSS_MODEL_H */

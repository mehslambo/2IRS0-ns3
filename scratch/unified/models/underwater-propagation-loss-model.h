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
  // The conductivity (in S/m) of the water
  double m_sigma;
  // The real relative permittivity of the water at low frequencies
  // Aka static permittivity
  double m_epsilon_s;
  // The real relative permittivity of the water at high frequencies
  // Aka high frequency permittivity
  double m_epsilon_inf;
  // The relaxation frequency of the water
  double m_f_ref;

  virtual int64_t DoAssignStreams (int64_t stream);
};

} // namespace ns3

#endif /* UNDERWATER_PROPAGATION_LOSS_MODEL_H */

#include "underwater-propagation-loss-model.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/attribute.h"
#include "ns3/object.h"
#include "ns3/mobility-model.h"
#include <cmath>
#include <complex>

namespace ns3 {

//NS_LOG_COMPONENT_DEFINE ("UnderwaterPropagationLossModel");
NS_OBJECT_ENSURE_REGISTERED (UnderwaterPropagationLossModel);

TypeId
UnderwaterPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UnderwaterPropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<UnderwaterPropagationLossModel> ()
    .AddAttribute ("Frequency",
      "The frequency (in Hz) at which the model is applied.",
      DoubleValue (900e6),
      MakeDoubleAccessor (&UnderwaterPropagationLossModel::m_frequency),
      MakeDoubleChecker<double> (0))
.AddAttribute ("Temperature",
      "The temperature of water at a specific place (in Celsius). Default: 20.",
      DoubleValue (20),
      MakeDoubleAccessor (&UnderwaterPropagationLossModel::m_temp),
      MakeDoubleChecker<double> (0))
.AddAttribute ("Salinity",
      "The salinity of water at a specific place (in PSU). Default: 35.",
      DoubleValue (0.5),
      MakeDoubleAccessor (&UnderwaterPropagationLossModel::m_S),
      MakeDoubleChecker<double> (0))
.AddAttribute ("HighFrequencyPermittivity",
      "The real relative permittivity of the water at high frequencies. Default: 4.9.",
      DoubleValue (4.9),
      MakeDoubleAccessor (&UnderwaterPropagationLossModel::m_epsilon_inf),
      MakeDoubleChecker<double> (0));
  return tid;
}

UnderwaterPropagationLossModel::UnderwaterPropagationLossModel ()
{
  //NS_LOG_FUNCTION (this);
  std::cout<<"UnderwaterPropagationLossModel initialized"<<std::endl;
}

UnderwaterPropagationLossModel::~UnderwaterPropagationLossModel ()
{
}

double
UnderwaterPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                                          Ptr<MobilityModel> a,
                                                          Ptr<MobilityModel> b) const
{
  double lossDb = DoCalcLoss (a, b);
  // Received power is transmitter power minus the computed loss (in dB)
  double rxPowerDbm = txPowerDbm - lossDb;
  return rxPowerDbm;
}

double
UnderwaterPropagationLossModel::DoCalcLoss (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  Vector posA = a->GetPosition ();
  Vector posB = b->GetPosition ();
  double dx = posA.x - posB.x;
  double dy = posA.y - posB.y;
  double dz = posA.z - posB.z;
  double D  = std::sqrt (dx * dx + dy * dy + dz * dz);

  double pi = std::acos(-1.0);
  double mu0 = 4.0 * pi * 1e-7;
  double epsilon_0 = 8.85e-12;
  double c = 2.998e8;
  double omega = 2.0 * pi * m_frequency;

  // Fixed values
  double T = 20.0;  // Temperature in Celsius
  double S_sw = 35.0;  // Salinity in PSU
  const double m_epsilon_inf = 4.9;

  // Static Permittivity (epsilon_s(T, S_sw))
  double m_epsilon_s = (87.134 - 1.949e-1 * T - 1.276e-2 * T * T + 2.491e-4 * T * T * T) 
                     * (1 + 1.613e-5 * T * S_sw - 3.656e-3 * S_sw 
                        + 3.21e-5 * S_sw * S_sw - 4.232e-7 * S_sw * S_sw * S_sw);

  // Relaxation Time (tau_sw(T, S_sw))
  double tau_sw_0 = 1 / (2 * pi) * (1.11e-10 - 3.824e-12 * T + 6.938e-14 * T * T - 5.096e-16 * T * T * T);
  double B = 1 + 2.282e-2 * T * S_sw - 7.638e-4 * S_sw 
             - 7.76e-6 * S_sw * S_sw + 1.105e-8 * S_sw * S_sw * S_sw;
  double tau_sw = tau_sw_0 * B;

  // Conductivity (sigma(T, S))
  double delta = 25 - T;
  double sigma_25_S = S_sw * (0.182521 - 1.46192e-3 * S_sw + 2.09324e-5 * S_sw * S_sw 
                             - 1.82025e-7 * S_sw * S_sw * S_sw);
  double phi = 0.02033 + 1.266e-4 * delta + 2.464e-4 * delta * delta
               - S_sw * (1.849e-5 - 2.551e-7 * delta + 2.551e-8 * delta * delta);
  double m_sigma = sigma_25_S * exp(-phi);

  // Relaxation Frequency (f_rel)
  double m_f_ref = 1 / (2 * pi * tau_sw);  // f_rel = 1 / (2 * pi * tau_sw)

  std::complex<double> j (0.0, 1.0);
  std::complex<double> denom = 1.0 + j * (m_frequency / m_f_ref);
  std::complex<double> epsilon_r = (m_epsilon_inf + ((m_epsilon_s - m_epsilon_inf) / denom))- ((j * m_sigma) / (2 * pi * m_frequency * epsilon_0));
  std::complex<double> epsilon = epsilon_0 * epsilon_r;

  std::complex<double> gamma = std::sqrt (j * omega * mu0 * (m_sigma + j * omega * epsilon));
  double alpha = std::real (gamma);

  double absorptionLoss = 20.0 * std::log10 (std::exp (alpha * D));

  double pathLoss = absorptionLoss;
  return pathLoss;
}

int64_t
UnderwaterPropagationLossModel::DoAssignStreams (int64_t stream)
{
  // This model doesn't use random variables, so return 0
  return 0;
}

} // namespace ns3

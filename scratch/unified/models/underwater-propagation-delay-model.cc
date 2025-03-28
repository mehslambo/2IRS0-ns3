#include "underwater-propagation-delay-model.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/attribute.h"
#include "ns3/object.h"
#include "ns3/mobility-model.h"
#include "ns3/simulator.h"
#include <cmath>
#include <complex>

namespace ns3 {

//NS_LOG_COMPONENT_DEFINE ("UnderwaterPropagationDelayModel");
NS_OBJECT_ENSURE_REGISTERED (UnderwaterPropagationDelayModel);

TypeId
UnderwaterPropagationDelayModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UnderwaterPropagationDelayModel")
    .SetParent<PropagationDelayModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<UnderwaterPropagationDelayModel> ()
    .AddAttribute ("Frequency",
          "The frequency (in Hz) at which the model is applied.",
          DoubleValue (900e6),
          MakeDoubleAccessor (&UnderwaterPropagationDelayModel::m_frequency),
          MakeDoubleChecker<double> (0))
    .AddAttribute ("Temperature",
          "The temperature of water at a specific place (in Celsius). Default: 20.",
          DoubleValue (20),
          MakeDoubleAccessor (&UnderwaterPropagationDelayModel::m_temp),
          MakeDoubleChecker<double> (0))
    .AddAttribute ("Salinity",
          "The salinity of water at a specific place (in PSU). Default: 35.",
          DoubleValue (0.5),
          MakeDoubleAccessor (&UnderwaterPropagationDelayModel::m_S),
          MakeDoubleChecker<double> (0))
    .AddAttribute ("HighFrequencyPermittivity",
          "The real relative permittivity of the water at high frequencies. Default: 4.9.",
          DoubleValue (4.9),
          MakeDoubleAccessor (&UnderwaterPropagationDelayModel::m_epsilon_inf),
          MakeDoubleChecker<double> (0));
  return tid;
}

UnderwaterPropagationDelayModel::UnderwaterPropagationDelayModel ()
{
  //NS_LOG_FUNCTION (this);
  std::cout<<"UnderwaterPropagationDelayModel initialized"<<std::endl;
}

UnderwaterPropagationDelayModel::~UnderwaterPropagationDelayModel ()
{
}

Time
UnderwaterPropagationDelayModel::GetDelay (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
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
  double omega = 2.0 * pi * m_frequency;

  // Static Permittivity (epsilon_s(m_temp, m_S))
  double m_epsilon_s = (87.134 - 1.949e-1 * m_temp - 1.276e-2 * m_temp * m_temp + 2.491e-4 * m_temp * m_temp * m_temp) 
                     * (1 + 1.613e-5 * m_temp * m_S - 3.656e-3 * m_S 
                        + 3.21e-5 * m_S * m_S - 4.232e-7 * m_S * m_S * m_S);

  // Relaxation Time (tau_sw(m_temp, m_S))
  double tau_sw_0 = 1 / (2 * pi) * (1.11e-10 - 3.824e-12 * m_temp + 6.938e-14 * m_temp * m_temp - 5.096e-16 * m_temp * m_temp * m_temp);
  double B = 1 + 2.282e-2 * m_temp * m_S - 7.638e-4 * m_S 
             - 7.76e-6 * m_S * m_S + 1.105e-8 * m_S * m_S * m_S;
  double tau_sw = tau_sw_0 * B;

  // Conductivity (sigma(m_temp, S))
  double delta = 25 - m_temp;
  double sigma_25_S = m_S * (0.182521 - 1.46192e-3 * m_S + 2.09324e-5 * m_S * m_S 
                             - 1.82025e-7 * m_S * m_S * m_S);
  double phi = 0.02033 + 1.266e-4 * delta + 2.464e-4 * delta * delta
               - m_S * (1.849e-5 - 2.551e-7 * delta + 2.551e-8 * delta * delta);
  double m_sigma = sigma_25_S * exp(-phi);

  // Relaxation Frequency (f_ref)
  double m_f_ref = 1 / (2 * pi * tau_sw);  

  std::complex<double> j (0.0, 1.0);
  std::complex<double> denom = 1.0 + j * (m_frequency / m_f_ref);
  std::complex<double> epsilon_r = (m_epsilon_inf + ((m_epsilon_s - m_epsilon_inf) / denom))- ((j * m_sigma) / (2 * pi * m_frequency * epsilon_0));
  std::complex<double> epsilon = epsilon_0 * epsilon_r;

  std::complex<double> gamma = std::sqrt (j * omega * mu0 * (m_sigma + j * omega * epsilon));
  double beta = std::imag (gamma);

  double delaySeconds = D / (omega/beta);

  return Seconds (delaySeconds);
}

int64_t
UnderwaterPropagationDelayModel::DoAssignStreams (int64_t stream)
{
  // This model doesn't use random variables, so return 0
  return 0;
}

} // namespace ns3

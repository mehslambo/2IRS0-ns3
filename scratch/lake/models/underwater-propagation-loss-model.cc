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
    .AddAttribute ("Conductivity",
          "The conductivity (in S/m) of the water. Default: 0.01 S/m (tap water).",
          DoubleValue (0.01),
          MakeDoubleAccessor (&UnderwaterPropagationLossModel::m_sigma),
          MakeDoubleChecker<double> (0))
    .AddAttribute ("StaticPermittivity",
          "The real relative permittivity of the water at low frequencies. Default: 80 (tap water).",
          DoubleValue (80),
          MakeDoubleAccessor (&UnderwaterPropagationLossModel::m_epsilon_s),
          MakeDoubleChecker<double> (0))
    .AddAttribute ("HighFrequencyPermittivity",
          "The real relative permittivity of the water at high frequencies. Default: 4.22 (tap water).",
          DoubleValue (4.22),
          MakeDoubleAccessor (&UnderwaterPropagationLossModel::m_epsilon_inf),
          MakeDoubleChecker<double> (0))
    .AddAttribute ("RelaxationFrequency",
          "The relaxation frequency of the water. Default: 17.4e9 Hz (tap water).",
          DoubleValue (17.4e9),
          MakeDoubleAccessor (&UnderwaterPropagationLossModel::m_f_ref),
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
  double epsilon0 = 8.85e-12;
  double c = 2.998e8;
  double omega = 2.0 * pi * m_frequency;

  std::complex<double> j (0.0, 1.0);
  std::complex<double> denom = 1.0 + j * (m_frequency / m_f_ref);
  std::complex<double> epsilon_r = m_epsilon_inf + (m_epsilon_s - m_epsilon_inf) / denom;
  std::complex<double> epsilon = epsilon0 * epsilon_r;

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

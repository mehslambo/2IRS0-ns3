#include "underwater-propagation-loss-model.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/attribute.h"
#include "ns3/object.h"
#include "ns3/mobility-model.h"
#include <cmath>

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
          "The conductivity (in S/m) of the water. Default: 0.02 S/m (tap water).",
          DoubleValue (0.02),
          MakeDoubleAccessor (&UnderwaterPropagationLossModel::m_sigma),
          MakeDoubleChecker<double> (0))
    .AddAttribute ("RelativePermittivity",
          "The relative permittivity of the water. Default: 81.0 (tap water).",
          DoubleValue (81.0),
          MakeDoubleAccessor (&UnderwaterPropagationLossModel::m_epsilon_r),
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
 // Get positions from the mobility models
 Vector posA = a->GetPosition ();
 Vector posB = b->GetPosition ();

 // Calculate the Euclidean distance D (in meters)
 double dx = posA.x - posB.x;
 double dy = posA.y - posB.y;
 double dz = posA.z - posB.z;
 double D = std::sqrt (dx*dx + dy*dy + dz*dz);

 // Constants
 const double pi = std::acos(-1.0);
 const double mu0 = 4 * pi * 1e-7;       // Permeability of free space (H/m)
 const double epsilon0 = 8.85e-12;         // Permittivity of free space (F/m)

 // Calculate absolute permittivity (F/m)
 double epsilon = epsilon0 * m_epsilon_r;

 // Angular frequency (rad/s)
 double omega = 2 * pi * m_frequency;

 // Compute the ratio sigma/(omega*epsilon)
 double term = m_sigma / (omega * epsilon);
 
 // Calculate the exact attenuation factor α (in Np/m) using:
 // α = ω * sqrt{ (mu0 * epsilon / 2) * [sqrt(1 + (sigma/(ωε))^2) - 1] }
 double sqrt_inner = std::sqrt (1.0 + term * term);
 double alpha = omega * std::sqrt ((mu0 * epsilon / 2.0) * (sqrt_inner - 1.0));

 // Convert α from nepers/m to dB/m using the conversion factor 20/ln(10) ≈ 8.686
 double alphaDbPerMeter = alpha * 8.686;

 // Total propagation loss (in dB) is given by:
 // Loss [dB] = α (in dB/m) × D (in m)
 double lossDb = alphaDbPerMeter * D;

 return lossDb;
}

int64_t
UnderwaterPropagationLossModel::DoAssignStreams (int64_t stream)
{
  // This model doesn't use random variables, so return 0
  return 0;
}

} // namespace ns3

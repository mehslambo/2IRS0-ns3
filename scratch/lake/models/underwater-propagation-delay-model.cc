#include "underwater-propagation-delay-model.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/attribute.h"
#include "ns3/object.h"
#include "ns3/mobility-model.h"
#include "ns3/simulator.h"
#include <cmath>

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
    .AddAttribute ("Conductivity",
          "The conductivity (in S/m) of the water. Default: 0.02 S/m (tap water).",
          DoubleValue (0.02),
          MakeDoubleAccessor (&UnderwaterPropagationDelayModel::m_sigma),
          MakeDoubleChecker<double> (0))
    .AddAttribute ("RelativePermittivity",
          "The relative permittivity of the water. Default: 81.0 (tap water).",
          DoubleValue (81.0),
          MakeDoubleAccessor (&UnderwaterPropagationDelayModel::m_epsilon_r),
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
  // Get node positions
  Vector posA = a->GetPosition ();
  Vector posB = b->GetPosition ();

  // Calculate the Euclidean distance D (in meters)
  double dx = posA.x - posB.x;
  double dy = posA.y - posB.y;
  double dz = posA.z - posB.z;
  double D = std::sqrt (dx*dx + dy*dy + dz*dz);

  // Constants
  const double pi = std::acos (-1.0);
  const double mu0 = 4 * pi * 1e-7;       // Permeability of free space (H/m)
  const double epsilon0 = 8.85e-12;         // Permittivity of free space (F/m)

  // Calculate absolute permittivity (F/m)
  double epsilon = epsilon0 * m_epsilon_r;

  // Angular frequency (rad/s)
  double omega = 2 * pi * m_frequency;

  // Compute term = sigma/(omega*epsilon)
  double term = m_sigma / (omega * epsilon);
  double sqrt_inner = std::sqrt (1.0 + term * term);

  // Calculate phase constant beta (rad/m)
  double beta = omega * std::sqrt ((mu0 * epsilon / 2.0) * (sqrt_inner + 1.0));

  // Calculate the one-way delay: t_delay = (D * beta) / omega (in seconds)
  double delaySeconds = (D * beta) / omega;

  return Seconds (delaySeconds);
}

int64_t
UnderwaterPropagationDelayModel::DoAssignStreams (int64_t stream)
{
  // This model doesn't use random variables, so return 0
  return 0;
}

} // namespace ns3

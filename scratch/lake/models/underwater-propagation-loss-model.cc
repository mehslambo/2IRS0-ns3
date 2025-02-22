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
                   "The signal frequency in Hz",
                   DoubleValue (1e6),
                   MakeDoubleAccessor (&UnderwaterPropagationLossModel::m_frequency),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Temperature",
                   "Water temperature in Celsius",
                   DoubleValue (25.0),
                   MakeDoubleAccessor (&UnderwaterPropagationLossModel::m_temperature),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Salinity",
                   "Water salinity in parts per thousand (ppt)",
                   DoubleValue (35.0),
                   MakeDoubleAccessor (&UnderwaterPropagationLossModel::m_salinity),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RelativePermittivity",
                   "Relative permittivity of water",
                   DoubleValue (80.0),
                   MakeDoubleAccessor (&UnderwaterPropagationLossModel::m_relativePermittivity),
                   MakeDoubleChecker<double> ());
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
  // Compute distance between transmitter and receiver.
  Vector posA = a->GetPosition ();
  Vector posB = b->GetPosition ();
  double dx = posA.x - posB.x;
  double dy = posA.y - posB.y;
  double dz = posA.z - posB.z;
  double distance = std::sqrt (dx * dx + dy * dy + dz * dz);

  // Physical constants.
  const double epsilon0 = 8.854187817e-12; // F/m
  const double mu0 = 4 * M_PI * 1e-7;      // H/m

  // Angular frequency ω = 2πf.
  double omega = 2 * M_PI * m_frequency;
  // Permittivity ε = ε0 * εr.
  double epsilon = epsilon0 * m_relativePermittivity;

  // --- Salinity-Based Conductivity ---
  // Compute sigma(25°, S)
  double sigma25 = m_salinity * (0.182521 
                  - 1.46192e-3 * m_salinity 
                  + 2.09324e-5 * m_salinity * m_salinity 
                  - 1.82025e-7 * m_salinity * m_salinity * m_salinity);
  double delta = 25.0 - m_temperature;
  double phi = delta * (0.02033 
                + 1.266e-4 * delta 
                + 2.464e-4 * delta * delta 
                - m_salinity * (1.849e-5 
                - 2.551e-7 * delta 
                + 2.551e-8 * delta * delta));
  double sigma = sigma25 * std::exp (-phi);

  // --- Exact Attenuation Constants ---
  // Compute factor = σ / (ωε) and the intermediate square-root term.
  double factor = sigma / (omega * epsilon);
  double sqrtTerm = std::sqrt (1.0 + factor * factor);
  // Attenuation constant α (in Np/m)
  double alpha = omega * std::sqrt ((mu0 * epsilon / 2.0) * (sqrtTerm - 1.0));

  // --- Path-Loss Calculation ---
  // The large-scale path loss in dB is given by:
  //   PathLoss[dB] = (20/ln(10)) * α * distance
  // (Note: 20/ln(10) ≈ 8.686 converts nepers to dB.)
  double lossDb = (20.0 / std::log (10.0)) * alpha * distance;
  return lossDb;
}

int64_t
UnderwaterPropagationLossModel::DoAssignStreams (int64_t stream)
{
  // This model doesn't use random variables, so return 0
  return 0;
}

} // namespace ns3

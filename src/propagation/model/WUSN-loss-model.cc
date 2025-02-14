/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Telum (www.telum.ru)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Kirill Andreev <andreev@telum.ru>
 */

#include "WUSN-loss-model.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include <cmath>


namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("WUSN");
  
NS_OBJECT_ENSURE_REGISTERED (WUSNLossModel);


WUSNLossModel::WUSNLossModel()
{
  frequency = 0;
}

WUSNLossModel::~WUSNLossModel()
{}

TypeId
WUSNLossModel::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::WUSNLossModel")
    .SetParent<PropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<WUSNLossModel> ()
    .AddAttribute ("Frequency", "frequency",
        DoubleValue (900000000),
        MakeDoubleAccessor (&WUSNLossModel::frequency),
        MakeDoubleChecker<double> ())
  ;
  return tid;
}

double
WUSNLossModel::DoCalcRxPower (double txPowerDbm,
                                          Ptr<MobilityModel> a,
                                          Ptr<MobilityModel> b) const
{
    //Check who is what
  Ptr<MobilityModel> sensor;
  Ptr<MobilityModel> ap;
  if (a->GetPosition().z < 0)
  {
    sensor = a;
    ap = b;
  }
  else
  {
    sensor = b;
    ap = a;
  }


  static const double C = 299792458.0; // speed of light in vacuum
  static const double d1 = 0.03;
  static const double burialDepth = -1 * sensor->GetPosition().z;  
  static const double eAcc = 27.42;
  static const double eAccAcc = 5.93;
  static const double degToRad = 15 * (M_PI / 180);
  static const double dUg = burialDepth / cos(degToRad);



  //Calculate alpha beta
  double innerSqrt = sqrt(1 + (eAccAcc / eAcc) * (eAccAcc / eAcc));
  double outer = 2 * M_PI * frequency;
  
  double alpha = outer * sqrt((innerSqrt - 1) * (eAcc / (C * C * 2)));
  double beta = outer * sqrt((innerSqrt + 1) * (eAcc / (C * C * 2)));


  //find d2 start point
  double distance = a->GetDistanceFrom(b);
  Vector direction(ap->GetPosition().x - sensor->GetPosition().x, ap->GetPosition().y - sensor->GetPosition().y, ap->GetPosition().z - sensor->GetPosition().z);
  Vector directionNorm = Vector(direction.x / distance, direction.y / distance, direction.z / distance);
  Vector calcPos = Vector(dUg * directionNorm.x, dUg * directionNorm.y, dUg * directionNorm.z);

  Vector another(sensor->GetPosition().x + calcPos.x, sensor->GetPosition().y + calcPos.y, sensor->GetPosition().z + calcPos.z);
  double d2 = CalculateDistance(another, ap->GetPosition());

  double phi = -288.8 + 20 * log10(d1 * d2  * dUg * beta) + 8.69 * alpha * dUg;
  double total = phi + 40 * log10(frequency);
  return txPowerDbm - total;
}


int64_t
WUSNLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

} // namespace ns3


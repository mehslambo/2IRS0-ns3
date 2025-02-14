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
 */

#include "Weissberger-loss-model.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include <cmath>


namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("Weissberger");
  
NS_OBJECT_ENSURE_REGISTERED (WeissbergerLossModel);


WeissbergerLossModel::WeissbergerLossModel()
{
  frequency = 0;
}

WeissbergerLossModel::~WeissbergerLossModel()
{}

TypeId
WeissbergerLossModel::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::WeissbergerLossModel")
    .SetParent<PropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<WeissbergerLossModel> ()
    .AddAttribute ("Frequency", "frequency",
            DoubleValue (868000000),
            MakeDoubleAccessor (&WeissbergerLossModel::frequency),
            MakeDoubleChecker<double> ())
  ;
  return tid;
}

double
WeissbergerLossModel::DoCalcRxPower (double txPowerDbm,
                                          Ptr<MobilityModel> a,
                                          Ptr<MobilityModel> b) const
{
  double distance = a->GetDistanceFrom(b) / 100.0;
  double ghz = frequency / 1000000000;
  double total = 0;

  if (distance == 0)
  {
    return txPowerDbm;
  }

  NS_ASSERT (distance <= 400);
  if (distance > 14)
  {
    total = 1.33 * pow(ghz, 0.284) * pow(distance, 0.588);
  }
  else
  {
    total = 0.45 * pow(ghz, 0.284) * distance;    
  }

  return txPowerDbm - total;
}


int64_t
WeissbergerLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

} // namespace ns3


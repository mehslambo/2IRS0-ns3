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
#ifndef WEISSBERGER_LOSS_MODEL_H
#define WEISSBERGER_LOSS_MODEL_H

#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-cache.h"

namespace ns3
{
/**
 * \ingroup propagation
 *
 * \brief a  Jakes narrowband propagation model.
 * Symmetrical cache for JakesProcess
 */

class WeissbergerLossModel : public PropagationLossModel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId ();

  WeissbergerLossModel ();

  virtual ~WeissbergerLossModel ();
  
  double frequency;

  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  WeissbergerLossModel (const WeissbergerLossModel &);

  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  WeissbergerLossModel & operator = (const WeissbergerLossModel &);
  

  double DoCalcRxPower (double txPowerDbm,
                        Ptr<MobilityModel> a,
                        Ptr<MobilityModel> b) const;


  virtual int64_t DoAssignStreams (int64_t stream);
};

} // namespace ns3

#endif /* WUSN_LOSS_MODEL_H */


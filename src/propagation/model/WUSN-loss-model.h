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
#ifndef WUSN_LOSS_MODEL_H
#define WUSN_LOSS_MODEL_H

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

class WUSNLossModel : public PropagationLossModel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId ();

  WUSNLossModel ();

  virtual ~WUSNLossModel ();
  
  double frequency;

  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  WUSNLossModel (const WUSNLossModel &);

  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  WUSNLossModel & operator = (const WUSNLossModel &);
  

  double DoCalcRxPower (double txPowerDbm,
                        Ptr<MobilityModel> a,
                        Ptr<MobilityModel> b) const;


  virtual int64_t DoAssignStreams (int64_t stream);
};

} // namespace ns3

#endif /* WUSN_LOSS_MODEL_H */


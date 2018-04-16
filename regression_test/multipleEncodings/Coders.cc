/******************************************************************************
 * Copyright (c) 2000-2018 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Baranyi, Botond
 *
 ******************************************************************************/

#include "Phantom.hh"
#include "TtcnTypes.hh"
#include "AsnTypes.hh"

namespace Phantom {

  BITSTRING f__enc__abc(const TtcnTypes::R1& x)
  {
    return oct2bit(char2oct("abc"));
  }
  
  INTEGER f__dec__per(BITSTRING& x, AsnTypes::SetOf& y)
  {
    if (x == oct2bit(char2oct("PER"))) {
      y.set_size(2);
      y[0] = 0;
      y[1] = 1;
      x = BITSTRING(0, NULL);
      return 0;
    }
    else if (x.lengthof() < 24) {
      return 2;
    }
    else {
      return 1;
    }
  }

}

namespace TtcnTypes {

  INTEGER f__dec__abc(BITSTRING& x, R1& y)
  {
    if (x == oct2bit(char2oct("abc"))) {
      y.f1() = E1::val1;
      x = BITSTRING(0, NULL);
      return 0;
    }
    else if (x.lengthof() < 24) {
      return 2;
    }
    else {
      return 1;
    }
  }
  
  INTEGER f__dec__abc2(BITSTRING& x, R1& y)
  {
    if (x == oct2bit(char2oct("abc"))) {
      y.f1() = E1::val2;
      x = BITSTRING(0, NULL);
      return 0;
    }
    else if (x.lengthof() < 24) {
      return 2;
    }
    else {
      return 1;
    }
  }

}

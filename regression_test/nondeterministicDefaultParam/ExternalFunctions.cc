/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Baranyi, Botond
 *
 ******************************************************************************/

#include "Resources.hh"

namespace Resources {

static INTEGER current_number(0);

INTEGER ef__next__number()
{
  return ++current_number;
}

void ef__reset()
{
  current_number = 0;
}

BITSTRING ef__enc__uni(const Uni& x) {
  return int2bit(x.i(), 8);
}

}

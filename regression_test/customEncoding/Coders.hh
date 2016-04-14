/******************************************************************************
 * Copyright (c) 2000-2016 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Baranyi, Botond
 *
 ******************************************************************************/

#include "Custom2.hh"
#include "Custom1.hh"

#ifndef CODERS_HH
#define CODERS_HH

namespace Custom2 {

// Coding functions for the record type in test 1
BITSTRING f__enc__rec(const Custom3::Rec& x);
INTEGER f__dec__rec(BITSTRING& b, Custom3::Rec& x);

// Coding functions for the union type in test 3
BITSTRING f__enc__uni(const Custom1::Uni& x);
INTEGER f__dec__uni(BITSTRING& b, Custom1::Uni& x);

}

namespace Custom1 {

// Coding functions for the record-of type in test 2
BITSTRING f__enc__recof(const RecOf& x);
INTEGER f__dec__recof(BITSTRING& b, RecOf& x);

}

#endif

/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#ifndef OER_HH
#define OER_HH

#include "Types.h"

struct TTCN_OERdescriptor_t 
{
  int bytes;
  boolean signed_;
};

extern const TTCN_OERdescriptor_t BOOLEAN_oer_;

#endif /* OER_HH */


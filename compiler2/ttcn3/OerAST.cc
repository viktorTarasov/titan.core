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
#include "OerAST.hh"

OerAST::OerAST() :
  bytes(-1),
  signed_(false),
  length(-1),
  extendable(false),
  nr_of_root_comps(0),
  ext_attr_groups(),
  p()
{}

OerAST::~OerAST() {
  for (size_t i = 0; i < ext_attr_groups.size(); i++) {
    delete ext_attr_groups[i];
  }
  ext_attr_groups.clear();
  for (size_t i = 0; i < p.size(); i++) {
    delete p[i];
  }
  p.clear();
}

bool OerAST::empty() const {
  return bytes == -1 && signed_ == false && length == -1 &&
    extendable == false && nr_of_root_comps == 0 && ext_attr_groups.empty() && p.empty();
}

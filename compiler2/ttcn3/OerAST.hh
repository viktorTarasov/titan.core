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
#ifndef OERAST_HH
#define OERAST_HH

class OerAST {
public:
  int bytes;
  bool signed_;
  
  OerAST();
  bool empty() const;
};

#endif /* OERAST_HH */


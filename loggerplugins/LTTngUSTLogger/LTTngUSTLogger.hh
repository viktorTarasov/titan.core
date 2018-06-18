/******************************************************************************
 * Copyright (c) 2016 EfficiOS Inc., Philippe Proulx
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html
 *
 * Contributors:
 *   Proulx, Philippe
 ******************************************************************************/

#ifndef LTTngUSTLogger_HH
#define LTTngUSTLogger_HH

#include <stdio.h>
#include "ILoggerPlugin.hh"

namespace TitanLoggerApi { class TitanLogEvent; }

class LTTngUSTLogger: public ILoggerPlugin
{
public:
  LTTngUSTLogger();
  virtual ~LTTngUSTLogger();
  void log(const TitanLoggerApi::TitanLogEvent& event, bool log_buffered,
           bool separate_file, bool use_emergency_mask);
  inline bool is_static() { return false; }
  inline void init(const char *options) { }
  inline void fini() { }

private:
  CHARSTRING joinStrings(const TitanLoggerApi::Strings& strings);
  bool hasTpp;
};

#endif	// LTTngUSTLogger_HH

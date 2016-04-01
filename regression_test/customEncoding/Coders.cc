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

#include "Coders.hh"

using Custom1::c__separator;

namespace Custom2 {

BITSTRING f__enc__rec(const Custom3::Rec& x)
{
  return int2bit(x.num(), 8);
}

INTEGER f__dec__rec(BITSTRING& b, Custom3::Rec& x)
{
  x.num() = bit2int(b);
  x.str() = "c++";
  return 0;
}

BITSTRING f__enc__uni(const Custom1::Uni& x)
{
  if (x.get_selection() == Custom1::Uni::ALT_i) {
    return c__separator + int2bit(x.i(), 8) + c__separator;
  }
  else {
    return c__separator + c__separator;
  }
}

INTEGER f__dec__uni(BITSTRING& b, Custom1::Uni& x)
{
  int b_len = b.lengthof();
  int sep_len = c__separator.lengthof();
  if (b_len >= 2 * sep_len &&
      substr(b, 0, sep_len) == c__separator &&
      substr(b, b_len - sep_len, sep_len) == c__separator) {
    if (b_len > 2 * sep_len) {
      x.i() = bit2int(substr(b, sep_len, b_len - 2 * sep_len));
    }
    return 0;
  }
  else {
    return 1;
  }
}

} // namespace Custom2

namespace Custom1 {

BITSTRING f__enc__recof(const RecOf& x)
{
  BITSTRING res = x[0];
  for (int i = 1; i < x.size_of(); ++i) {
    res = res + c__separator + x[i];
  }
  return res;
}

int find_bitstring(const BITSTRING& src, int start, const BITSTRING& fnd)
{
  int len = fnd.lengthof();
  for (int i = start; i <= src.lengthof() - len; ++i) {
    if (substr(src, i, len) == fnd) {
      return i;
    }
  }
  return -1;
}

INTEGER f__dec__recof(BITSTRING& b, RecOf& x)
{
  int start = 0;
  int end = find_bitstring(b, start, c__separator);
  int index = 0;
  while(end != -1) {
    x[index] = substr(b, start, end - start);
    ++index;
    start = end + c__separator.lengthof();
    end = find_bitstring(b, start, c__separator);
  }
  x[index] = substr(b, start, b.lengthof() - start);
  return 0;
}

} // namespace Custom1

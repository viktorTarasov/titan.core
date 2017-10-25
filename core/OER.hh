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
#include "Vector.hh"

class TTCN_Buffer;
class ASN_BERdescriptor_t;
class ASN_Tag_t;

struct TTCN_OERdescriptor_t 
{
  int bytes;
  boolean signed_;
  int length;
  boolean extendable; // Always false. Possibly a bug with is_extendable()
};

struct OER_struct
{
  OER_struct() : opentype_poses(0), pos(0) {}
  Vector<size_t> opentype_poses;
  size_t pos;
};

void encode_oer_length(size_t num_bytes, TTCN_Buffer& buf, boolean seof);

size_t decode_oer_length(TTCN_Buffer& buf, boolean seof);

void encode_oer_tag(const ASN_BERdescriptor_t& descr, TTCN_Buffer& buf);

ASN_Tag_t decode_oer_tag(TTCN_Buffer& buf);

extern const TTCN_OERdescriptor_t BOOLEAN_oer_;
extern const TTCN_OERdescriptor_t INTEGER_oer_;
extern const TTCN_OERdescriptor_t BITSTRING_oer_;
extern const TTCN_OERdescriptor_t OCTETSTRING_oer_;
extern const TTCN_OERdescriptor_t FLOAT_oer_;
extern const TTCN_OERdescriptor_t ASN_NULL_oer_;
extern const TTCN_OERdescriptor_t IA5String_oer_;
extern const TTCN_OERdescriptor_t VisibleString_oer_;
extern const TTCN_OERdescriptor_t NumericString_oer_;
extern const TTCN_OERdescriptor_t PrintableString_oer_;
extern const TTCN_OERdescriptor_t BMPString_oer_;
extern const TTCN_OERdescriptor_t UniversalString_oer_;
extern const TTCN_OERdescriptor_t UTF8String_oer_;
extern const TTCN_OERdescriptor_t TeletexString_oer_;
extern const TTCN_OERdescriptor_t VideotexString_oer_;
extern const TTCN_OERdescriptor_t GraphicString_oer_;
extern const TTCN_OERdescriptor_t GeneralString_oer_;
extern const TTCN_OERdescriptor_t OBJID_oer_;
extern const TTCN_OERdescriptor_t ASN_ROID_oer_;
extern const TTCN_OERdescriptor_t EMBEDDED_PDV_oer_;
extern const TTCN_OERdescriptor_t EXTERNAL_oer_;
extern const TTCN_OERdescriptor_t ObjectDescriptor_oer_;

#endif /* OER_HH */


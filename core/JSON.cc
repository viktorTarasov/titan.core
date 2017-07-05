/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Baji, Laszlo
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#include "JSON.hh"
#include "../common/memory.h"
#include "Integer.hh"
#include "Float.hh"
#include "Bitstring.hh"
#include "Hexstring.hh"
#include "Octetstring.hh"
#include "Charstring.hh"
#include "Universal_charstring.hh"
#include "Addfunc.hh"

#include <openssl/bn.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// JSON descriptors for base types
const TTCN_JSONdescriptor_t INTEGER_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t FLOAT_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t BOOLEAN_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t BITSTRING_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t HEXSTRING_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t OCTETSTRING_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t CHARSTRING_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t UNIVERSAL_CHARSTRING_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t VERDICTTYPE_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t GeneralString_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t NumericString_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t UTF8String_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t PrintableString_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t UniversalString_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t BMPString_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t GraphicString_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t IA5String_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t TeletexString_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t VideotexString_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t VisibleString_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t ASN_NULL_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t OBJID_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t ASN_ROID_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t ASN_ANY_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };

const TTCN_JSONdescriptor_t ENUMERATED_json_ = { FALSE, NULL, FALSE, NULL, FALSE, FALSE };



// Never use buff.get_read_data() without checking if it has enough bytes in the
// buffer.
const unsigned char* check_and_get_buffer(const TTCN_Buffer& buff, int bytes) {
  if (bytes < 0) {
    TTCN_error("Incorrect length byte received: %d, while decoding using cbor2json()", bytes);
  }
  if (buff.get_pos() + bytes > buff.get_len()) {
    TTCN_error("Not enough bytes in bytestream while decoding using cbor2json().");
  }
  return buff.get_read_data();
}

void encode_ulong_long_int_cbor(TTCN_Buffer& buff, int bytes, unsigned long long int value) {
  for (int i = bytes - 1; i >= 0; i--) {
    buff.put_c(static_cast<unsigned char>((value >> i*8)));
  }
}


// major_type parameter needed for the string encoding
void encode_int_cbor(TTCN_Buffer& buff, int major_type, INTEGER& int_num) {
  bool is_negative = false;
  int_val_t num = int_num.get_val();
  if (num.is_negative()) {
    major_type = 1 << 5;
    int_num = (int_num * -1) - 1;
    num = int_num.get_val();
    is_negative = true;
  }
  if (num.is_native()) {
    unsigned int uns_num = num.get_val();
    if (uns_num <= 23) {
      buff.put_c(static_cast<unsigned char>(major_type + uns_num));
    } else if (uns_num <= 0xFF) { // 8 bit
      buff.put_c(static_cast<unsigned char>(major_type + 24));
      encode_ulong_long_int_cbor(buff, 1, uns_num);
    } else if (uns_num <= 0xFFFF) { // 16 bit
      buff.put_c(static_cast<unsigned char>(major_type + 25));
      encode_ulong_long_int_cbor(buff, 2, uns_num);
    } else if (uns_num <= 0xFFFFFFFF) { // 32 bit
      buff.put_c(static_cast<unsigned char>(major_type + 26));
      encode_ulong_long_int_cbor(buff, 4, uns_num);
    }
  } else {
    BIGNUM* bn = BN_dup(int_num.get_val().get_val_openssl());
    INTEGER bn_length = BN_num_bytes(bn);
    long long int long_int = int_num.get_long_long_val();
    if (bn_length <= 4) { // 32 bit
      buff.put_c(static_cast<unsigned char>(major_type + 26));
      encode_ulong_long_int_cbor(buff, 4, long_int);
    } else if (bn_length <= 8) {
      buff.put_c(static_cast<unsigned char>(major_type + 27));
      encode_ulong_long_int_cbor(buff, 8, long_int);
    } else {
      // It is a bignum. Encode as bytestring
      major_type = 6 << 5; // Major type 6 for bignum
      major_type += is_negative ? 3 : 2; // Tag 2 or 3 for negative
      buff.put_c(static_cast<unsigned char>(major_type));
      major_type = 2 << 5; // Major type 2 for bytestring
      encode_int_cbor(buff, major_type, bn_length); // encode the length of the bignum
      size_t buff_len = bn_length.get_val().get_val();
      unsigned char* tmp_num = static_cast<unsigned char*>(Malloc(buff_len*sizeof(unsigned char)));
      BN_bn2bin(bn, tmp_num);
      buff.put_s(buff_len, reinterpret_cast<const unsigned char*>(tmp_num));
      Free(tmp_num);
    }
    BN_free(bn);
  }
}


void decode_int_cbor(TTCN_Buffer& buff, int bytes, INTEGER& value) {
  const unsigned char* tmp = check_and_get_buffer(buff, bytes);
  TTCN_Buffer tmp_buf;
  tmp_buf.put_s(bytes, tmp);
  OCTETSTRING os;
  tmp_buf.get_string(os);
  value = oct2int(os);
  buff.increase_pos(bytes);
}

void decode_uint_cbor(TTCN_Buffer& buff, int bytes, unsigned int& value) {
  value = 0;
  const unsigned char* tmp = check_and_get_buffer(buff, bytes);
  for (int i = bytes - 1; i >= 0; i--) {
    value += *(tmp) << i*8;
    tmp++;
  }
  buff.increase_pos(bytes);
}

void decode_ulong_long_int_cbor(TTCN_Buffer& buff, int bytes, unsigned long long int& value) {
  value = 0;
  const unsigned char* tmp = check_and_get_buffer(buff, bytes);
  for (int i = bytes - 1; i >= 0; i--) {
    value += *(tmp) << i*8;
    tmp++;
  }
  buff.increase_pos(bytes);
}

void decode_integer_cbor(TTCN_Buffer& buff, int minor_type, INTEGER& result) {
  if (minor_type <= 23) {
    result = minor_type;
  } else if (minor_type == 24) { // A number in 8 bits
    unsigned int num;
    decode_uint_cbor(buff, 1, num);
    result = num;
  } else if (minor_type == 25) { // A number in 16 bits
    unsigned int num;
    decode_uint_cbor(buff, 2, num);
    result = num;
  } else if (minor_type == 26) { // A number in 32 bits
    unsigned int num;
    decode_uint_cbor(buff, 4, num);
    result = num;
  } else if (minor_type == 27) { // A number in 64 bits
    decode_int_cbor(buff, 8, result);
  }
}


void decode_bytestring_cbor(TTCN_Buffer& buff, JSON_Tokenizer& tok, int minor_type, int tag) {
  INTEGER length;
  decode_integer_cbor(buff, minor_type, length);
  const unsigned char* tmp = check_and_get_buffer(buff, length.get_val().get_val());
  OCTETSTRING os(length.get_val().get_val(), tmp);
  buff.increase_pos(length.get_val().get_val());
  CHARSTRING cs;
  if (tag == 22 || tag == 23 || tag == 2 || tag == 3) { // base64 or base64url or +-bigint
    cs = encode_base64(os);
    // The difference between the base64 and base64url encoding is that the
    // + is replaced with -, the / replaced with _ and the trailing = padding
    // is removed.
    if (tag != 22) { // if tag is not base64 >--> base64url
      const char* data = (const char*)cs;
      char* pch = strchr(const_cast<char*>(data),'+');
      while (pch!=NULL)
      {
        *pch = '-';
        pch=strchr(pch+1,'+');
      }
      pch = strchr(const_cast<char*>(data),'/');
      while (pch!=NULL)
      {
        *pch = '_';
        pch=strchr(pch+1,'/');
      }
      // Max 2 padding = is possible
      if (cs[cs.lengthof()-1] == "=") {
        cs = replace(cs, cs.lengthof()-1, 1, "");
      }
      if (cs[cs.lengthof()-1] == "=") {
        cs = replace(cs, cs.lengthof()-1, 1, "");
      }
    }    
  } else if (tag == 21) { // base16
    cs = oct2str(os);
  }
  // If the bignum encoded as bytestring is negative the tilde is needed before
  // the base64url encoding
  char* tmp_str = mprintf("\"%s%s\"", tag == 3 ? "~" : "", (const char*)cs);
  tok.put_next_token(JSON_TOKEN_STRING, tmp_str);
  Free(tmp_str);
}

// RAW descriptor for raw encoding
TTCN_RAWdescriptor_t cbor_float_raw_ = {64,SG_NO,ORDER_LSB,ORDER_LSB,ORDER_LSB,ORDER_LSB,EXT_BIT_NO,ORDER_LSB,ORDER_LSB,TOP_BIT_INHERITED,0,0,0,8,0,NULL,-1,CharCoding::UNKNOWN};
const TTCN_Typedescriptor_t cbor_float_descr_ = { NULL, NULL, &cbor_float_raw_, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE };


void json2cbor_coding(TTCN_Buffer& buff, JSON_Tokenizer& tok, size_t& num_of_items) {
  json_token_t token;
  char* content = NULL;
  size_t len;
  size_t prev_pos = tok.get_buf_pos();
  tok.get_next_token(&token, &content, &len);
  switch(token) {
    case JSON_TOKEN_NUMBER: {
      char *str = mcopystrn(content, len);
      size_t curr_pos = tok.get_buf_pos();
      tok.set_buf_pos(prev_pos);
      bool is_float = false;
      tok.check_for_number(&is_float);
      tok.set_buf_pos(curr_pos);
      if (is_float) {
        int c = 7 << 5; // Major type 7
        c += 27; // Minor type 27 (64 bit floating point) always
        buff.put_c(c);
        double d;
        sscanf(str, "%lf", &d);
        FLOAT f = d;
        f.encode(cbor_float_descr_, buff, TTCN_EncDec::CT_RAW);
      } else {
        int c = 0; // Major type 0
        INTEGER int_num = str2int(str);
        encode_int_cbor(buff, c, int_num);
      }
      Free(str);
      num_of_items++;
      break;
    }
    case JSON_TOKEN_STRING:
    case JSON_TOKEN_NAME: {
      int c = 3 << 5; // Major type 3
      INTEGER length = token == JSON_TOKEN_NAME ? len : len - 2; // 2 "-s
      encode_int_cbor(buff, c, length);
      char * str = mcopystrn(token == JSON_TOKEN_NAME ? content : content+1, length.get_val().get_val()); // Remove "-s
      buff.put_string(str);
      Free(str);
      num_of_items++;
      break;
    }
    case JSON_TOKEN_ARRAY_START: {
      int c = 4 << 5; // Major type 4
      size_t nof_items = 0;
      TTCN_Buffer sub_buff;
      while ((prev_pos = tok.get_buf_pos(), tok.get_next_token(&token, NULL, NULL))) {
        if (token != JSON_TOKEN_ARRAY_END) {
          tok.set_buf_pos(prev_pos);
          json2cbor_coding(sub_buff, tok, nof_items);
        } else {
          INTEGER num = nof_items;
          encode_int_cbor(buff, c, num);
          buff.put_buf(sub_buff);
          break;
        }
      }
      num_of_items++;
      break;
    }
    case JSON_TOKEN_ARRAY_END:
      TTCN_error("Unexpected array end character while encoding using json2cbor().");
      break;
    case JSON_TOKEN_OBJECT_START: {
      int c = 5 << 5; // Major type 5
      size_t nof_items = 0;
      TTCN_Buffer sub_buff;
      while ((prev_pos = tok.get_buf_pos(), tok.get_next_token(&token, NULL, NULL))) {
        if (token != JSON_TOKEN_OBJECT_END) { // todo hibas json eseten vegtelen ciklus?
          tok.set_buf_pos(prev_pos);
          json2cbor_coding(sub_buff, tok, nof_items);
        } else {
          INTEGER num = nof_items / 2; // num is the number of key-value pairs
          encode_int_cbor(buff, c, num);
          buff.put_buf(sub_buff);
          break;
        }
      }
      num_of_items++;
      break;
    }
    case JSON_TOKEN_OBJECT_END:
      TTCN_error("Unexpected object end character while encoding using json2cbor().");
    case JSON_TOKEN_LITERAL_FALSE:
    case JSON_TOKEN_LITERAL_TRUE:
    case JSON_TOKEN_LITERAL_NULL: {
      int c = 7 << 5; // Major type 7
      INTEGER i;
      if (token == JSON_TOKEN_LITERAL_FALSE) {
        i = 20;
      } else if (token == JSON_TOKEN_LITERAL_TRUE) {
        i = 21;
      } else if (token == JSON_TOKEN_LITERAL_NULL) {
        i = 22;
      }
      encode_int_cbor(buff, c, i);
      num_of_items++;
      break;
    }
    default:
      TTCN_error("Unexpected json token %i, while encoding using json2cbor().", token);
  }
}

void cbor2json_coding(TTCN_Buffer& buff, JSON_Tokenizer& tok, bool in_object) {
  unsigned char type = *(check_and_get_buffer(buff, 1));
  buff.increase_pos(1);
  int major_type = type >> 5; // First 3 bit of byte
  int minor_type = type & 0x1F; // Get the last 5 bits
  switch(major_type) {
    case 0: { // Integer
      INTEGER i;
      decode_integer_cbor(buff, minor_type, i);
      if (i.is_native()) {
        char* tmp_str = mprintf("%u", i.get_val().get_val());
        tok.put_next_token(JSON_TOKEN_NUMBER, tmp_str);
        Free(tmp_str);
      } else {
        char* tmp_str = i.get_val().as_string();
        tok.put_next_token(JSON_TOKEN_NUMBER, tmp_str);
        Free(tmp_str);
      }
      break;
    }
    case 1: // Negative integer
      switch (minor_type) {
        case 24: { // Integer on 1 byte
          unsigned int num;
          decode_uint_cbor(buff, 1, num);
          char* tmp_str = mprintf("%d", (num+1)*-1);
          tok.put_next_token(JSON_TOKEN_NUMBER, tmp_str);
          Free(tmp_str);
          break;
        }
        case 25: { // Integer on 2 byte
          unsigned int num;
          decode_uint_cbor(buff, 2, num);
          char* tmp_str = mprintf("%d", (num+1)*-1);
          tok.put_next_token(JSON_TOKEN_NUMBER, tmp_str);
          Free(tmp_str);
          break;
        }
        case 26: { // Integer on 4 byte
          unsigned long long int num;
          decode_ulong_long_int_cbor(buff, 4, num);
          INTEGER i;
          i.set_long_long_val((num+1)*-1);
          char* tmp_str = i.get_val().as_string();
          tok.put_next_token(JSON_TOKEN_NUMBER, tmp_str);
          Free(tmp_str);
          break;
        }
        case 27: { // Integer on 8 byte
          INTEGER i;
          decode_int_cbor(buff, 8, i);
          i = i + 1;
          i = i * -1;
          char* tmp_str = i.get_val().as_string();
          tok.put_next_token(JSON_TOKEN_NUMBER, tmp_str);
          Free(tmp_str);
          break;
        }
        default:
          if (minor_type < 24) { // Integer  0 <= num <= 24
            char* tmp_str = mprintf("%d", (minor_type+1)*-1);
            tok.put_next_token(JSON_TOKEN_NUMBER, tmp_str);
            Free(tmp_str);
          }
      }
      break;
    case 2: // Bytestring, encoded into a base64url json string
      decode_bytestring_cbor(buff, tok, minor_type, 23); // base64url by default
      break;
    case 3: { // String
      INTEGER length;
      decode_integer_cbor(buff, minor_type, length);
      const unsigned char* tmp = check_and_get_buffer(buff, length.get_val().get_val());
      
      char* json_str = mcopystrn((const char*)tmp, length.get_val().get_val());
      if (in_object) {
        tok.put_next_token(JSON_TOKEN_NAME, json_str);
      } else {
        char* tmp_str = mprintf("\"%s\"", json_str);
        tok.put_next_token(JSON_TOKEN_STRING, tmp_str);
        Free(tmp_str);
      }
      Free(json_str);
      buff.increase_pos(length.get_val().get_val());
      break;
    }
    case 4: { // Array
      tok.put_next_token(JSON_TOKEN_ARRAY_START, NULL);
      INTEGER num_of_items;
      decode_integer_cbor(buff, minor_type, num_of_items);
      for (INTEGER i = 0; i < num_of_items; i = i + 1) {
        cbor2json_coding(buff, tok, false);
      }
      tok.put_next_token(JSON_TOKEN_ARRAY_END, NULL);
      break;
    }
    case 5: { // Object
      tok.put_next_token(JSON_TOKEN_OBJECT_START, NULL);
      INTEGER num_of_items;
      decode_integer_cbor(buff, minor_type, num_of_items);
      num_of_items = num_of_items * 2; // Number of all keys and values
      for (INTEGER i = 0; i < num_of_items; i = i + 1) {
        // whether to put : or , after the next token
        in_object = i.get_val().get_val() % 2 == 0;
        cbor2json_coding(buff, tok, in_object); 
      }
      tok.put_next_token(JSON_TOKEN_OBJECT_END, NULL);
      break;
    }
    case 6: {
      int tag = minor_type;
      switch(tag) {
        case 2: // Positive bignum
        case 3: // Negative bignum
        case 21: // bytestring as base16
        case 22: // bytestring as base64
        case 23: // bytestring as base64url
          type = *(check_and_get_buffer(buff, 1));
          buff.increase_pos(1);
          minor_type = type & 0x1F; // Get the last 5 bits
          decode_bytestring_cbor(buff, tok, minor_type, tag);
          break;
        default:
          cbor2json_coding(buff, tok, in_object);
      }
      break;
    }
    case 7: // Other values
      switch (minor_type) {
        case 20: // False
          tok.put_next_token(JSON_TOKEN_LITERAL_FALSE, NULL);
          break;
        case 21: // True
          tok.put_next_token(JSON_TOKEN_LITERAL_TRUE, NULL);
          break;
        case 22: // NULL
        tok.put_next_token(JSON_TOKEN_LITERAL_NULL, NULL);
          break;
        case 25: { // Half precision float 16 bit
          // Decoding algorithm from the standard
          const unsigned char* halfp = check_and_get_buffer(buff, 2);
          buff.increase_pos(2);
          int half = (halfp[0] << 8) + halfp[1];
          int exp = (half >> 10) & 0x1f;
          int mant = half & 0x3ff;
          double val;
          if (exp == 0) val = ldexp(mant, -24);
          else if (exp != 31) val = ldexp(mant + 1024, exp - 25);
          else val = mant == 0 ? INFINITY : NAN;
          val = half & 0x8000 ? -val : val;
          FLOAT f = val;
          f.JSON_encode(cbor_float_descr_, tok);
          break;
        }
        case 26: { // Single precision float 32bit
          OCTETSTRING os(4, check_and_get_buffer(buff, 4));
          buff.increase_pos(4);
          INTEGER i = oct2int(os);
          // Some non standard way but it is widely accepted
          union
          {
            unsigned int num;
            float fnum;
          } my_union;
          my_union.num = i.get_long_long_val();
          float f2 = my_union.fnum;
          FLOAT f = f2;
          f.JSON_encode(cbor_float_descr_, tok);
          break;
        }
        case 27: { // Double precision float 64bit
          cbor_float_raw_.fieldlength = 64;
          FLOAT f;
          OCTETSTRING os(8, check_and_get_buffer(buff, 8));
          INTEGER i = oct2int(os);
          if (i.get_long_long_val() != 0x7FF8000000000000) { // NAN    
            f.decode(FLOAT_descr_, buff, TTCN_EncDec::CT_RAW);
            f.JSON_encode(cbor_float_descr_, tok);
          } else {
            tok.put_next_token(JSON_TOKEN_STRING, "\"not_a_number\"");
            buff.increase_pos(8);
          }
          break;
        }
        default: {
          // put the simple value into the the json
          if (minor_type >= 0 && minor_type <= 23) {
            char* tmp_str = mprintf("%d", minor_type);
            tok.put_next_token(JSON_TOKEN_NUMBER, tmp_str);
            Free(tmp_str);
          } else if (minor_type == 24) { // The value is on the next byte
            int simple_value = *(buff.get_read_data());
            buff.increase_pos(1);
            char* tmp_str = mprintf("%d", simple_value);
            tok.put_next_token(JSON_TOKEN_NUMBER, tmp_str);
            Free(tmp_str);
          }
        }
      }
      break;
    default:
      TTCN_error("Unexpected major type %i while decoding using cbor2json().", major_type);
  }
}

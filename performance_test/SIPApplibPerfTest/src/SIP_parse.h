/******************************************************************************
 * Copyright (c) 2000-2018 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 ******************************************************************************/
//
//  File:               SIP_parse.h
//  Rev:                <RnXnn>
//  Prodnr:             CNL 113 319
//  Updated:            2013-02-20
//  Contact:            http://ttcn.ericsson.se
//  Reference:          RFC3261, RFC2806, RFC2976, RFC3262, RFC3311, RFC3323, 
//                      RFC3325, RFC3326, RFC3265, RFC3455, RFC4244
//                      IETF Draft draft-ietf-dip-session-timer-15.txt,
//                      IETF Draft draft-levy-sip-diversion-08.txt
//                      531/0363-FCP 101 5091

#ifndef PARSE_HH
#define PARSE_HH

#define BUF_SIZE 1901		// 1300 by the SIP standard


extern int SIP_parse_lex(void);
extern int SIP_parse_debug;

int my_SIP_parse_input(char *buffer, int max_size);

extern int num_chars;


#endif

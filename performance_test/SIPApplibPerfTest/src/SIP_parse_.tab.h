/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     _SP = 258,
     LONE_CRLF = 259,
     EXTENSION_CODE = 260,
     REASON_PHRASE = 261,
     SIP_VERSION = 262,
     SIP_SPHT_VERSION = 263,
     _METHOD = 264,
     CONTENT_WITHOUTENDINGCRLF = 265,
     _CALLID = 266,
     WWW_AUTHENTICATELWSCOLON = 267,
     _CRLF = 268,
     PROXY_AUTHENTICATELWSCOLON = 269,
     TOLWSCOLON = 270,
     CONTACTLWSCOLON = 271,
     ACCEPTRESPRIOLWSCOLON = 272,
     RESPRIOLWSCOLON = 273,
     FROMLWSCOLON = 274,
     _TYPEID = 275,
     TOKENS = 276,
     QUOTED_STRING = 277,
     EPARENT = 278,
     SPARENT = 279,
     OTHERLWSCOLON = 280,
     CALL_IDLWSCOLON = 281,
     SOMELWS = 282,
     _LAES_CONTENT = 283,
     _TOKEN = 284,
     _STOKEN = 285,
     _DIGEST = 286,
     _COMMENT = 287,
     USERINFO_AT = 288,
     _STAR = 289,
     GEOLOCERRORLWSCOLON = 290,
     GEOLOCATIONLWSCOLON = 291,
     SUPPORTEDLWSCOLON = 292,
     VIALWSCOLON = 293,
     SUBJECTLWSCOLON = 294,
     CONTENT_ENCODINGLWSCOLON = 295,
     CONTENT_LENGTHLWSCOLON = 296,
     SOMEDIGITS = 297,
     CONTENT_TYPELWSCOLON = 298,
     ACCEPTLWSCOLON = 299,
     ACCEPT_ENCODINGLWSCOLON = 300,
     ACCEPT_LANGUAGELWSCOLON = 301,
     AUTHINFOLWSCOLON = 302,
     CALL_INFOLWSCOLON = 303,
     MINEXPIRESLWSCOLON = 304,
     CSEQLWSCOLON = 305,
     EVENTLWSCOLON = 306,
     ALLOWEVENTLWSCOLON = 307,
     SUBSTATEWSCOLON = 308,
     DATELWSCOLON = 309,
     MIME_VERSIONLWSCOLON = 310,
     ORGANIZATIONLWSCOLON = 311,
     RECORD_ROUTELWSCOLON = 312,
     PCALLEPPTYLWSCOLON = 313,
     PVISITEDNETLWSCOLON = 314,
     REQUIRELWSCOLON = 315,
     HISTORYLWSCOLON = 316,
     TIMESTAMPLWSCOLON = 317,
     USER_AGENTLWSCOLON = 318,
     PASSOCURILWSCOLON = 319,
     DIVERSIONWSCOLON = 320,
     ERROR_INFOLWSCOLON = 321,
     RETRY_AFTERLWSCOLON = 322,
     SERVERLWSCOLON = 323,
     UNSUPPORTEDLWSCOLON = 324,
     WARNINGLWSCOLON = 325,
     PASSERTEDLWSCOLON = 326,
     REASONLWSCOLON = 327,
     PPREFERREDLWSCOLON = 328,
     PRIVACYLWSCOLON = 329,
     RACKLWSCOLON = 330,
     RSEQLWSCOLON = 331,
     ALERT_INFOLWSCOLON = 332,
     AUTHORIZATIONLWSCOLON = 333,
     IN_REPLY_TOLWSCOLON = 334,
     REPLY_TOLWSCOLON = 335,
     MAX_FORWARDSLWSCOLON = 336,
     REFER_TOLWSCOLON = 337,
     PRIORITYLWSCOLON = 338,
     PROXY_AUTHORIZATIONLWSCOLON = 339,
     PROXY_REQUIRELWSCOLON = 340,
     ROUTELWSCOLON = 341,
     ALLOWLWSCOLON = 342,
     PEARLYMEDIALWSCOLON = 343,
     CONTENT_DISPOSITIONLWSCOLON = 344,
     CONTENT_LANGUAGELWSCOLON = 345,
     EXPIRESLWSCOLON = 346,
     SESSIONEXPWSCOLON = 347,
     SESSIONIDLWSCOLON = 348,
     MINSELWSCOLON = 349,
     SOMELWSCOMMA = 350,
     PACCESSNETLWSCOLON = 351,
     REQDISPLWSCOLON = 352,
     PCHARGEADDRLWSCOLON = 353,
     PCHARGEVECTORLWSCOLON = 354,
     ANSWERMODELWSCOLON = 355,
     PRIVANSWERMODELWSCOLON = 356,
     ALERTMODELWSCOLON = 357,
     REFERSUBLWSCOLON = 358,
     PALERTINGMODELWSCOLON = 359,
     PANSWERSTATELWSCOLON = 360,
     ACCEPTCONTACTLWSCOLON = 361,
     REJECTCONTACTLWSCOLON = 362,
     PMEDIAAUTHLWSCOLON = 363,
     PATHLWSCOLON = 364,
     SECCLIENTLWSCOLON = 365,
     SECSERVERLWSCOLON = 366,
     SECVERIFYLWSCOLON = 367,
     PTARCEPTYIDLWSCOLON = 368,
     POSPSLWSCOLON = 369,
     PBILLINGINFOLWSCOLON = 370,
     PLAESLWSCOLON = 371,
     PDCSREDIRLWSCOLON = 372,
     PUSERDBASELWSCOLON = 373,
     SERVICEROUTELWSCOLON = 374,
     REFERREDBYTOLWSCOLON = 375,
     REPLACESLWSCOLON = 376,
     IFMATCHLWSCOLON = 377,
     ETAGLWSCOLON = 378,
     JOINLWSCOLON = 379,
     PSERVEDUSERLWSCOLON = 380,
     PPROFILEKEYLWSCOLON = 381,
     PSERVICEINDICATIONLWSCOLON = 382,
     PSERVICENOTIFICATIONLWSCOLON = 383,
     GEOROUTINGLWSCOLON = 384,
     TARGETDIALOGLWSCOLON = 385,
     INFOPACKAGELWSCOLON = 386,
     FEATURECAPSLWSCOLON = 387,
     RECVINFOLWSCOLON = 388,
     _TOKEN_NO_DOT = 389,
     _HEXTOKEN = 390,
     _DOT = 391,
     _SLASH = 392,
     SEMICOLON = 393,
     EQUALSIGN = 394,
     _ABO = 395,
     _ABC = 396,
     _COLON = 397,
     QUESTMARK = 398,
     AMPERSANT = 399,
     SOMELWS_SEMICOLON = 400,
     _AT = 401,
     _HOST = 402,
     SCHEME = 403,
     _HNAME = 404,
     _HVALUE = 405,
     _PORT = 406,
     SOMELWS_COMMA_SOMELWS = 407,
     SOMELWS_SLASH_SOMELWS = 408,
     PROTOCOL_NAME = 409,
     PROTOCOL_VERSION = 410,
     TRANSPORT = 411,
     _URLTOKEN = 412,
     PASSERTEDSERVICELWSCOLON = 413,
     PPREFERREDSERVICELWSCOLON = 414,
     XAUTLWSCOLON = 415,
     XCARRIERINFOLWSCOLON = 416,
     XCHGDELAYLWSCOLON = 417,
     PAREAINFOLWSCOLON = 418,
     XCHGINFOLWSCOLON = 419
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 119 "SIP_parse.y"

  unsigned int iv;
  char cv  ;
  char *sv ;
  SIPmsg__Types::Event__type *e_type;
  SIPmsg__Types::Event__template__list *e_template_list;
  SIPmsg__Types::Event__type__list *e_list;
  SIPmsg__Types::NameAddr *naddr;
  SIPmsg__Types::P__Assoc__uri__spec__list *u_spec_list;
  SIPmsg__Types::P__Assoc__uri__spec *u_spec;
  SIPmsg__Types::Network__spec *n_spec;
  SIPmsg__Types::Network__spec__list *n_spec_list;
  SIPmsg__Types::Access__net__spec *an_spec;
  SIPmsg__Types::Access__net__spec__list *an_spec_list;
  SIPmsg__Types::GenericParam__List *p_list;
  SIPmsg__Types::Contact__list *c_list;
  SIPmsg__Types::Request__disp__directive__list *d_list;
  SIPmsg__Types::Media__auth__token__list *a_t_list;
  SIPmsg__Types::RouteBody *route_val;
  SIPmsg__Types::RouteBody__List *route_list;
  SIPmsg__Types::Security__mechanism *sec_mech;
  SIPmsg__Types::Security__mechanism__list *sec_mech_list;
  SIPmsg__Types::HostPort *host_p;
  SIPmsg__Types::Diversion__params *divparam;
  SIPmsg__Types::Diversion__params__list *divparam_list;
  SIPmsg__Types::Hi__Entry *history_entry;
  SIPmsg__Types::Hi__Entry__list *history_entry_list;
  SIPmsg__Types::Em__param__List *em_bdy_list;
  SIPmsg__Types::Service__ID__List *service_1toN;
  SIPmsg__Types::Rvalue__List *r_val_list;
  SIPmsg__Types::Rvalue *rval;
  SIPmsg__Types::Location__value__list *g_val_list;
  SIPmsg__Types::Location__value *g_val;



/* Line 2068 of yacc.c  */
#line 251 "SIP_parse_.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE SIP_parse_lval;



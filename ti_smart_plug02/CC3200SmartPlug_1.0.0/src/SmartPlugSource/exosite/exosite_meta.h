/*****************************************************************************
*
*  exosite_meta.h - Meta informatio header
*  Copyright (C) 2012 Exosite LLC
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Exosite LLC nor the names of its contributors may
*    be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*****************************************************************************/

#ifndef EXOSITE_META_H
#define EXOSITE_META_H

// defines
#define META_SIZE                 256

#define META_CIK_SIZE             40
#define META_SERVER_SIZE          32
#define META_PORT_NUM_SIZE        2
#define META_EXO_STS_SIZE         2
#define META_UUID_SIZE            12
#define META_VNAME_SIZE           16
#define META_MNAME_SIZE           11
#define META_PAD1_SIZE            5
#define META_MARK_SIZE            8            //total 128 bytes
#define META_MFR_SIZE             128

typedef struct {
    char cik[META_CIK_SIZE];                   // our client interface key
    char server[META_SERVER_SIZE];             // domain name m2.exosite.com
    char portnum[META_PORT_NUM_SIZE];          // port mum 80 (non secure) or 443 (secure)
    char exo_status[META_EXO_STS_SIZE];        // exo status pad 2 bytes
    char uuid[META_UUID_SIZE];                 // UUID in ascii
    char vendorname[META_VNAME_SIZE];          // vendor name
    char modelname[META_MNAME_SIZE];           // model name
    char pad1[META_PAD1_SIZE];                 // pad 'modelname' to 16 bytes
    char mark[META_MARK_SIZE];                 // watermark
    char mfr[META_MFR_SIZE];                   // manufacturer data structure
} exosite_meta;

#define EXO_PORT_NONSEC           80           //non secure
#define EXO_PORT_SECURE           443          //secure

#define VENDOR_NAME               "texasinstruments"
#define MODEL_NAME                "smartplugv2"
#define EXO_SERVER_NAME           "m2.exosite.com"
#define EXO_DST_PORT              EXO_PORT_SECURE  //secure

#define EXO_SSL_CLNT_CA           "/cert/exositeca.der" /* Server CA file ID */

#define EXOMARK                   "exosite!"

#define EXO_STATUS_INIT_DONE              (UINT16)0x0001
#define EXO_STATUS_VALID_CONN             (UINT16)0x0002
#define EXO_STATUS_SECUR_CONN             (UINT16)0x0004
#define EXO_STATUS_VALID_ID               (UINT16)0x0008
#define EXO_STATUS_VALID_CIK              (UINT16)0x0010

#define EXO_STATUS_VALID_INIT             (EXO_STATUS_INIT_DONE|EXO_STATUS_VALID_ID|EXO_STATUS_VALID_CIK)

typedef enum
{
    META_CIK,
    META_SERVER,
    META_PORT_NUM,
    META_UUID,
    META_VENDOR,
    META_MODEL,
    META_MARK,
    META_MFR,
    META_NONE
} MetaElements;

// functions for export
int exosite_meta_defaults(const unsigned char if_nbr);
void exosite_meta_init(const unsigned char if_nbr, int reset);
void exosite_meta_write(unsigned char * write_buffer, unsigned short srcBytes, unsigned char element);
void exosite_meta_read(unsigned char * read_buffer, unsigned short destBytes, unsigned char element);

#endif



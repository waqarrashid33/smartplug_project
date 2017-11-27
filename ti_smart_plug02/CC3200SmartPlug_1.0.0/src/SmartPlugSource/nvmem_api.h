//*****************************************************************************
// File: nvmem_api.h
//
// Description: NVMEM API handling header file of Smartplug gen-1 application
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
//
// Author : dheeraj@ti.com
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

#ifndef __NVMEM_API_H__
#define __NVMEM_API_H__

#define SL_DEVICE_SFLASH        2
#define MAX_FILE_SIZE           64L*1024L
#define READ_SUCCESS            1
#define WRITE_SUCCESS           2
#define READ_FAIL               -1
#define WRITE_FAIL              -2
#define READ_NOT_OPENED         -3
#define WRITE_NOT_OPENED        -4
#define FILE_NOT_CLOSED         -5
#define MAX_FILE_SIZE_EXCEED    -6

#define SMARTPLUG_NVMEM_FILE_NAME     "mSmartPlugNvmemFile.txt"

extern int WriteSmartPlugNvmemFile( void );
extern int ReadSmartPlugNvmemFile( void );
extern int CreatSmartPlugNvmemFile( void );
extern int UpdateExositeCA(unsigned char *InBuff, unsigned short int length);
extern unsigned char ComputeCheckSum(unsigned char *buff, unsigned short int size);

#endif //  __MDNS_H__

//*****************************************************************************
// File: android_task.h
//
// Description: Android tasks handler header file of Smartplug gen-1 application
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

#ifndef __ANDROID_TASK_H__
#define __ANDROID_TASK_H__

#define APP_TCP_PORT            1204
#define RX_SIZE                 128

#define SL_SSL_SRV_KEY          "/cert/serverkey.der"   /* Server key file ID */
#define SL_SSL_SRV_CERT         "/cert/servercert.der"  /* Server certificate file ID */

#define MAX_ANDROID_RX_BUFF                    2048 //2KB buffer
#define SERVER_MAX_SETUP_RETRY_COUNT           3

typedef struct
{
  int           ServerSoc;
  int           g_ClientSD;
  OsiLockObj_t  g_ClientLockObj;

} s_ConnectionStatus;

typedef struct t_AndroidClientRxBuff
{
  UINT8               RxBuffer[MAX_ANDROID_RX_BUFF];
  UINT16              ReadIndex;
  UINT16              WriteIndex;
  UINT32              TimeOutStart;

} t_AndroidClientRxBuff;

extern int CreateTCPServerSocket(unsigned int uiPortNum);
extern int OpenTCPServerSocket(unsigned int uiPortNum);
extern void CloseTCPServerSocket(int iSockDesc);
extern int CreateTCPClientSocket(int iSockDesc);
extern void CloseTCPClientSocket(int iSockDesc);
extern int ClientSocketSend(long socket, char * buffer, unsigned int len);
extern void AndroidClientTask(void * param);
extern void AndroidClientRecvTask(void * param);

#endif   //__ANDROID_TASK_H__


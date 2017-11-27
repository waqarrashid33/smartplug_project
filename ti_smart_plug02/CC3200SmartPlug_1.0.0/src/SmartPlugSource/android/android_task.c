//*****************************************************************************
// File: android_task.c
//
// Description: Android tasks handler of Smartplug gen-1 application
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
#include "datatypes.h"
#include "simplelink.h"
#include "cc3200.h"
#include "nvmem_api.h"
#include <string.h>
#include "uart_logger.h"
#include "utils.h"
#include "exosite_task.h"
#include "exosite_meta.h"
#include "exosite_hal.h"
#include "metrology.h"
#include "smartplug_task.h"
#include "android_task.h"
#include "clienthandler.h"
#include "gpio_if.h"

/****************************************************************************/
/*        MACROS                    */
/****************************************************************************/

/****************************************************************************
                              Global variables
****************************************************************************/
s_ConnectionStatus  g_ConnectionState;
extern t_SmartPlugNvmmFile SmartPlugNvmmFile;
extern t_SmartplugErrorRecovery g_SmartplugErrorRecovery;
t_AndroidClientRxBuff   g_ClientRxbuff;
extern volatile LedIndicationName  g_SmartplugLedCurrentState;
extern volatile unsigned long g_MsCounter;
extern t_mDNSService mDNSService;
extern UINT16 g_CABinarySize;

#define DBG_PRINT               Report

/****************************************************************************
                              Synch Objects
****************************************************************************/
extern OsiLockObj_t  g_NvmemLockObj;
OsiMsgQ_t     AndroidClientTaskMsgQ;

//****************************************************************************
//
//! Creates a TCP socket and binds to it
//!
//! \param uiPortNum is the port number to bind to
//!
//! This function
//!    1. Creates a TCP socket and binds to it
//!
//! \return Soceket Descriptor, < 1 if error.
//
//****************************************************************************
int OpenTCPServerSocket(unsigned int uiPortNum)
{
  int iSockDesc, iRetVal;
  sockaddr_in sServerAddress;
  SlSockNonblocking_t enableOption;
  enableOption.NonblockingEnabled = 1;

  /* this is possible only in error case */
  if(g_ConnectionState.ServerSoc >= 0)
  {
    return -1;
  }

  //
  // opens a secure socket
  //
  iSockDesc = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, SL_SEC_SOCKET);
  if( iSockDesc < 0 )
  {
     return -1;
  }

  //non blocking socket - Enable nonblocking mode
  iRetVal = sl_SetSockOpt(iSockDesc,SOL_SOCKET,SL_SO_NONBLOCKING, &enableOption,sizeof(enableOption));
  if(iRetVal < 0)
  {
    CloseTCPServerSocket(iSockDesc);
    return -1;
  }

  iRetVal = sl_SetSockOpt(iSockDesc, SL_SOL_SOCKET, SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME,SL_SSL_SRV_KEY, strlen(SL_SSL_SRV_KEY));
  if( iRetVal < 0 )
  {
      CloseTCPServerSocket(iSockDesc);
      return -1;
  }
  iRetVal = sl_SetSockOpt(iSockDesc, SL_SOL_SOCKET, SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME,SL_SSL_SRV_CERT, strlen(SL_SSL_SRV_CERT));
  if( iRetVal < 0 )
  {
      CloseTCPServerSocket(iSockDesc);
      return -1;
  }

  //
  // Bind - Assign a port to the socket
  //
  sServerAddress.sin_family = AF_INET;
  sServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  sServerAddress.sin_port = htons(uiPortNum);
  if ( bind(iSockDesc, (struct sockaddr*)&sServerAddress, sizeof(sServerAddress)) != 0 )
  {
    CloseTCPServerSocket(iSockDesc);
    return -1;
  }

  return iSockDesc;
}

int CreateTCPServerSocket(unsigned int uiPortNum)
{
  int iSockDesc = -1;
  unsigned char connectRetries = 0;

  if(g_ConnectionState.ServerSoc >= 0)
  {
    return g_ConnectionState.ServerSoc;
  }

  while (connectRetries++ < SERVER_MAX_SETUP_RETRY_COUNT)
  {
    iSockDesc = OpenTCPServerSocket(uiPortNum);

    if (iSockDesc < 0)
    {
      continue;
    }

    if(listen(iSockDesc, 2) != 0) // 1 client connection (1 buffer)
    {
      CloseTCPServerSocket(iSockDesc);
      iSockDesc = -1;
      OSI_DELAY(100);//wait 100ms
      continue;
    }
    else
    {
      connectRetries = 0;
      break;
    }
  }

  if(iSockDesc < 0)
  {
    UnsetCC3200MachineState(CC3200_GOOD_TCP|CC3200_GOOD_INTERNET);// bad TCP
  }
  else
  {
    g_ConnectionState.ServerSoc = iSockDesc;
    SetCC3200MachineState(CC3200_GOOD_TCP);
  }

  return iSockDesc;
}

void CloseTCPServerSocket(int iSockDesc)
{
  int ittr = 0;

  if(iSockDesc < 0)
  {
    return;
  }

  g_ConnectionState.ServerSoc = iSockDesc;
  do
  {
    if(sl_Close(iSockDesc) >= 0)
    {
      g_ConnectionState.ServerSoc = -1;
      iSockDesc = -1;
      Report("Android server socket closed\n\r");
      break;
    }
    else
    {
      Report("\n Android client socket close error\n\r");
      OSI_DELAY(200);//wait 200ms
    }
    ittr++;
  }while(ittr < 3);

  if(iSockDesc >= 0)
  {
    UnsetCC3200MachineState(CC3200_GOOD_TCP|CC3200_GOOD_INTERNET);//NWP not responding
  }
}

int CreateTCPClientSocket(int iSockDesc)
{
  sockaddr sClientAddr;
  socklen_t uiClientAddrLen = sizeof(sClientAddr);
  int sock = -1;
  SlTimeval_t timeVal;
  SlSockNonblocking_t enableOption;

  sock = accept(iSockDesc, &sClientAddr, &uiClientAddrLen);
  if(sock >= 0)
  {
    enableOption.NonblockingEnabled = 0;
    //Blocking socket - Enable blocking mode
    if(sl_SetSockOpt(iSockDesc,SOL_SOCKET,SL_SO_NONBLOCKING, &enableOption,sizeof(enableOption)) < 0)
    {
      CloseTCPClientSocket(sock);
      return -1;
    }

    enableOption.NonblockingEnabled = 0;
    //Blocking socket - Enable blocking mode
    if(sl_SetSockOpt(sock,SOL_SOCKET,SL_SO_NONBLOCKING, &enableOption,sizeof(enableOption)) < 0)
    {
      CloseTCPClientSocket(sock);
      return -1;
    }

    timeVal.tv_sec =  5;             // 5 Seconds
    timeVal.tv_usec = 0;             // Microseconds. 10000 microseconds resoultion
    if((sl_SetSockOpt(sock,SOL_SOCKET,SL_SO_RCVTIMEO, &timeVal, sizeof(timeVal))) < 0)
    {
      CloseTCPClientSocket(sock);
      return -1;
    }// Enable receive timeout

    g_ConnectionState.g_ClientSD = sock;
  }

  return sock;
}

void CloseTCPClientSocket(int iSockDesc)
{
  SlSockNonblocking_t enableOption;
  enableOption.NonblockingEnabled = 1;
  int ittr = 0;

  if(iSockDesc < 0)
  {
    return;
  }

  g_ConnectionState.g_ClientSD = iSockDesc;
  do
  {
    if(sl_Close(iSockDesc) >= 0)
    {
      g_ConnectionState.g_ClientSD = -1;
      iSockDesc = -1;
      Report("\n Android client socket closed\n\r");

      if(g_ConnectionState.ServerSoc >= 0)
      {
        //non Blocking server socket - Enable non blocking mode
        if(sl_SetSockOpt(g_ConnectionState.ServerSoc,SOL_SOCKET,SL_SO_NONBLOCKING, &enableOption,sizeof(enableOption)) < 0)
        {
          CloseTCPServerSocket(g_ConnectionState.ServerSoc);
        }
      }
      break;
    }
    else
    {
      Report("\n Android client socket close error\n\r");
      OSI_DELAY(200);//wait 200ms
    }
    ittr++;
  }while(ittr < 3);

  if(iSockDesc >= 0)
  {
    UnsetCC3200MachineState(CC3200_GOOD_TCP|CC3200_GOOD_INTERNET);//NWP not responding
  }
}

int ClientSocketSend(long socket, char * buffer, unsigned int len)
{
  int send_len = 0, Ittr = 0;
  LedIndicationName  LedCurrentState = NO_LED_IND;

  do
  {
    if((CLOUD_DATA_TRANSFER_IND != g_SmartplugLedCurrentState) &&\
        (DEVICE_ERROR_IND != g_SmartplugLedCurrentState))
    {
      AndroidDataTranLEDInd();
      LedCurrentState = g_SmartplugLedCurrentState;
      g_SmartplugLedCurrentState = ANDROID_DATA_TRANSFER_IND;
    }

    send_len = (int)send((int)socket, buffer, (int)len, 0);

    if((ANDROID_DATA_TRANSFER_IND == g_SmartplugLedCurrentState) &&\
        (NO_LED_IND != LedCurrentState))
    {
      g_SmartplugLedCurrentState = LedCurrentState;
    }

    if(send_len > 0)
    {
      if(len != send_len)
      {
        Report("client Send length is not matching %d \n\r", send_len);
        send_len = -1;
      }
      return send_len;
    }
    else if(send_len != SL_EAGAIN)
    {
      Report("\n client socket send error %d\n\r", send_len);
      return -1;
    }

    Ittr++;

  }  while((SL_EAGAIN == send_len) && (Ittr < 3));

  Report("\n client send time out %d\n\r", send_len);

  return -1;
}

void AndroidClientTask(void * param)
{
  t_SmartPlugSendData SmartPlugSendData;
  UINT32 MetrologyCount,TimeStamp, DataSender;

  while(1)
  {
    osi_MsgQRead(&AndroidClientTaskMsgQ,&SmartPlugSendData,OSI_WAIT_FOREVER);

    osi_LockObjLock(&g_ConnectionState.g_ClientLockObj, SL_OS_WAIT_FOREVER);
    g_SmartplugErrorRecovery.AndroidTxThreadError = 0;//clear counter

    memcpy((void *)&TimeStamp,(const void *)&SmartPlugSendData.SmartPlugMetrologyData.UpdateTime[0],4);

    if(g_ConnectionState.g_ClientSD >= 0)
    {
      memcpy((void *)&DataSender,(const void *)&SmartPlugSendData.DataSender,4);
      memcpy((void *)&MetrologyCount,(const void *)&SmartPlugSendData.SmartPlugMetrologyData.MetrologyCount[0],4);

      DBG_PRINT("Android: Time=%d,Metrolgy=%d, data=%x\n\r",TimeStamp,MetrologyCount,DataSender);

      // Code block to send device status update to android
      if(DataSender & SEND_DEVICE_STATUS)
      {
        SendDeviceStatus(&g_ConnectionState, &SmartPlugSendData);
      }
      if(DataSender & (SEND_THRESHOLD_PW_WARNING|SEND_THRESHOLD_EN_WARNING|SEND_METROLOGY_WARNING))
      {
        sendWarning(&g_ConnectionState, &SmartPlugSendData);
      }
      if(DataSender & SEND_EXO_SSL_CA_DONE)
      {
        sendCloudCAUpdateSuccess(&g_ConnectionState, &SmartPlugSendData);
      }
      if(DataSender & SEND_METROLOGY_DATA)
      {
        SendMetrologyData(&g_ConnectionState, &SmartPlugSendData);
      }
      if(DataSender & SEND_AVERAGE_POWER)
      {
        SendAvrgEnergy(&g_ConnectionState, &SmartPlugSendData);
      }
      if(DataSender & SEND_SCHEDULE_TABLE)
      {
        sendScheduleTable(&g_ConnectionState, &SmartPlugSendData);
      }
      if(DataSender & SEND_24HR_ENERGY)
      {
        send24HrAvgEnergy(&g_ConnectionState, &SmartPlugSendData);
      }
      if(DataSender & SEND_EXOSITE_CFG)
      {
        sendCloudInfo(&g_ConnectionState, &SmartPlugSendData);
      }
      if(DataSender & SEND_METROLOGY_CFG)
      {
        sendMetrologyCalInfo(&g_ConnectionState, &SmartPlugSendData);
      }
    }
    else
    {
      #if 0 //DKS not required
      if(TimeStamp >= (mDNSService.mDNSBroadcastTime + mDNSService.mDNSBroadcastUpdateInterval))
      {
        int status;
        status = mDNSBroadcast(1);
        if(status != 0)
        {
          g_SmartplugErrorRecovery.MdnsError++;
        }
        else
        {
          g_SmartplugErrorRecovery.MdnsError = 0;
        }
        mDNSService.mDNSBroadcastTime = TimeStamp;
      }
      #endif
    }
    osi_LockObjUnlock(&g_ConnectionState.g_ClientLockObj);
    g_SmartplugErrorRecovery.AndroidTxThreadError = 0;//clear counter
  }
}

void AndroidClientRecvTask(void * param)
{
  int iRecvLen = 0, sock = -1, index = 0;

  while(1)
  {
    if(GetCC3200State() & CC3200_GOOD_TCP)
    {
      if(g_ConnectionState.ServerSoc < 0)
      {
        CreateTCPServerSocket(APP_TCP_PORT);
        Report("Android server socket %d \n\r", g_ConnectionState.ServerSoc);
      }

      if((g_ConnectionState.ServerSoc >= 0) && (GetCC3200State() & CC3200_GOOD_TCP))
      {
        if(g_ConnectionState.g_ClientSD < 0)
        {
          sock = CreateTCPClientSocket(g_ConnectionState.ServerSoc);//non blocking func
          Report("Android created client socket is %d\n\r",sock); //WAQAR
          if(sock >= 0 )
          {
            if(g_SmartplugLedCurrentState == CONNECTED_TO_AP_IND)
            {
              g_SmartplugLedCurrentState = CONNECTED_ANDROID_IND;
            }
            Report("\n Android Client connected\n\r");

            osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
            SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_DATA_SEND_DEV_CFG|NVMEM_DATA_SEND_EXO_CFG|NVMEM_DATA_SEND_SCHEDULE_TABLE);// sync
            SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToAndrid = 1;
            SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToAndrid = 1;
            osi_LockObjUnlock(&g_NvmemLockObj);
            g_ClientRxbuff.WriteIndex = 0;
            g_ClientRxbuff.ReadIndex = 0;
            g_CABinarySize = 0xFFFF;
          }
          else if((sock != SL_EAGAIN) && (sock != SL_POOL_IS_EMPTY) && (sock != SL_INEXE) && (sock != SL_ENSOCK))
          {
            Report("\n Android server socket error %d\n\r",sock); //Error
            CloseTCPServerSocket(g_ConnectionState.ServerSoc);
          }
          else
          {
            Report("Android accept retry %d\n\r",sock);
            /* wait 500ms */
            OSI_DELAY(500);//wait 500ms
          }
        }

        if((g_ConnectionState.g_ClientSD >= 0) && (GetCC3200State() & CC3200_GOOD_TCP))
        {
          unsigned char RxBuffer[RX_SIZE+4] = {0xFF};
          LedIndicationName  LedCurrentState = NO_LED_IND;

          iRecvLen = (int)recv(g_ConnectionState.g_ClientSD, &RxBuffer[0], RX_SIZE, 0);//blocking func with timeout 5sec

          if(iRecvLen > 0)
          {
            int length;

            if((CLOUD_DATA_TRANSFER_IND != g_SmartplugLedCurrentState) &&\
                (DEVICE_ERROR_IND != g_SmartplugLedCurrentState))
            {
              AndroidDataTranLEDInd();
              LedCurrentState = g_SmartplugLedCurrentState;
              g_SmartplugLedCurrentState = ANDROID_DATA_TRANSFER_IND;
            }

            Report("\nAndroid Client Data received %d\n\r",iRecvLen);

            length = (int)g_ClientRxbuff.WriteIndex - (int)g_ClientRxbuff.ReadIndex;
            if(length < 0)
            {
              length += MAX_ANDROID_RX_BUFF;
            }

            if(length != 0)
            {
              /* Have 900ms time out for buffered data */
              if((g_MsCounter - g_ClientRxbuff.TimeOutStart) >= 9)
              {
                g_ClientRxbuff.WriteIndex = 0;
                g_ClientRxbuff.ReadIndex = 0;
                g_CABinarySize = 0xFFFF;
              }
            }

            for(index = 0; index < iRecvLen; index++)
            {
              g_ClientRxbuff.RxBuffer[g_ClientRxbuff.WriteIndex++] = RxBuffer[index];
              if(g_ClientRxbuff.WriteIndex >= MAX_ANDROID_RX_BUFF)
              {
                g_ClientRxbuff.WriteIndex = 0;
              }
            }

            ProcessData(&g_ClientRxbuff.RxBuffer[0]);

            if((ANDROID_DATA_TRANSFER_IND == g_SmartplugLedCurrentState) &&\
                (NO_LED_IND != LedCurrentState))
            {
              g_SmartplugLedCurrentState = LedCurrentState;
            }
          }
          else if((iRecvLen != SL_EAGAIN) && (iRecvLen != SL_POOL_IS_EMPTY))
          {
            osi_LockObjLock(&g_ConnectionState.g_ClientLockObj, SL_OS_WAIT_FOREVER);
            if(g_SmartplugLedCurrentState == CONNECTED_ANDROID_IND)
            {
              g_SmartplugLedCurrentState = CONNECTED_TO_AP_IND;
            }
            Report("\nAndroid Client close %d\n\r", iRecvLen);
            CloseTCPClientSocket(g_ConnectionState.g_ClientSD);

            osi_LockObjUnlock(&g_ConnectionState.g_ClientLockObj);
          }
          else
          {
            Report("Android receive retry %d\n\r",iRecvLen);
          }
        }
        else if(g_ConnectionState.g_ClientSD < 0)
        {
          /* Clear flag if client not connected */
          osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
          SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToAndrid = 0;
          SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToAndrid = 0;
          osi_LockObjUnlock(&g_NvmemLockObj);
          g_ClientRxbuff.WriteIndex = 0;
          g_ClientRxbuff.ReadIndex = 0;
          g_CABinarySize = 0xFFFF;
        }
      }
    }

    if(!(GetCC3200State() & CC3200_GOOD_TCP))
    {
      osi_LockObjLock(&g_ConnectionState.g_ClientLockObj, SL_OS_WAIT_FOREVER);
      if(g_ConnectionState.g_ClientSD >= 0)
      {
        CloseTCPClientSocket(g_ConnectionState.g_ClientSD);
      }

      if(g_ConnectionState.ServerSoc >= 0)
      {
        CloseTCPServerSocket(g_ConnectionState.ServerSoc);
      }
      osi_LockObjUnlock(&g_ConnectionState.g_ClientLockObj);

      g_ClientRxbuff.WriteIndex = 0;
      g_ClientRxbuff.ReadIndex = 0;
      g_CABinarySize = 0xFFFF;

      OSI_DELAY(1000);//wait 1sec
    }

    g_SmartplugErrorRecovery.AndroidRxThreadError = 0;//clear counter
  }
}


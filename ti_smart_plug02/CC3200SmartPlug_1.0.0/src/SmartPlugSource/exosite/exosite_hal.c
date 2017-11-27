/*****************************************************************************
*
*  exosite_hal.c - Exosite hardware & environmenat adapation layer.
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
#include "simplelink.h"
#include "datatypes.h"
#include "exosite_hal.h"
#include "exosite_meta.h"
#include "metrology.h"
#include "smartplug_task.h"
#include "exosite.h"
#include <stdlib.h>
#include "socket.h"
#include "hw_types.h"
#include "cc3200.h"
#include <netcfg.h>
#include "nvmem_api.h"
#include <uart_logger.h>
#include "gpio_if.h"

// local variables

static SlSockAddrIn_t exo_SocketAddr;
unsigned char g_MACAddress[6];

extern s_ExoConnectionStatus ExositeConStatus;
extern volatile LedIndicationName  g_SmartplugLedCurrentState;

// local functions

// externs

// global variables

/*****************************************************************************
*
*  exoHAL_ReadUUID
*
*  \param  if_nbr - Interface Number (1 - WiFi)
*          UUID_buf - buffer to return hexadecimal MAC
*
*  \return 0 if failure; len of UUID if success;
*
*  \brief  Reads the MAC address from the hardware
*
*****************************************************************************/
int
exoHAL_ReadUUID(unsigned char if_nbr, char * UUID_buf)
{
  int valuelen = 0;
  int i = 0, j = 0;
  char conv = 0;

  switch (if_nbr) {
    case IF_GPRS:
      break;
    case IF_ENET:
      break;
    case IF_WIFI:
      CC3200_MAC_ADDR_GET(g_MACAddress);
      Report("MAC Address : %02x:%02x:%02x:%02x:%02x:%02x\r\n", g_MACAddress[0], g_MACAddress[1],\
              g_MACAddress[2], g_MACAddress[3], g_MACAddress[4], g_MACAddress[5]);
      for (i = 0, j = 0; i < 6; i++, j += 2)
      {
        conv = (g_MACAddress[i] & 0xf0) >> 4;
        itoa(conv, &UUID_buf[j], 16);
        conv = g_MACAddress[i] & 0xf;
        itoa(conv, &UUID_buf[j + 1], 16);
      }

      valuelen = j;
      UUID_buf[valuelen] = 0;
      break;
    default:
      break;
  }

  return valuelen;
}


/*****************************************************************************
*
* exoHAL_EnableNVMeta
*
*  \param  None
*
*  \return None
*
*  \brief  Enables meta non-volatile memory, if any
*
*****************************************************************************/
void
exoHAL_EnableMeta(void)
{
  return;
}


/*****************************************************************************
*
*  exoHAL_EraseNVMeta
*
*  \param  None
*
*  \return None
*
*  \brief  Wipes out meta information - replaces with 0's
*
*****************************************************************************/
void
exoHAL_EraseMeta(void)
{
  return;
}

/*****************************************************************************
*
*  exoHAL_SocketClose
*
*  \param  socket - socket handle
*
*  \return None
*
*  \brief  Closes a socket
*
*****************************************************************************/
void
exoHAL_SocketClose(long socket)
{
  int ittr = 0;

  if(socket < 0)
  {
    return;
  }

  ExositeConStatus.g_ClientSoc = socket;
  do
  {
    if(sl_Close(socket) >= 0)
    {
      ExositeConStatus.g_ClientSoc = -1;
      socket = -1;
      break;
    }
    else
    {
      Report("\n exosite socket close error\n\r");
      OSI_DELAY(200);//wait 200ms
    }
    ittr++;
  }while(ittr < 3);

  if(socket >= 0)
  {
    UnsetCC3200MachineState(CC3200_GOOD_TCP|CC3200_GOOD_INTERNET);//NWP not responding
  }
  return;
}


/*****************************************************************************
*
*  exoHAL_SocketOpenTCP
*
*  \param  server - server's ip
*
*  \return -1: failure; Other: socket handle
*
*  \brief  Opens a TCP socket
*
*****************************************************************************/
long
exoHAL_SocketOpenTCP(unsigned short int portNum)
{
  long sock = -1, iRetVal = -1;
  SlTimeval_t timeVal;
  //unsigned int uiIP;

  /* this is possible only in error case */
  if(ExositeConStatus.g_ClientSoc >= 0)
  {
    return -1;
  }

  //
  // opens a secure socket
  //
  if(EXO_PORT_SECURE == portNum)
  {
    sock = (long)socket(AF_INET, SOCK_STREAM, SL_SEC_SOCKET);
    Report("Secured Exosite connection\n\r");
  }
  else
  {
    sock = (long)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    Report("Non Secured Exosite connection\n\r");
  }

  if (sock < 0)
  {
    return -1;
  }

  timeVal.tv_sec =  5;             // 5 Seconds
  timeVal.tv_usec = 0;             // Microseconds. 10000 microseconds resoultion
  if((sl_SetSockOpt(sock,SOL_SOCKET,SL_SO_RCVTIMEO, &timeVal, sizeof(timeVal))) < 0)
  {
    exoHAL_SocketClose(sock);
    return -1;
  }// Enable receive timeout

  if(EXO_PORT_SECURE == portNum)
  {
    iRetVal = sl_SetSockOpt(sock, SL_SOL_SOCKET, SL_SO_SECURE_FILES_CA_FILE_NAME,EXO_SSL_CLNT_CA, strlen(EXO_SSL_CLNT_CA));
    if( iRetVal < 0 )
    {
        exoHAL_SocketClose(sock);
        return -1;
    }
  }

  return (long)sock;
}

/*****************************************************************************
*
*  exoHAL_ServerAddressGet
*
*  \param  None
*
*  \return
*
*  \brief  The function resolves IP address from domain name
*
*****************************************************************************/
long
exoHAL_ServerAddressGet(char *server, unsigned short int portNum)
{
  unsigned int uiIP;
  int iRetVal = -1;

  Report("Exosite name %s, %d \n\r",server, portNum);

  /* Get IP address of m2.exosite.com */
  iRetVal = sl_NetAppDnsGetHostByName((signed char*)server, strlen(server),
                                    (unsigned long*)&uiIP, SL_AF_INET);
  if( iRetVal < 0 )
  {
      return -1;
  }

  Report("Exosite IP address %x \n\r", uiIP);

  exo_SocketAddr.sin_family = SL_AF_INET;
  exo_SocketAddr.sin_port = sl_Htons(portNum);
  exo_SocketAddr.sin_addr.s_addr = sl_Htonl(uiIP);

  return 0;
}

/*****************************************************************************
*
*  exoHAL_ServerConnect
*
*  \param  None
*
*  \return socket - socket handle
*
*  \brief  The function opens a TCP socket
*
*****************************************************************************/
long
exoHAL_ServerConnect(long sock)
{
  if (connect((int)sock, (struct sockaddr*)&exo_SocketAddr, sizeof(exo_SocketAddr)) != 0)
    sock = -1;

  return (long)sock;
}

/*****************************************************************************
*
*  exoHAL_SocketSend
*
*  \param  socket - socket handle; buffer - string buffer containing info to
*          send; len - size of string in bytes;
*
*  \return Number of bytes sent
*
*  \brief  Sends data out to the internet
*
*****************************************************************************/
UINT32 iCnt = 0;
int
exoHAL_SocketSend(long socket, char * buffer, unsigned int len)
{
  int send_len = 0, Ittr = 0;
  LedIndicationName  LedCurrentState = NO_LED_IND;

  do
  {
    iCnt = 1;
    if(DEVICE_ERROR_IND != g_SmartplugLedCurrentState)
    {
      if(ANDROID_DATA_TRANSFER_IND == g_SmartplugLedCurrentState)
      {
        LedCurrentState = CONNECTED_CLOUD_IND;
      }
      else
      {
        LedCurrentState = g_SmartplugLedCurrentState;
      }
      g_SmartplugLedCurrentState = CLOUD_DATA_TRANSFER_IND;
      ToggleLEDIndication();
    }

    send_len = (int)send((int)socket, buffer, (int)len, 0);

    if((CLOUD_DATA_TRANSFER_IND == g_SmartplugLedCurrentState) &&\
        (NO_LED_IND != LedCurrentState))
    {
      g_SmartplugLedCurrentState = LedCurrentState;
    }

    iCnt = 2;

    if(send_len > 0)
    {
      if(len != send_len)
      {
        Report("Send length is not matching %d \n\r", send_len);
        send_len = -1;
      }
      return send_len;
    }
    else if(send_len != SL_EAGAIN)
    {
      Report("\n Exosite socket send error %d\n\r", send_len);
      exoHAL_SocketClose(socket);
      return -1;
    }

    Ittr++;

  }  while((SL_EAGAIN == send_len) && (Ittr < 3));

  Report("\n send time out %d\n\r", send_len);
  Report("\n Exosite socket closed \n\r");
  exoHAL_SocketClose(socket);

  return -1;
}


/*****************************************************************************
*
*  exoHAL_SocketRecv
*
*  \param  socket - socket handle; buffer - string buffer to put info we
*          receive; len - size of buffer in bytes;
*
*  \return Number of bytes received
*
*  \brief  Receives data from the internet
*
*****************************************************************************/
int
exoHAL_SocketRecv(long socket, char * buffer, unsigned int len)
{
  int recv_len = 0, Ittr = 0;
  LedIndicationName  LedCurrentState = NO_LED_IND;

  do
  {
    iCnt = 3;
    if(DEVICE_ERROR_IND != g_SmartplugLedCurrentState)
    {
      if(ANDROID_DATA_TRANSFER_IND == g_SmartplugLedCurrentState)
      {
        LedCurrentState = CONNECTED_CLOUD_IND;
      }
      else
      {
        LedCurrentState = g_SmartplugLedCurrentState;
      }
      g_SmartplugLedCurrentState = CLOUD_DATA_TRANSFER_IND;
      ToggleLEDIndication();
    }

    recv_len = (int)recv((int)socket, buffer, (int)len, 0);

    if((CLOUD_DATA_TRANSFER_IND == g_SmartplugLedCurrentState) &&\
        (NO_LED_IND != LedCurrentState))
    {
      g_SmartplugLedCurrentState = LedCurrentState;
    }

    iCnt = 4;

    if(recv_len > 0)
    {
      return recv_len;
    }
    else if((recv_len != SL_EAGAIN) && (recv_len != SL_POOL_IS_EMPTY))
    {
      Report("\n Exosite socket closed %d\n\r", recv_len);
      exoHAL_SocketClose(socket);
      return -1;
    }
    Ittr++;

  }  while(((SL_EAGAIN == recv_len) ||(SL_POOL_IS_EMPTY == recv_len)) && (Ittr < 3));

  Report("\n read time out %d\n\r", recv_len);
  Report("\n Exosite socket closed \n\r");
  exoHAL_SocketClose(socket);

  return -1;
}


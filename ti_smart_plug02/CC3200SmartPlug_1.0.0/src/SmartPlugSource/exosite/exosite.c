/*****************************************************************************
*
*  exosite.c - Exosite cloud communications.
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
#include <stdio.h>
#include "stdlib.h"
#include "user.h"
#include "datatypes.h"
#include "exosite_hal.h"
#include "exosite_meta.h"
#include "metrology.h"
#include "smartplug_task.h"
#include "android_task.h"
#include "exosite.h"
#include <string.h>
#include "simplelink.h"
#include <uart_logger.h>
#include "clienthandler.h"
#include "utils.h"
#include "cc3200.h"
#include "gpio_if.h"

extern OsiMsgQ_t SmartPlugMsgQ;
extern s_ConnectionStatus g_ConnectionState;

//local defines
#define EXOSITE_MAX_CONNECT_RETRY_COUNT        3
#define EXOSITE_LENGTH                         72
#define RX_SIZE                                128
#define EXO_RX_SIZE                            512
static char exosite_provision_info[EXOSITE_LENGTH];
static char ContentBody[EXO_RX_SIZE];
enum lineTypes
{
  CIK_LINE,
  HOST_LINE,
  CONTENT_LINE,
  ACCEPT_LINE,
  LENGTH_LINE,
  GETDATA_LINE,
  POSTDATA_LINE,
  GETDATA_GEN_LINE,
  EMPTY_LINE
};

#define DBG_PRINT   Report

#define STR_CIK_HEADER "X-Exosite-CIK: "
#define STR_CONTENT_LENGTH "Content-Length: "
#define STR_GET_URL "GET /onep:v1/stack/alias?"
#define STR_GET_GEN_URL "GET /"
#define STR_HTTP "  HTTP/1.1\r\n"
#define STR_HOST "Host: m2.exosite.com\r\n"
#define STR_ACCEPT "Accept: application/x-www-form-urlencoded; charset=utf-8\r\n"
#define STR_CONTENT "Content-Type: application/x-www-form-urlencoded; charset=utf-8\r\n"
#define STR_VENDOR "vendor="
#define STR_MODEL "model="
#define STR_SN "sn="
#define STR_CRLF "\r\n"

// local functions

// externs
extern t_SmartPlugNvmmFile SmartPlugNvmmFile;
extern OsiLockObj_t        g_NvmemLockObj;

// global variables
static int status_code = EXO_STATUS_END;
extern s_ExoConnectionStatus ExositeConStatus;
extern volatile LedIndicationName  g_SmartplugLedCurrentState;

/*****************************************************************************
*
* info_assemble
*
*  \param  char * vendor, custom's vendor name
*          char * model, custom's model name
*
*  \return string length of assembly customize's vendor information
*
*  \brief  Initializes the customer's vendor and model name for
*          provisioning
*
*****************************************************************************/
int
info_assemble(const char * vendor, const char *model, const char *sn)
{
  int info_len = 0;
  int assemble_len = 0;
  char * vendor_info = exosite_provision_info;

  // verify the assembly length
  assemble_len = strlen(STR_VENDOR) + strlen(vendor)
                 + strlen(STR_MODEL) + strlen(model)
                 + strlen(STR_SN) + strlen(sn) + 5;
  if (assemble_len > EXOSITE_LENGTH)
    return info_len;

  // vendor=
  memcpy(vendor_info, STR_VENDOR, strlen(STR_VENDOR));
  info_len = strlen(STR_VENDOR);

  // vendor="custom's vendor"
  memcpy(&vendor_info[info_len], vendor, strlen(vendor));
  info_len += strlen(vendor);

  // vendor="custom's vendor"&
  vendor_info[info_len] = '&'; // &
  info_len += 1;

  // vendor="custom's vendor"&model=
  memcpy(&vendor_info[info_len], STR_MODEL, strlen(STR_MODEL));
  info_len += strlen(STR_MODEL);

  // vendor="custom's vendor"&model="custom's model"
  memcpy(&vendor_info[info_len], model, strlen(model));
  info_len += strlen(model);

  // vendor="custom's vendor"&model="custom's model"&
  vendor_info[info_len] = '&'; // &
  info_len += 1;

  // vendor="custom's vendor"&model="custom's model"&sn=
  memcpy(&vendor_info[info_len], STR_SN, strlen(STR_SN));
  info_len += strlen(STR_SN);

  // vendor="custom's vendor"&model="custom's model"&sn="device's sn"
  memcpy(&vendor_info[info_len], sn, strlen(sn));
  info_len += strlen(sn);

  vendor_info[info_len] = 0;

  return info_len;
}

/*****************************************************************************
*
* Exosite_StatusCode
*
*  \param  None
*
*  \return 1 success; 0 failure
*
*  \brief  Provides feedback from Exosite status codes
*
*****************************************************************************/
int
Exosite_StatusCode(void)
{
  return status_code;
}

/*****************************************************************************
*
* Exosite_Init
*
*  \param  char * vendor - vendor name
*          char * model  - model name
*          char if_nbr   - network interface
*          int reset     - reset the settings to Exosite default
*
*  \return 1 success; 0 failure
*
*  \brief  Initializes the Exosite meta structure, UUID and
*          provision information
*
*****************************************************************************/
int
Exosite_Init(const unsigned char if_nbr, int reset)
{
  unsigned char struuid[META_UUID_SIZE+1],
                vendor[META_VNAME_SIZE+1],
                model[META_MNAME_SIZE+1];
  UINT16 status;

  exosite_meta_init(if_nbr, reset);  //always initialize Exosite meta structure

  // read vendor, model & UUID into 'sn'
  exosite_meta_read((unsigned char *)vendor, META_VNAME_SIZE, META_VENDOR);  //read vendor name
  vendor[META_VNAME_SIZE] = 0;

  exosite_meta_read((unsigned char *)model,  META_MNAME_SIZE, META_MODEL);   //read model name
  model[META_MNAME_SIZE] = 0;

  exosite_meta_read((unsigned char *)struuid, META_UUID_SIZE, META_UUID);    //read UUID (mac add)
  struuid[META_UUID_SIZE] = 0;

  info_assemble((const char *)vendor, (const char *)model, (const char *)struuid);

  if(reset)
  {
    status_code = EXO_STATUS_INIT;
    status = EXO_STATUS_INIT_DONE;
    memcpy((void *)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0], (void *)&status, META_EXO_STS_SIZE);
  }
  else
  {
    status_code = EXO_STATUS_OK;
    status = EXO_STATUS_VALID_INIT;
    memcpy((void *)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0], (void *)&status, META_EXO_STS_SIZE);
  }

  DBG_PRINT("Exosite initialiased\n\r");

  return 1;
}

char TestCharBuff[300];
/*****************************************************************************
*
* Exosite_Activate
*
*  \param  None
*
*  \return 1  - activation success
*          0  - activation failure
*
*  \brief  Called after Init has been run in the past, but maybe comms were
*          down and we have to keep trying
*
*****************************************************************************/
int
Exosite_Activate(void)
{
  int length, TstrLen = 0;
  char strLens[16];
  char *cmp_ss = "Content-Length: ";
  //char *cmp_dt = "Date: ";
  char *cmp = cmp_ss;
  //char *cmpdt = cmp_dt;

  int newcik = 0;
  int http_status = 0;
  char strBuf[RX_SIZE + 8];
  int strLen;
  int send_status = -1;

  if(!(GetCC3200State() & CC3200_GOOD_INTERNET))
  {
    return 0;
  }

  long sock = connect_to_exosite(EXO_DEFAULT_CON);
  if (sock < 0) {
    return 0;
  }

  // Get activation Serial Number
  length = strlen(exosite_provision_info);
  itoa(length, strLens, 10); //make a string for length

  send_status = sendLine(sock, POSTDATA_LINE, "/provision/activate");
  if(send_status < 0)
  {
    return 0;
  }

  send_status = sendLine(sock, HOST_LINE, NULL);
  if(send_status < 0)
  {
    return 0;
  }

  send_status = sendLine(sock, CONTENT_LINE, NULL);
  if(send_status < 0)
  {
    return 0;
  }

  send_status = sendLine(sock, LENGTH_LINE, strLens);
  if(send_status < 0)
  {
    return 0;
  }

  send_status = exoHAL_SocketSend(sock, exosite_provision_info, length);
  if(send_status < 0)
  {
    return 0;
  }

  http_status = get_http_status(sock);
  if(http_status < 0)
  {
    return 0;
  }

  if (200 == http_status)
  {
    unsigned char len;
    unsigned char cik_len_valid = 0;
    char *p;
    unsigned char crlf = 0;
    unsigned char ciklen = 0;
    char NCIK[CIK_LENGTH + 1];
    char plenbuf[16] = {0};
    unsigned char contlen = 0, ContLenfound = 0;
    int ContentLen = 1;

    osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
    (*(UINT16*)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0]) |= EXO_STATUS_VALID_ID;
    osi_LockObjUnlock(&g_NvmemLockObj);

    do
    {
      strLen = exoHAL_SocketRecv(sock, &strBuf[0], RX_SIZE);
      if(strLen < 0)
      {
         return 0;
      }
      //memcpy((void *)&TestCharBuff[TstrLen],&strBuf[0],strLen);//DKS test
      TstrLen += strLen;

      len = strLen;
      p = strBuf;

      // Find 4 consecutive \r or \n - should be: \r\n\r\n
      while (0 < len && 4 > crlf)
      {
        if ('\r' == *p || '\n' == *p)
        {
          ++crlf;
          if((2 == crlf) && (1 == ContLenfound))
          {
            ContLenfound = 2;
          }
          if((4 == crlf) && (2 == ContLenfound))
          {
            ContentLen = atoi(plenbuf);
            Report("Content length active %d\n\r", ContentLen);
            if(ContentLen == CIK_LENGTH)
            {
              cik_len_valid = 1;
            }
          }
        }
        else
        {
          crlf = 0;
          if (*cmp == *p)
          {
            cmp++;
          }
          else if ((cmp > (cmp_ss + strlen(cmp_ss) - 1)) && (*cmp == 0))
          {
            // check the cik length from http response
            if(2 != ContLenfound)//2 means cont length read
            {
              plenbuf[contlen++] = *p;
              ContLenfound = 1;//wait till /r/n
            }
          }
          else
          {
            cmp = cmp_ss;
          }
        }
        ++p;
        --len;
      }

      // The body is the cik
      if (0 < len && 4 == crlf )
      {
        // TODO, be more robust - match Content-Length header value to CIK_LENGTH
        unsigned char need, part;

        if((CIK_LENGTH > ciklen) && (1 == cik_len_valid))
        {
          need = CIK_LENGTH - ciklen;
          part = need < len ? need : len;
          strncpy(NCIK + ciklen, p, part);
          ciklen += part;
        }

        ContentLen -= len;
        if(ContentLen < 0)
        {
          Report("Error in content length \n\r");
          ContentLen = 0;
          exoHAL_SocketClose(sock);// because of closing the socket no need to flush rx buffer
          sock = -1;
          Report("exosite socket closed %d \n\r", http_status);
        }
      }
    } while (ContentLen > 0);

    if (CIK_LENGTH == ciklen)
    {
      NCIK[40] = 0;
      Exosite_SetCIK(NCIK);
      newcik = 1;
    }
    else
    {
      exoHAL_SocketClose(sock);// because of closing the socket no need to flush rx buffer
      Report("exosite socket closed %d \n\r", http_status);
    }

    Report("3 extra lines in active %d, %d \n\r", TstrLen, http_status);
  }
  else
  {
    exoHAL_SocketClose(sock);// because of closing the socket no need to flush rx buffer
    Report("exosite socket closed %d \n\r", http_status);
  }

  osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
  if (404 == http_status)
  {
    status_code = EXO_STATUS_BAD_SN;
    (*(UINT16*)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0]) &= (~EXO_STATUS_VALID_ID);
  }
  else if ((409 == http_status) || (408 == http_status))
  {
    status_code = EXO_STATUS_CONFLICT;
    (*(UINT16*)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0]) &= (~EXO_STATUS_VALID_CIK);
    (*(UINT16*)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0]) |= EXO_STATUS_VALID_ID;
  }
  SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_DATA_SEND_EXO_CFG;
  osi_LockObjUnlock(&g_NvmemLockObj);

  return newcik;
}


/*****************************************************************************
*
* Exosite_SetMRF
*
*  \param  char *buffer, int length
*
*  \return None
*
*  \brief  Programs a new MRF structure to flash / non volatile
*
*****************************************************************************/
void
Exosite_SetMRF(char *buffer, int length)
{
  exosite_meta_write((unsigned char *)buffer, length, META_MFR);
}


/*****************************************************************************
*
* Exosite_GetMRF
*
*  \param  char *buffer, int length
*
*  \return None
*
*  \brief  Retrieves a MRF structure from flash / non volatile
*
*****************************************************************************/
void
Exosite_GetMRF(char *buffer, int length)
{
  exosite_meta_read((unsigned char *)buffer, length, META_MFR);
}


/*****************************************************************************
*
* Exosite_SetCIK
*
*  \param  pointer to CIK
*
*  \return None
*
*  \brief  Programs a new CIK to flash / non volatile
*
*****************************************************************************/
void
Exosite_SetCIK(char * pCIK)
{
  exosite_meta_write((unsigned char *)pCIK, CIK_LENGTH, META_CIK);

  osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
  (*(UINT16*)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0]) |= EXO_STATUS_VALID_CIK;
  SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_EXO_CFG|NVMEM_DATA_SEND_EXO_CFG);
  osi_LockObjUnlock(&g_NvmemLockObj);

  status_code = EXO_STATUS_OK;
  return;
}


/*****************************************************************************
*
* Exosite_GetCIK
*
*  \param  pointer to buffer to receive CIK or NULL
*
*  \return 1 - CIK was valid, 0 - CIK was invalid.
*
*  \brief  Retrieves a CIK from flash / non volatile and verifies the CIK
*          format is valid
*
*****************************************************************************/
int
Exosite_GetCIK(char * pCIK)
{
  unsigned char i;
  char tempCIK[CIK_LENGTH + 1];

  exosite_meta_read((unsigned char *)tempCIK, CIK_LENGTH, META_CIK);
  tempCIK[CIK_LENGTH] = 0;

  for (i = 0; i < CIK_LENGTH; i++)
  {
    if (!(tempCIK[i] >= 'a' && tempCIK[i] <= 'f' || tempCIK[i] >= '0' && tempCIK[i] <= '9'))
    {
      status_code = EXO_STATUS_BAD_CIK;

      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      (*(UINT16*)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0]) &= ~(EXO_STATUS_VALID_CIK);
      SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_DATA_SEND_EXO_CFG;
      osi_LockObjUnlock(&g_NvmemLockObj);

      return 0;
    }
  }

  if (NULL != pCIK)
    memcpy(pCIK ,tempCIK ,CIK_LENGTH + 1);

  return 1;
}

UINT32 mCnt = 0;
UINT32 nCnt = 0;

/*****************************************************************************
*
* Exosite_Write
*
*  \param  pbuf - string buffer containing data to be sent
*          bufsize - number of bytes to send
*
*  \return 1 success; 0 failure
*
*  \brief  Writes data to Exosite cloud
*
*****************************************************************************/
int
Exosite_Write(char * pbuf, unsigned int bufsize)
{
  int success = 0, TstrLen = 0;
  int http_status = 0;
  char bufCIK[44];
  char strBuf[16];
  char strBufDummy[RX_SIZE + 8];
  int strLen;
  int send_status = -1;
  char *cmp_ss = "Content-Length: ";
  //char *cmp_dt = "Date: ";
  char *cmp = cmp_ss;
  //char *cmpdt = cmp_dt;

mCnt = 0;

  if(!(GetCC3200State() & CC3200_GOOD_INTERNET))
  {
    return success;
  }

  if (EXO_STATUS_OK != Exosite_StatusCode())
  {
    return success;
  }

  if (!Exosite_GetCIK(bufCIK))
  {
    return success;
  }

mCnt = 1;
  long sock = connect_to_exosite(EXO_DEFAULT_CON);
  if (sock < 0)
  {
    return 0;
  }
mCnt = 2;

// This is an example write POST...
//  s.send('POST /onep:v1/stack/alias HTTP/1.1\r\n')
//  s.send('Host: m2.exosite.com\r\n')
//  s.send('X-Exosite-CIK: 5046454a9a1666c3acfae63bc854ec1367167815\r\n')
//  s.send('Content-Type: application/x-www-form-urlencoded; charset=utf-8\r\n')
//  s.send('Content-Length: 6\r\n\r\n')
//  s.send('temp=2')

  itoa((int)bufsize, strBuf, 10); //make a string for length

mCnt = 3;
  send_status = sendLine(sock, POSTDATA_LINE, "/onep:v1/stack/alias");
  if(send_status < 0)
  {
    return 0;
  }

  send_status = sendLine(sock, HOST_LINE, NULL);
  if(send_status < 0)
  {
    return 0;
  }

  send_status = sendLine(sock, CIK_LINE, bufCIK);
  if(send_status < 0)
  {
    return 0;
  }

  send_status = sendLine(sock, CONTENT_LINE, NULL);
  if(send_status < 0)
  {
    return 0;
  }

  send_status = sendLine(sock, LENGTH_LINE, strBuf);
  if(send_status < 0)
  {
    return 0;
  }

  send_status = exoHAL_SocketSend(sock, pbuf, bufsize);
  if(send_status < 0)
  {
    return 0;
  }

  http_status = get_http_status(sock);
  if(http_status < 0)
  {
    return 0;
  }

  /* flush Rx buffer after receiving http status */
  if((http_status == 204) || (200 == http_status))
  {
    unsigned char len;
    char *p;
    unsigned char crlf = 0;
    char plenbuf[16] = {0};
    unsigned char contlen = 0, ContLenfound = 0;
    int ContentLen = 1;

    do
    {
      strLen = exoHAL_SocketRecv(sock, &strBufDummy[0], RX_SIZE);
      if(strLen < 0)
      {
         return 0;
      }
      //memcpy((void *)&TestCharBuff[TstrLen],&strBufDummy[0],strLen);//DKS test

      TstrLen += strLen;

      len = strLen;
      p = strBufDummy;

      // Find 4 consecutive \r or \n - should be: \r\n\r\n
      while (0 < len && 4 > crlf)
      {
        if ('\r' == *p || '\n' == *p)
        {
          ++crlf;
          if((2 == crlf) && (1 == ContLenfound))
          {
            ContLenfound = 2;
          }
          if((4 == crlf) && (2 == ContLenfound))
          {
            ContentLen = atoi(plenbuf);
            Report("Content length read %d\n\r", ContentLen);
          }
        }
        else
        {
          crlf = 0;
          if (*cmp == *p)
          {
            cmp++;
          }
          else if ((cmp > (cmp_ss + strlen(cmp_ss) - 1)) && (*cmp == 0))
          {
            // check the cik length from http response
            if(2 != ContLenfound)//2 means cont length read
            {
              plenbuf[contlen++] = *p;
              ContLenfound = 1;//wait till /r/n
            }
          }
          else
          {
            cmp = cmp_ss;
          }
        }
        ++p;
        --len;
      }

      // The body is the cik
      if (0 < len && 4 == crlf )
      {
        ContentLen -= len;
        if(ContentLen < 0)
        {
          Report("Error in content length \n\r");
          ContentLen = 0;
          nCnt = 4;
          exoHAL_SocketClose(sock);// because of closing the socket no need to flush rx buffer
          sock = -1;
          nCnt = 5;
          Report("exosite socket closed %d \n\r", http_status);
        }
      }
    } while (ContentLen > 0);

    //Report("1 extra lines in send %d, %d \n\r", TstrLen, http_status);
    Report("http status write %d \n\r", http_status);
  }
  else
  {
    mCnt = 4;
    exoHAL_SocketClose(sock);// because of closing the socket no need to flush rx buffer
    mCnt = 5;
    Report("exosite socket closed %d \n\r", http_status);
  }

  if ((204 == http_status) || (200 == http_status))
  {
    success = 1;
    status_code = EXO_STATUS_OK;
  }
  else if (401 == http_status)
  {
    status_code = EXO_STATUS_NOAUTH;
    osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
    (*(UINT16*)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0]) &= (~EXO_STATUS_VALID_CIK);
    osi_LockObjUnlock(&g_NvmemLockObj);
  }

  return success;
}

/*****************************************************************************
*
* Exosite_Read
*
*  \param  palias - string, name of the datasource alias to read from
*          pbuf - read buffer to put the read response into
*          buflen - size of the input buffer
*
*  \return number of bytes read
*
*  \brief  Reads data from Exosite cloud
*
*****************************************************************************/
int Exosite_Read(char * palias, t_ExositeReadData *ExodataRead)
{
  int success = 0, TstrLen = 0;
  int http_status = 0, strLen, Contglobalind = 0;
  char bufCIK[44];
  char strBuf[RX_SIZE + 8];
  unsigned char len, vlen = 0, alias_match = 0, Start_Ind = 0, End_ind = 0;
  char *array_alias[12] = {0};
  char *p, *pcheck;
  int send_status = -1;
  char *cmp_ss = "Content-Length: ";
  //char *cmp_dt = "Date: ";
  char *cmp = cmp_ss;
  //char *cmpdt = cmp_dt;

nCnt = 0;
  /* Clear validity flag */
  ExodataRead->val_flag = 0;

  if(!(GetCC3200State() & CC3200_GOOD_INTERNET))
  {
    return success;
  }

  if (EXO_STATUS_OK != Exosite_StatusCode())
  {
    return success;
  }

  if (!Exosite_GetCIK(bufCIK))
  {
    return success;
  }

nCnt = 1;
  long sock = connect_to_exosite(EXO_DEFAULT_CON);
  if (sock < 0)
  {
    return 0;
  }
nCnt = 2;

// This is an example read GET
//  s.send('GET /onep:v1/stack/alias?temp HTTP/1.1\r\n')
//  s.send('Host: m2.exosite.com\r\n')
//  s.send('X-Exosite-CIK: 5046454a9a1666c3acfae63bc854ec1367167815\r\n')
//  s.send('Accept: application/x-www-form-urlencoded; charset=utf-8\r\n\r\n')

  send_status = sendLine(sock, GETDATA_LINE, palias);
  if(send_status < 0)
  {
    return 0;
  }

  send_status = sendLine(sock, HOST_LINE, NULL);
  if(send_status < 0)
  {
    return 0;
  }

  send_status = sendLine(sock, CIK_LINE, bufCIK);
  if(send_status < 0)
  {
    return 0;
  }

  send_status = sendLine(sock, ACCEPT_LINE, "\r\n");
  if(send_status < 0)
  {
    return 0;
  }

  if(memcmp(EXO_READ_COMMAND, (const void *)palias, strlen(EXO_READ_COMMAND)) == 0)
  {
    /* read command */
    array_alias[E_EXO_DATA_COMMAND] = EXO_DATA_COMMAND;
    array_alias[E_EXO_DATA_CONTROL] = EXO_DATA_CONTROL;
    Start_Ind = E_EXO_DATA_COMMAND;
    End_ind = E_EXO_DATA_CONTROL;
  }
  else if(memcmp(EXO_READ_DEV_INFO, (const void *)palias, strlen(EXO_READ_DEV_INFO)) == 0)
  {
    /* read device info */
    array_alias[E_EXO_DATA_CONTROL] = EXO_DATA_CONTROL;
    array_alias[E_EXO_DATA_REPOINT] = EXO_DATA_REPOINT;
    array_alias[E_EXO_DATA_POWMODE] = EXO_DATA_POWMODE;
    array_alias[E_EXO_DATA_POWRTHD] = EXO_DATA_POWRTHD;
    array_alias[E_EXO_DATA_ENRGTHD] = EXO_DATA_ENRGTHD;
    array_alias[E_EXO_DATA_SCHEDUL] = EXO_DATA_SCHEDULE;
    Start_Ind = E_EXO_DATA_CONTROL;
    End_ind   = E_EXO_DATA_SCHEDUL;
  }
nCnt = 3;

  http_status = get_http_status(sock);
  if(http_status < 0)
  {
    return 0;
  }

  Report("http status read %d \n\r", http_status);

  if ((200 == http_status) || (204 == http_status))
  {
    unsigned char crlf = 0;
    char plenbuf[16] = {0}, databuf[128];
    unsigned char contlen = 0, ContLenfound = 0, cont_ind = 0;
    unsigned char bufflen = 100, aliasind_found = 0, alias_ind = 0;
    int ContentLen = 1;

    do
    {
      strLen = exoHAL_SocketRecv(sock, &strBuf[0], RX_SIZE);
      if(strLen < 0)
      {
         return 0;
      }

      //memcpy((void *)&TestCharBuff[TstrLen],&strBuf[0],strLen);//DKS test
      TstrLen += strLen;

      len = strLen;
      p = strBuf;

      // Find 4 consecutive \r or \n - should be: \r\n\r\n
      while (0 < len && 4 > crlf)
      {
        if ('\r' == *p || '\n' == *p)
        {
          ++crlf;
          if((2 == crlf) && (1 == ContLenfound))
          {
            ContLenfound = 2;
          }
          if((4 == crlf) && (2 == ContLenfound))
          {
            ContentLen = atoi(plenbuf);
            Report("Content length read %d\n\r", ContentLen);
          }
        }
        else
        {
          crlf = 0;
          if (*cmp == *p)
          {
            cmp++;
          }
          else if ((cmp > (cmp_ss + strlen(cmp_ss) - 1)) && (*cmp == 0))
          {
            // check the cik length from http response
            if(2 != ContLenfound)//2 means cont length read
            {
              plenbuf[contlen++] = *p;
              ContLenfound = 1;//wait till /r/n
            }
          }
          else
          {
            cmp = cmp_ss;
          }
        }
        ++p;
        --len;
      }

      // The body is "<key>=<value>"
      if (0 < len && 4 == crlf)
      {
        ContentLen -= len;
        if(ContentLen < 0)
        {
          Report("Error in content length \n\r");
          ContentLen = 0;
          nCnt = 4;
          exoHAL_SocketClose(sock);// because of closing the socket no need to flush rx buffer
          sock = -1;
          nCnt = 5;
          Report("exosite socket closed %d \n\r", http_status);
        }

        for(cont_ind = 0; cont_ind < len; cont_ind++)
        {
          ContentBody[Contglobalind++] = *p++;
        }
      }
    }while (ContentLen > 0);

    len = Contglobalind;
    p = &ContentBody[0];

    while (0 < len)
    {
      for(vlen = 0; vlen < bufflen; vlen++)
      {
        databuf[vlen] = 0;
      }
      vlen = 0;

      // Move past "<key>"
      while (0 < len && (0 == alias_match))
      {
        for(alias_ind = Start_Ind; alias_ind <= End_ind; alias_ind++)
        {
          pcheck = array_alias[alias_ind];
          if(0 == pcheck)
          {
            continue;
          }
          if(memcmp(pcheck, (const void *)p, strlen(pcheck)) == 0)
          {
            //Match '='
            if('=' == *(p+strlen(pcheck)))
            {
              alias_match = 1;
              aliasind_found = alias_ind;
              p += strlen(pcheck);
              len -= strlen(pcheck);
              break;
            }
          }
        }
        ++p;//for '=' check
        --len;
      }

      // we should now have '<key>='
      // read in the rest of the body as the value
      while (0 < len && (1 == alias_match))
      {
        if(*p != '&')
        {
          if(vlen < bufflen)
          {
            databuf[vlen++] = *p;
          }
        }
        else
        {
          alias_match = 0;
        }
        ++p;
        --len;
      }

      if(vlen > 0)
      {
        switch(aliasind_found)
        {
          case E_EXO_DATA_COMMAND:
            ExodataRead->command   = (UINT8)atoi(databuf);
            ExodataRead->val_flag  |= (1 << E_EXO_DATA_COMMAND);
            break;
          case E_EXO_DATA_CONTROL:
            ExodataRead->control   = (UINT8)atoi(databuf);
            ExodataRead->val_flag  |= (1 << E_EXO_DATA_CONTROL);
            break;
          case E_EXO_DATA_REPOINT:
            ExodataRead->interval  = (UINT8)atoi(databuf);
            ExodataRead->val_flag  |= (1 << E_EXO_DATA_REPOINT);
            break;
          case E_EXO_DATA_POWMODE:
            ExodataRead->psmode    = (UINT8)atoi(databuf);
            ExodataRead->val_flag  |= (1 << E_EXO_DATA_POWMODE);
            break;
          case E_EXO_DATA_POWRTHD:
            ExodataRead->powerthd  = (float)atof(databuf);
            ExodataRead->val_flag  |= (1 << E_EXO_DATA_POWRTHD);
            break;
          case E_EXO_DATA_ENRGTHD:
            ExodataRead->energythd = (float)atof(databuf);
            ExodataRead->val_flag  |= (1 << E_EXO_DATA_ENRGTHD);
            break;
          case E_EXO_DATA_SCHEDUL:
          {
            char  *pS, databufS[4]={0};
            UINT8 *p_Table, lenS = 0, bufflenS = 2;
            /* Decode schedule table from string */
            // we should now have schedule table string
            p_Table = &ExodataRead->schedule.mon_fri.wk_hour;
            pS = &databuf[0];
            while (0 < vlen)
            {
              if(0 == memcmp(pS, (const void *)"%2c", 3)) //',' ascii char is 2c, received as %2c string in URL encoding
              {
                pS += 3;
              }
              if(*pS != ',')
              {
                databufS[lenS++] = *pS;
                if(lenS == bufflenS)
                {
                  *p_Table++ = (UINT8)atoi(databufS);
                  lenS = 0;
                }
              }
              ++pS;
              --vlen;
            }
            ExodataRead->val_flag  |= (1 << E_EXO_DATA_SCHEDUL);
            break;
          }
        }
      }
    }

    //Report("read size %x, %d \n\r", ExodataRead->val_flag, TstrLen);

    if((0 == vlen) && (200 == http_status))
    {
      nCnt = 4;
      exoHAL_SocketClose(sock);// because of closing the socket no need to flush rx buffer
      nCnt = 5;
      Report("exosite socket closed %d \n\r", http_status);
    }
  }
  else
  {
    nCnt = 4;
    exoHAL_SocketClose(sock);// because of closing the socket no need to flush rx buffer
    nCnt = 5;
    Report("exosite socket closed %d \n\r", http_status);
  }

  if ((200 == http_status)||(204 == http_status))
  {
    status_code = EXO_STATUS_OK;
  }
  else if (401 == http_status)
  {
    status_code = EXO_STATUS_NOAUTH;
    osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
    (*(UINT16*)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0]) &= (~EXO_STATUS_VALID_CIK);
    osi_LockObjUnlock(&g_NvmemLockObj);
  }

  return (ExodataRead->val_flag);
}

/*****************************************************************************
*
* Exosite_Time_Read
*
*
*  \return number of bytes read
*
*  \brief  Reads data from Exosite cloud
*
*****************************************************************************/
int Exosite_Time_Read(t_smartplugdate *SmartPlugDate)
{
  int success = 0, TstrLen = 0;
  int http_status = 0, strLen;
  char strBuf[RX_SIZE + 5];
  unsigned char len;
  char *p;
  int send_status = -1;
  char *cmp_ss = "Date: ";
  char *cmp = cmp_ss;

  long sock = connect_to_exosite(EXO_NON_SECURE_CON);
  if (sock < 0)
  {
    return 0;
  }

  /* Clear flag as this is not secured connection */
  osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
  (*(UINT16*)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0]) &= (~EXO_STATUS_VALID_CONN);
  osi_LockObjUnlock(&g_NvmemLockObj);

  // This is an example read GET /timestamp
  //  s.send('GET /timestamp HTTP/1.1\r\n')
  //  s.send('Host: m2.exosite.com\r\n\r\n')

  send_status = sendLine(sock, GETDATA_GEN_LINE, "timestamp");
  if(send_status < 0)
  {
    return 0;
  }

  send_status = sendLine(sock, HOST_LINE, NULL);
  if(send_status < 0)
  {
    return 0;
  }

  send_status = sendLine(sock, EMPTY_LINE, "\r\n");
  if(send_status < 0)
  {
    return 0;
  }

  http_status = get_http_status(sock);
  if(http_status < 0)
  {
    return 0;
  }

  Report("http status read %d \n\r", http_status);

  if ((200 == http_status) || (204 == http_status))
  {
    unsigned char crlf = 0;
    char plenbuf[50] = {0};
    unsigned char contlen = 0, ContLenfound = 0;
    int ContentLen = 1;

    do
    {
      strLen = exoHAL_SocketRecv(sock, &strBuf[0], RX_SIZE);
      if(strLen < 0)
      {
         return 0;
      }

      //memcpy((void *)&TestCharBuff[TstrLen],&strBuf[0],strLen);//DKS test
      TstrLen += strLen;

      len = strLen;
      p = strBuf;

      // Find 4 consecutive \r or \n - should be: \r\n\r\n
      while (0 < len && 4 > crlf)
      {
        if ('\r' == *p || '\n' == *p)
        {
          ++crlf;
          if((2 == crlf) && (1 == ContLenfound))
          {
            ContLenfound = 2;
            ContentLen = 0;
            break;
          }
          if(4 == crlf)
          {
            ContentLen = 0;
            break;
          }
        }
        else
        {
          crlf = 0;
          if (*cmp == *p)
          {
            cmp++;
          }
          else if ((cmp > (cmp_ss + strlen(cmp_ss) - 1)) && (*cmp == 0))
          {
            // check the cik length from http response
            if(2 != ContLenfound)//2 means cont length read
            {
              plenbuf[contlen++] = *p;
              ContLenfound = 1;//wait till /r/n
            }
          }
          else
          {
            cmp = cmp_ss;
          }
        }
        ++p;
        --len;
      }
    }while (ContentLen > 0);

    /* parse for date */
    if((2 == ContLenfound) && (contlen >= 29))
    {
      char date[2], month[3], year[4];

      p = &plenbuf[0];
      success = 1;

      /* Date: Fri, 14 Mar 2014 08:44:32 GMT - RFC 1123 format */
      p += 5; //week,

      date[0] = *p++;
      date[1] = *p++;//date
      p++; //
      month[0] = *p++;
      month[1] = *p++;
      month[2] = *p++;//month
      p++; //
      year[0] = *p++;
      year[1] = *p++;
      year[2] = *p++;
      year[3] = *p++;//year

      SmartPlugDate->date = (UINT8)atoi(date);
      SmartPlugDate->year = (UINT16)atoi(year);

      if(memcmp(month, (const void *)"Jan", strlen("Jan")) == 0)
      {
        SmartPlugDate->month = 1;
      }
      else if(memcmp(month, (const void *)"Feb", strlen("Feb")) == 0)
      {
        SmartPlugDate->month = 2;
      }
      else if(memcmp(month, (const void *)"Mar", strlen("Mar")) == 0)
      {
        SmartPlugDate->month = 3;
      }
      else if(memcmp(month, (const void *)"Apr", strlen("Apr")) == 0)
      {
        SmartPlugDate->month = 4;
      }
      else if(memcmp(month, (const void *)"May", strlen("May")) == 0)
      {
        SmartPlugDate->month = 5;
      }
      else if(memcmp(month, (const void *)"Jun", strlen("Jun")) == 0)
      {
        SmartPlugDate->month = 6;
      }
      else if(memcmp(month, (const void *)"Jul", strlen("Jul")) == 0)
      {
        SmartPlugDate->month = 7;
      }
      else if(memcmp(month, (const void *)"Aug", strlen("Aug")) == 0)
      {
        SmartPlugDate->month = 8;
      }
      else if(memcmp(month, (const void *)"Sep", strlen("Sep")) == 0)
      {
        SmartPlugDate->month = 9;
      }
      else if(memcmp(month, (const void *)"Oct", strlen("Oct")) == 0)
      {
        SmartPlugDate->month = 10;
      }
      else if(memcmp(month, (const void *)"Nov", strlen("Nov")) == 0)
      {
        SmartPlugDate->month = 11;
      }
      else if(memcmp(month, (const void *)"Dec", strlen("Dec")) == 0)
      {
        SmartPlugDate->month = 12;
      }
      else
      {
        success = 0;
      }
    }

    Report("smartplug date %d,%d,%d, %d \n\r", SmartPlugDate->date, SmartPlugDate->month, SmartPlugDate->year, contlen);
  }

  exoHAL_SocketClose(sock);// close the socket once done

  return (success);
}

UINT32 oCnt = 0;
/*****************************************************************************
*
* connect_to_exosite
*
*  \param  None
*
*  \return success: socket handle; failure: -1;
*
*  \brief  Establishes a connection with Exosite API server
*
*****************************************************************************/
long
connect_to_exosite(ExositeConnect con_type)
{
  unsigned char connectRetries = 0, NoInternet = 0;
  char server[META_SERVER_SIZE+1];
  long sock = -1;
  UINT16 portNum = 0;
  oCnt = 0;

  if(ExositeConStatus.g_ClientSoc >= 0)
  {
    return ExositeConStatus.g_ClientSoc;
  }

  exosite_meta_read((unsigned char*)server, META_SERVER_SIZE, META_SERVER);
  server[META_SERVER_SIZE] = 0;

  switch(con_type)
  {
    case EXO_NON_SECURE_CON:
      portNum = EXO_PORT_NONSEC;
      break;

    case EXO_SECURE_CON:
      portNum = EXO_PORT_SECURE;
      break;

    default:
    case EXO_DEFAULT_CON:
      exosite_meta_read((unsigned char*)&portNum, META_PORT_NUM_SIZE, META_PORT_NUM);
      break;
  }

  while (connectRetries++ < EXOSITE_MAX_CONNECT_RETRY_COUNT)
  {
    /* Get IP address of m2.exosite.com */
    if(exoHAL_ServerAddressGet(server, portNum) < 0)
    {
      NoInternet = 1;
      break;
    }

    oCnt = 1;
    sock = exoHAL_SocketOpenTCP(portNum);
    oCnt = 2;

    if (sock < 0)
    {
      continue;
    }

    oCnt = 3;
    if (exoHAL_ServerConnect(sock) < 0)  // Try to connect
    {
      // TODO - the typical reason the connect doesn't work is because
      // something was wrong in the way the comms hardware was initialized (timing, bit
      // error, etc...). There may be a graceful way to kick the hardware
      // back into gear at the right state, but for now, we just
      // return and let the caller retry us if they want
      oCnt = 4;
      exoHAL_SocketClose(sock);
      oCnt = 5;
      sock = -1;

      NoInternet = 1;
      Report("Could not connect to exosite \n\r");
      OSI_DELAY(100);//wait 100ms
      continue;
    }
    else
    {
      connectRetries = 0;
      NoInternet = 0;
      oCnt = 6;
      break;
    }
  }
  oCnt = 7;

  osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
  if(EXO_PORT_SECURE == portNum)
  {
    (*(UINT16*)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0]) |= (EXO_STATUS_SECUR_CONN);
  }
  else
  {
    (*(UINT16*)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0]) &= (~EXO_STATUS_SECUR_CONN);
  }
  osi_LockObjUnlock(&g_NvmemLockObj);

  if(sock < 0)
  {
    if(0 == NoInternet)
    {
      UnsetCC3200MachineState(CC3200_GOOD_TCP|CC3200_GOOD_INTERNET);
    }
    else
    {
      UnsetCC3200MachineState(CC3200_GOOD_INTERNET);
      if(g_SmartplugLedCurrentState > DEVICE_ERROR_IND)
      {
        if(g_ConnectionState.g_ClientSD >= 0)
        {
          g_SmartplugLedCurrentState = CONNECTED_ANDROID_IND;
        }
        else
        {
          g_SmartplugLedCurrentState = CONNECTED_TO_AP_IND;
        }
      }
    }

    osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
    (*(UINT16*)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0]) &= (~EXO_STATUS_VALID_CONN);
    SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_DATA_SEND_EXO_CFG;
    osi_LockObjUnlock(&g_NvmemLockObj);
  }
  else
  {
    ExositeConStatus.g_ClientSoc = sock;
    SetCC3200MachineState((CC3200_GOOD_TCP|CC3200_GOOD_INTERNET));
    if(g_SmartplugLedCurrentState > DEVICE_ERROR_IND)
    {
      g_SmartplugLedCurrentState = CONNECTED_CLOUD_IND;
    }

    osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
    (*(UINT16*)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0]) |= (EXO_STATUS_VALID_CONN);
    SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_DATA_SEND_EXO_CFG;
    osi_LockObjUnlock(&g_NvmemLockObj);

    Report("Connected to exosite \n\r");
  }
  // Success
  return sock;
}


/*****************************************************************************
*
* get_http_status
*
*  \param  socket handle, pointer to expected HTTP response code
*
*  \return http response code, 0 tcp failure
*
*  \brief  Reads first 12 bytes of HTTP response and extracts the 3 byte code
*
*****************************************************************************/
int
get_http_status(long socket)
{
  char rxBuf[15];
  int rxLen = 0;
  int code = 0;

  rxLen = exoHAL_SocketRecv(socket, rxBuf, 12);

  if(rxLen < 0)
  {
    return -1;
  }

  // exampel '4','0','4' =>  404  (as number)
  code = (((rxBuf[9] - 0x30) * 100) +
          ((rxBuf[10] - 0x30) * 10) +
          (rxBuf[11] - 0x30));

  return code;
}


/*****************************************************************************
*
*  sendLine
*
*  \param  Which line type
*
*  \return socket handle
*
*  \brief  Sends data out
*
*****************************************************************************/
int
sendLine(long socket, unsigned char LINE, const char * payload)
{
  char strBuf[255];
  unsigned int strLen = 0;
  int send_status = -1;

  switch(LINE) {
    case CIK_LINE:
      strLen = strlen(STR_CIK_HEADER);
      memcpy(strBuf,STR_CIK_HEADER,strLen);
      memcpy(&strBuf[strLen],payload, strlen(payload));
      strLen += strlen(payload);
      memcpy(&strBuf[strLen],STR_CRLF, 2);
      strLen += strlen(STR_CRLF);
      break;
    case HOST_LINE:
      strLen = strlen(STR_HOST);
      memcpy(strBuf,STR_HOST,strLen);
      break;
    case CONTENT_LINE:
      strLen = strlen(STR_CONTENT);
      memcpy(strBuf,STR_CONTENT,strLen);
      break;
    case ACCEPT_LINE:
      strLen = strlen(STR_ACCEPT);
      memcpy(strBuf,STR_ACCEPT,strLen);
      memcpy(&strBuf[strLen],payload, strlen(payload));
      strLen += strlen(payload);
      break;
    case LENGTH_LINE: // Content-Length: NN
      strLen = strlen(STR_CONTENT_LENGTH);
      memcpy(strBuf,STR_CONTENT_LENGTH,strLen);
      memcpy(&strBuf[strLen],payload, strlen(payload));
      strLen += strlen(payload);
      memcpy(&strBuf[strLen],STR_CRLF, 2);
      strLen += 2;
      memcpy(&strBuf[strLen],STR_CRLF, 2);
      strLen += 2;
      break;
    case GETDATA_LINE:
      strLen = strlen(STR_GET_URL);
      memcpy(strBuf,STR_GET_URL,strLen);
      memcpy(&strBuf[strLen],payload, strlen(payload));
      strLen += strlen(payload);
      memcpy(&strBuf[strLen],STR_HTTP, strlen(STR_HTTP));
      strLen += strlen(STR_HTTP);
      break;
    case POSTDATA_LINE:
      strLen = strlen("POST ");
      memcpy(strBuf,"POST ", strLen);
      memcpy(&strBuf[strLen],payload, strlen(payload));
      strLen += strlen(payload);
      memcpy(&strBuf[strLen],STR_HTTP, strlen(STR_HTTP));
      strLen += strlen(STR_HTTP);
      break;
    case GETDATA_GEN_LINE:
      strLen = strlen(STR_GET_GEN_URL);
      memcpy(strBuf,STR_GET_GEN_URL,strLen);
      memcpy(&strBuf[strLen],payload, strlen(payload));
      strLen += strlen(payload);
      memcpy(&strBuf[strLen],STR_HTTP, strlen(STR_HTTP));
      strLen += strlen(STR_HTTP);
      break;
    case EMPTY_LINE:
      strLen = strlen(STR_CRLF);
      memcpy(strBuf,STR_CRLF,strLen);
      break;
    default:
      break;
  }

  strBuf[strLen] = 0;

  //Report("Exosite send length %d \n\r", strLen);
  send_status = exoHAL_SocketSend(socket, strBuf, strLen);

  return send_status;
}




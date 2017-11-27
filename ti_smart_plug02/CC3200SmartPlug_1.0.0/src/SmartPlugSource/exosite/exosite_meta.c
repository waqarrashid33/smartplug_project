/*****************************************************************************
*
*  exosite_meta.c - Exosite meta information handler.
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

//const unsigned char meta_server_ip[6] = {173,255,209,28,0,80}; //23.239.1.50
//const unsigned char meta_server_ip[6] = {23,239,1,50,0,80};
//const unsigned char meta_server_ip_ssl[6] = {173,255,209,28,0x01,0xBB};//443

#include "datatypes.h"
#include "user.h"
#include "exosite_hal.h"
#include "exosite_meta.h"
#include "metrology.h"
#include "smartplug_task.h"
#include "exosite.h"
#include <uart_logger.h>

extern t_SmartPlugNvmmFile SmartPlugNvmmFile;
extern OsiLockObj_t        g_NvmemLockObj;

// external functions
// externs
// local functions
// exported functions
// local defines
// globals

/*****************************************************************************
*
*  exosite_meta_init
*
*  \param  int reset : 1 - reset meta data
*
*  \return None
*
*  \brief  Does whatever we need to do to initialize the NV meta structure
*
*****************************************************************************/
void exosite_meta_init(const unsigned char if_nbr, int reset)
{

  exoHAL_EnableMeta();  //turn on the necessary hardware / peripherals

  if (reset)
  {
    exosite_meta_defaults(if_nbr);
  }

  return;
}


/*****************************************************************************
*
*  exosite_meta_defaults
*
*  \param  None
*
*  \return None
*
*  \brief  Writes default meta values to NV memory.  Erases existing meta
*          information!
*
*****************************************************************************/
int exosite_meta_defaults(const unsigned char if_nbr)
{
  char struuid[META_UUID_SIZE+1];
  unsigned char uuid_len = 0;
  long portNum = EXO_DST_PORT;
  char server[META_SERVER_SIZE+1] = {0};

  exoHAL_EraseMeta(); //erase the information currently in meta

  uuid_len = exoHAL_ReadUUID(if_nbr, struuid);

  if (0 == uuid_len)
  {
    return 0;
  }

  if(uuid_len != META_UUID_SIZE)
  {
    Report("UUID length error");
  }

  memcpy((void *)&server[0],(void *)EXO_SERVER_NAME, strlen(EXO_SERVER_NAME));//copy server

  exosite_meta_write((unsigned char *)VENDOR_NAME, META_VNAME_SIZE, META_VENDOR);  //store vendor name

  exosite_meta_write((unsigned char *)MODEL_NAME,  META_MNAME_SIZE, META_MODEL);   //store model name

  exosite_meta_write((unsigned char *)struuid, META_UUID_SIZE, META_UUID);    //store UUID (mac add)

  exosite_meta_write((unsigned char *)server, META_SERVER_SIZE, META_SERVER);    //store server name

  exosite_meta_write((unsigned char *)&portNum, META_PORT_NUM_SIZE, META_PORT_NUM);    //store port num

  exosite_meta_write((unsigned char *)EXOMARK, META_MARK_SIZE, META_MARK); //store exosite mark

  return 1;
}


/*****************************************************************************
*
*  exosite_meta_write
*
*  \param  write_buffer - string buffer containing info to write to meta;
*          srcBytes - size of string in bytes; element - item from
*          MetaElements enum.
*
*  \return None
*
*  \brief  Writes specific meta information to meta memory
*
*****************************************************************************/
void exosite_meta_write(unsigned char * write_buffer, unsigned short srcBytes, unsigned char element)
{
  switch (element)
  {
    case META_CIK:
      if (srcBytes > META_CIK_SIZE) return;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      memcpy((void *)&SmartPlugNvmmFile.ExositeMetaData.cik[0],(void *)write_buffer,srcBytes);//store CIK
      osi_LockObjUnlock(&g_NvmemLockObj);
      break;
    case META_SERVER:
      if (srcBytes > META_SERVER_SIZE) return;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      memcpy((void *)&SmartPlugNvmmFile.ExositeMetaData.server[0],(void *)write_buffer,srcBytes);//store server name
      osi_LockObjUnlock(&g_NvmemLockObj);
      break;
    case META_PORT_NUM:
      if (srcBytes > META_PORT_NUM_SIZE) return;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      memcpy((void *)&SmartPlugNvmmFile.ExositeMetaData.portnum[0],(void *)write_buffer,srcBytes);//store port num
      if(EXO_PORT_SECURE == *((UINT16*)write_buffer))
      {
        (*(UINT16*)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0]) |= (EXO_STATUS_SECUR_CONN);
      }
      else
      {
        (*(UINT16*)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0]) &= (~EXO_STATUS_SECUR_CONN);
      }
      osi_LockObjUnlock(&g_NvmemLockObj);
      break;
    case META_MARK:
      if (srcBytes > META_MARK_SIZE) return;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      memcpy((void *)&SmartPlugNvmmFile.ExositeMetaData.mark[0],(void *)write_buffer,srcBytes);//store exosite mark
      osi_LockObjUnlock(&g_NvmemLockObj);
      break;
    case META_UUID:
      if (srcBytes > META_UUID_SIZE) return;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      memcpy((void *)&SmartPlugNvmmFile.ExositeMetaData.uuid[0],(void *)write_buffer,srcBytes);//store UUID
      osi_LockObjUnlock(&g_NvmemLockObj);
      break;
    case META_MFR:
      if (srcBytes > META_MFR_SIZE) return;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      memcpy((void *)&SmartPlugNvmmFile.ExositeMetaData.mfr[0],(void *)write_buffer,srcBytes);//store manufacturing info
      osi_LockObjUnlock(&g_NvmemLockObj);
      break;
    case META_VENDOR:
      if (srcBytes > META_VNAME_SIZE) return;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      memcpy((void *)&SmartPlugNvmmFile.ExositeMetaData.vendorname[0],(void *)write_buffer,srcBytes);//store vendor name
      osi_LockObjUnlock(&g_NvmemLockObj);
      break;
    case META_MODEL:
      if (srcBytes > META_MNAME_SIZE) return;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      memcpy((void *)&SmartPlugNvmmFile.ExositeMetaData.modelname[0],(void *)write_buffer,srcBytes);//store model name
      osi_LockObjUnlock(&g_NvmemLockObj);
      break;
    case META_NONE:
    default:
      break;
  }
  osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
  SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_EXO_CFG|NVMEM_DATA_SEND_EXO_CFG);
  osi_LockObjUnlock(&g_NvmemLockObj);

  return;
}


/*****************************************************************************
*
*  exosite_meta_read
*
*  \param  read_buffer - string buffer to receive element data; destBytes -
*          size of buffer in bytes; element - item from MetaElements enum.
*
*  \return None
*
*  \brief  Writes specific meta information to meta memory
*
*****************************************************************************/
void exosite_meta_read(unsigned char * read_buffer, unsigned short destBytes, unsigned char element)
{
  switch (element)
  {
    case META_CIK:
      if (destBytes < META_CIK_SIZE) return;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      memcpy((void *)read_buffer,(void *)&SmartPlugNvmmFile.ExositeMetaData.cik[0],META_CIK_SIZE);//read CIK
      osi_LockObjUnlock(&g_NvmemLockObj);
      break;
    case META_SERVER:
      if (destBytes < META_SERVER_SIZE) return;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      memcpy((void *)read_buffer,(void *)&SmartPlugNvmmFile.ExositeMetaData.server[0],META_SERVER_SIZE);//read server IP
      osi_LockObjUnlock(&g_NvmemLockObj);
      break;
    case META_PORT_NUM:
      if (destBytes < META_PORT_NUM_SIZE) return;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      memcpy((void *)read_buffer,(void *)&SmartPlugNvmmFile.ExositeMetaData.portnum[0],META_PORT_NUM_SIZE);//read server IP
      osi_LockObjUnlock(&g_NvmemLockObj);
      break;
    case META_MARK:
      if (destBytes < META_MARK_SIZE) return;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      memcpy((void *)read_buffer,(void *)&SmartPlugNvmmFile.ExositeMetaData.mark[0],META_MARK_SIZE);//read exosite mark
      osi_LockObjUnlock(&g_NvmemLockObj);
      break;
    case META_UUID:
      if (destBytes < META_UUID_SIZE) return;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      memcpy((void *)read_buffer,(void *)&SmartPlugNvmmFile.ExositeMetaData.uuid[0],META_UUID_SIZE);//read UUID
      osi_LockObjUnlock(&g_NvmemLockObj);
      break;
    case META_MFR:
      if (destBytes < META_MFR_SIZE) return;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      memcpy((void *)read_buffer,(void *)&SmartPlugNvmmFile.ExositeMetaData.mfr[0],META_MFR_SIZE);//read manufacturing info
      osi_LockObjUnlock(&g_NvmemLockObj);
      break;
    case META_VENDOR:
      if (destBytes < META_VNAME_SIZE) return;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      memcpy((void *)read_buffer,(void *)&SmartPlugNvmmFile.ExositeMetaData.vendorname[0],META_VNAME_SIZE);//read vendor name
      osi_LockObjUnlock(&g_NvmemLockObj);
      break;
    case META_MODEL:
      if (destBytes < META_MNAME_SIZE) return;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      memcpy((void *)read_buffer,(void *)&SmartPlugNvmmFile.ExositeMetaData.modelname[0],META_MNAME_SIZE);//read model name
      osi_LockObjUnlock(&g_NvmemLockObj);
      break;
    case META_NONE:
    default:
      break;
  }

  return;
}



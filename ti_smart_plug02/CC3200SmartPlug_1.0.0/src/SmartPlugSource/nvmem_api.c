//*****************************************************************************
// File: nvmem_api.c
//
// Description: NVMEM API handling functions of Smartplug gen-1 application
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
#include "stdlib.h"
#include "simplelink.h"
#include "nvmem_api.h"
#include "uart_logger.h"
#include "exosite_meta.h"
#include "uart_logger.h"
#include "metrology.h"
#include "smartplug_task.h"
#include "gpio_if.h"

extern t_SmartPlugNvmmFile SmartPlugNvmmFile;
extern OsiLockObj_t        g_NvmemLockObj;

#define DBG_PRINT           Report

static t_SmartPlugNvmmFile  SmartPlugNvmmFileCopy;
extern volatile LedIndicationName  g_SmartplugLedCurrentState;

int WriteSmartPlugNvmemFile( void )
{
  long lFileHandle;
  unsigned long ulToken;
  INT32 iRetVal, ret_val = -1;
  unsigned char *pFileName = (unsigned char*)SMARTPLUG_NVMEM_FILE_NAME;
  UINT32 NvmmFileUpdated;

  osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
  if(SmartPlugNvmmFile.NvmmFileUpdated & (NVMEM_UPDATED_DEV_CFG|NVMEM_UPDATED_EXO_CFG|NVMEM_UPDATED_MET_CFG))
  {
    NvmmFileUpdated = SmartPlugNvmmFile.NvmmFileUpdated;
    SmartPlugNvmmFile.NvmmFileUpdated &= ~(NVMEM_UPDATED_DEV_CFG|NVMEM_UPDATED_EXO_CFG|NVMEM_UPDATED_MET_CFG);
    SmartPlugNvmmFileCopy = SmartPlugNvmmFile;
  }
  else
  {
    ret_val = 0;
  }
  osi_LockObjUnlock(&g_NvmemLockObj);

  if(0 == ret_val)
  {
    return ret_val;
  }

  iRetVal = sl_FsOpen(pFileName,
                      FS_MODE_OPEN_WRITE,
                      &ulToken,
                      &lFileHandle);
  if(iRetVal < 0)
  {
    DBG_PRINT("Nvmem File write open fail\n\r");
    ret_val = -1;
  }
  else
  {
    iRetVal = sl_FsWrite(lFileHandle,
                (unsigned int)(0),
                (unsigned char *)&SmartPlugNvmmFileCopy, sizeof(SmartPlugNvmmFileCopy));
    if (iRetVal < 0)
    {
      DBG_PRINT("Nvmem File write fail\n\r");
      ret_val = -1;
    }
    else
    {
      ret_val = 1;
      DBG_PRINT("Nvmem Write success\n\r");
    }
  }

  iRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
  if (SL_RET_CODE_OK != iRetVal)
  {
    DBG_PRINT("Nvmem File close fail\n\r");
    ret_val = -1;
  }

  if(ret_val < 0)
  {
     g_SmartplugLedCurrentState = DEVICE_ERROR_IND;
     osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
     SmartPlugNvmmFile.NvmmFileUpdated |= (NvmmFileUpdated & (NVMEM_UPDATED_DEV_CFG|NVMEM_UPDATED_EXO_CFG|NVMEM_UPDATED_MET_CFG));
     osi_LockObjUnlock(&g_NvmemLockObj);
  }

  return ret_val;
}

int ReadSmartPlugNvmemFile( void )
{
  long lFileHandle;
  unsigned long ulToken;
  INT32 iRetVal, ret_val = -1;
  unsigned char *pFileName = (unsigned char*)SMARTPLUG_NVMEM_FILE_NAME;

  //
  // open a user file for reading
  //
  iRetVal = sl_FsOpen(pFileName,
                      FS_MODE_OPEN_READ,
                      &ulToken,
                      &lFileHandle);
  if(iRetVal < 0)
  {
    DBG_PRINT("Nvmem File read open fail\n\r");
    CreatSmartPlugNvmemFile();
    ret_val = -1;
  }
  else
  {
    iRetVal = sl_FsRead(lFileHandle,
                (unsigned int)(0),
                 (unsigned char *)&SmartPlugNvmmFileCopy, sizeof(SmartPlugNvmmFileCopy));

    if ((iRetVal < 0) || (iRetVal != sizeof(SmartPlugNvmmFileCopy)))
    {
      DBG_PRINT("Nvmem File read fail\n\r");
      ret_val = -1;
      g_SmartplugLedCurrentState = DEVICE_ERROR_IND;
    }
    else
    {
      ret_val = 1;
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      SmartPlugNvmmFile = SmartPlugNvmmFileCopy;
      osi_LockObjUnlock(&g_NvmemLockObj);
      DBG_PRINT("Nvmem Read success\n\r");
    }
    //
    // close the user file
    //
    iRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
    if (SL_RET_CODE_OK != iRetVal)
    {
      DBG_PRINT("Nvmem File close fail\n\r");
      ret_val = -1;
      g_SmartplugLedCurrentState = DEVICE_ERROR_IND;
    }
  }

  return ret_val;
}

int CreatSmartPlugNvmemFile( void )
{
  long lFileHandle;
  //unsigned long ulToken;
  INT32 iRetVal;
  unsigned char *pFileName = (unsigned char*)SMARTPLUG_NVMEM_FILE_NAME;

  //
  //  create a smartplug nvmem file file
  //
  iRetVal = sl_FsOpen(pFileName,FS_MODE_OPEN_CREATE(1024,(_FS_FILE_OPEN_FLAG_COMMIT|_FS_FILE_PUBLIC_WRITE|_FS_FILE_PUBLIC_READ)) ,NULL, &lFileHandle);

  if(iRetVal < 0)
  {
    //
    // File may already be created
    //
    iRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
    {
      DBG_PRINT("Nvmem File create error \n\r");
      return -1;
    }
  }
  else
  {
    //
    // close the user file
    //
    iRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
    if (SL_RET_CODE_OK != iRetVal)
    {
      g_SmartplugLedCurrentState = DEVICE_ERROR_IND;
      DBG_PRINT("Nvmem File create fail\n\r");
      return -1;
    }
    else
    {
      DBG_PRINT("Nvmem File created\n\r");
      return 0;
    }
  }
}

int UpdateExositeCA(unsigned char *InBuff, unsigned short int length)
{
  long lFileHandle;
  unsigned long ulToken;
  INT32 iRetVal = 1, ret_val = -1;
  unsigned char *pFileName = (unsigned char*)"/cert/exositeca.der";

  /* File already present, written through flashing */
  if(iRetVal > 0)
  {
    iRetVal = sl_FsOpen(pFileName,
                        FS_MODE_OPEN_WRITE,
                        &ulToken,
                        &lFileHandle);
    if(iRetVal < 0)
    {
      DBG_PRINT("CA Nvmem File write open fail\n\r");
      ret_val = -1;
    }
    else
    {
      iRetVal = sl_FsWrite(lFileHandle,
                  (unsigned int)(0),
                  (unsigned char *)InBuff, length);
      if (iRetVal < 0)
      {
        DBG_PRINT("CA Nvmem File write fail\n\r");
        ret_val = -1;
      }
      else
      {
        ret_val = 1;
        DBG_PRINT("CA Nvmem Write success\n\r");
      }
    }

    iRetVal = sl_FsClose(lFileHandle, 0, 0, 0);
    if (SL_RET_CODE_OK != iRetVal)
    {
      DBG_PRINT("CA Nvmem File close fail\n\r");
      ret_val = -1;
    }
  }

  if(ret_val < 0)
  {
    g_SmartplugLedCurrentState = DEVICE_ERROR_IND;
  }

  return ret_val;
}

unsigned char ComputeCheckSum(unsigned char *buff, unsigned short int size)
{
  UINT8 checksum = 0;
  UINT16 index = 0;

  for(index = 0; index < size; index++)
  {
    checksum ^= buff[index];
  }
  return checksum;
}


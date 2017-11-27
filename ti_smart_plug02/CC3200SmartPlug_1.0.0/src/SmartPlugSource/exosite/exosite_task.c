//*****************************************************************************
// File: exosite_task.c
//
// Description: Exosite tasks handler of Smartplug gen-1 application
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
#include "uart_logger.h"
#include "cc3200.h"
#include "exosite_hal.h"
#include "exosite_task.h"
#include "exosite_meta.h"
#include "metrology.h"
#include "smartplug_task.h"
#include "android_task.h"
#include "exosite.h"
#include "clienthandler.h"
#include "nvmem_api.h"

#define DBG_PRINT               Report

extern OsiMsgQ_t           ExositeTaskMsgQ;
extern t_SmartplugErrorRecovery g_SmartplugErrorRecovery;
extern t_SmartPlugNvmmFile SmartPlugNvmmFile;
extern OsiLockObj_t        g_NvmemLockObj;
s_ExoConnectionStatus  ExositeConStatus;

UINT32 lCnt = 0;
void ExositeSendTask(void *pvParams)
{
  t_SmartPlugSendData SmartPlugSendData;
  UINT32 MetrologyCount,TimeStamp, DataSender;

  while (1)
  {
    lCnt = 1;
    osi_MsgQRead(&ExositeTaskMsgQ,&SmartPlugSendData,OSI_WAIT_FOREVER);
    lCnt = 2;

    osi_LockObjLock(&ExositeConStatus.g_ClientLockObj, SL_OS_WAIT_FOREVER);
    g_SmartplugErrorRecovery.ExositeTxThreadError = 0;//clear counter

    if(GetCC3200State() & CC3200_GOOD_INTERNET)
    {
      if (EXO_STATUS_OK == Exosite_StatusCode())
      {
        memcpy((void *)&MetrologyCount,(const void *)&SmartPlugSendData.SmartPlugMetrologyData.MetrologyCount[0],4);
        memcpy((void *)&TimeStamp,(const void *)&SmartPlugSendData.SmartPlugMetrologyData.UpdateTimeServer[0],4);
        memcpy((void *)&DataSender,(const void *)&SmartPlugSendData.DataSender,4);

        DBG_PRINT("Exosite: Time=%d,Metrolgy=%d, data=%x\n\r",TimeStamp,MetrologyCount,DataSender);

        /* First Transmit all data */
        if(DataSender & SEND_DEVICE_STATUS_SERVER)
        {
          SendDeviceStatusToServer();
        }

        if(DataSender & SEND_THRESHOLD_PW_WARNING_SERVER)
        {
           sendPowerWarningToServer();
        }

        if(DataSender & SEND_THRESHOLD_EN_WARNING_SERVER)
        {
           sendEnergyWarningToServer();
        }

        if(DataSender & SEND_METROLOGY_DATA_SERVER)
        {
           SendMetrologyDataToServer(&SmartPlugSendData);
        }

        if(DataSender & SEND_AVERAGE_POWER_SERVER)
        {
           SendAvrgEnergyToServer(&SmartPlugSendData);
        }

        if(DataSender & SEND_SCHEDULE_TABLE_SERVER)
        {
           SendScheduleTableToServer();
        }

        //UnsetCC3200MachineState(CC3200_GOOD_TCP|CC3200_GOOD_INTERNET);//DKS test remove
      }
    }

    lCnt = 4;

    osi_LockObjUnlock(&ExositeConStatus.g_ClientLockObj);
    g_SmartplugErrorRecovery.ExositeTxThreadError = 0;//clear counter //DKS test
  }
}

void ExositeRecvTask(void *pvParams)
{
  while (1)
  {
    osi_LockObjLock(&ExositeConStatus.g_ClientLockObj, SL_OS_WAIT_FOREVER);
    g_SmartplugErrorRecovery.ExositeRxThreadError = 0;//clear counter

    if(GetCC3200State() & CC3200_GOOD_TCP)
    {
      if(GetCC3200State() & CC3200_GOOD_INTERNET)
      {
        if (EXO_STATUS_OK == Exosite_StatusCode())
        {
          ReceiveControlCmd();
        }
        else
        {
          DBG_PRINT("Exosite : Checking the CIK\n\r");
          if (!Exosite_Activate())
          {
            ExositeCIKFailLEDInd();
            osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
            SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer = 0;
            SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer = 0;
            osi_LockObjUnlock(&g_NvmemLockObj);

            DBG_PRINT("Exosite : CIK invalid!\n\r");
            OSI_DELAY(500);//wait 500ms
          }
          else
          {
            DBG_PRINT("Exosite : Activated Success!\n\r");
            osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
            SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_DATA_SEND_DEV_CFG|NVMEM_DATA_SEND_SCHEDULE_TABLE);//sync
            SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer = 1;
            SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer = 1;
            osi_LockObjUnlock(&g_NvmemLockObj);
          }
        }
      }
      else
      {
        int status;

        /* Close socket */
        if(ExositeConStatus.g_ClientSoc >= 0)
        {
          exoHAL_SocketClose(ExositeConStatus.g_ClientSoc);
        }

        /* Set NWP time from internet connection once internet is up for ssl validation */
        status = SetNwpTimeFromInternet();

        if(status < 0)
        {
          UnsetCC3200MachineState(CC3200_GOOD_INTERNET);
        }

        if(!(GetCC3200State() & CC3200_GOOD_INTERNET))
        {
          /* Clear flag if exosite is not up */
          osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
          SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer = 0;
          SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer = 0;
          osi_LockObjUnlock(&g_NvmemLockObj);

          DBG_PRINT("Bad internet\n\r");
          OSI_DELAY(1000);//wait 1sec
        }
        else
        {
          osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
          SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_DATA_SEND_DEV_CFG|NVMEM_DATA_SEND_SCHEDULE_TABLE);//sync
          SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer = 1;
          SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer = 1;
          osi_LockObjUnlock(&g_NvmemLockObj);
        }
      }
      lCnt = 4;
    }

    if(!(GetCC3200State() & CC3200_GOOD_TCP))
    {
      if(ExositeConStatus.g_ClientSoc >= 0)
      {
        exoHAL_SocketClose(ExositeConStatus.g_ClientSoc);

        osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
        (*(UINT16*)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0]) &= (~EXO_STATUS_VALID_CONN);
        SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_DATA_SEND_EXO_CFG;
        osi_LockObjUnlock(&g_NvmemLockObj);
      }

      /* Dummy wait 1sec */
      OSI_DELAY(1000);//wait 1sec
    }
    osi_LockObjUnlock(&ExositeConStatus.g_ClientLockObj);
    g_SmartplugErrorRecovery.ExositeRxThreadError = 0;//clear counter
  }
}


int SetNwpTimeFromInternet()
{
    SlDateTime_t    nwp_time;
    t_smartplugdate SmartPlugDate;
    long retVal;

    osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
    SmartPlugDate = SmartPlugNvmmFile.DeviceConfigData.SmartPlugDate;
    osi_LockObjUnlock(&g_NvmemLockObj);

    Report("Waiting to set NWP time from internet\n\r" );

    /* connect exosite unsecure */
    /* request timestamp */
    /* decode time from http response */
    retVal = Exosite_Time_Read(&SmartPlugDate);

    if(retVal > 0)
    {
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      SmartPlugNvmmFile.DeviceConfigData.SmartPlugDate = SmartPlugDate;
      osi_LockObjUnlock(&g_NvmemLockObj);

      if(0 == SmartPlugDate.validity)
      {
        osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
        nwp_time.sl_tm_day  = SmartPlugNvmmFile.DeviceConfigData.SmartPlugDate.date;
        nwp_time.sl_tm_mon  = SmartPlugNvmmFile.DeviceConfigData.SmartPlugDate.month;
        nwp_time.sl_tm_year = SmartPlugNvmmFile.DeviceConfigData.SmartPlugDate.year;
        nwp_time.sl_tm_hour = SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.hour;
        nwp_time.sl_tm_min  = SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.min;
        nwp_time.sl_tm_sec  = 0;
        osi_LockObjUnlock(&g_NvmemLockObj);

        retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                              SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                              sizeof(nwp_time),(unsigned char *)(&nwp_time));

        if(retVal >= 0)
        {
          osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
          SmartPlugNvmmFile.DeviceConfigData.SmartPlugDate.validity = 1;
          SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_UPDATED_DEV_CFG;
          osi_LockObjUnlock(&g_NvmemLockObj);
          Report("NWP time updated\n\r" );
          return 1;
        }
      }
      else
      {
        return 1;
      }
    }

    return -1;
}






















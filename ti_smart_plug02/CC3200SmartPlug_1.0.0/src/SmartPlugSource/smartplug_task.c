//*****************************************************************************
// File: smartplug_task.c
//
// Description: SmartPlug tasks handler functions of Smartplug gen-1 application
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
#include "smartconfig.h"
#include "exosite_task.h"
#include "exosite_meta.h"
#include "exosite_hal.h"
#include "metrology.h"
#include "smartplug_task.h"
#include "android_task.h"
#include "exosite.h"
#include "clienthandler.h"
#include "gpio_if.h"

/****************************************************************************/
/*        MACROS                    */
/****************************************************************************/

/****************************************************************************
                              Global variables
****************************************************************************/
extern s_ConnectionStatus  g_ConnectionState;
t_SmartPlugNvmmFile SmartPlugNvmmFile;
UINT8 g_BadTCPCount = 0, g_BadNWPCount = 0;

t_SmartplugErrorRecovery g_SmartplugErrorRecovery;
extern volatile unsigned long g_ulDeviceTimerInSec;
extern t_AndroidClientRxBuff   g_ClientRxbuff;
extern s_ExoConnectionStatus ExositeConStatus;
t_mDNSService mDNSService;
extern unsigned char g_MACAddress[6];
UINT8 g_SmartplugInitDone = 0;
volatile UINT8 g_NwpInitdone = 0;
extern volatile LedIndicationName  g_SmartplugLedCurrentState;

extern volatile char g_SmartPlugWps;
extern volatile char g_SmartPlugSmartconf;
extern volatile char g_SmartPlugAPProv;
extern volatile UINT32 g_SmartconfButtonSense;
extern volatile UINT8 g_MetrologyCalEn;
extern volatile UINT8 g_MetroCalCount;
extern float AvgRmsNoise;
extern UINT16 g_CABinarySize;
UINT8 g_ExoPriorityInverse = 0;
extern t_MetrologyDebugParams MetrologyDebugParams;

#define DBG_PRINT               Report

/****************************************************************************
                              Synch Objects
****************************************************************************/
OsiLockObj_t  g_NvmemLockObj;
OsiMsgQ_t     SmartPlugTaskMsgQ;
OsiMsgQ_t     ExositeTaskMsgQ;
OsiMsgQ_t     TimeHandlerTaskMsgQ;
extern OsiMsgQ_t    AndroidClientTaskMsgQ;
extern OsiLockObj_t  g_UartLockObj;

OsiTaskHandle AndroidClientTaskHndl;
OsiTaskHandle ExositeRecvTaskHndl;
OsiTaskHandle ExositeSendTaskHndl;
OsiTaskHandle AndroidClientRecvTaskHndl;

//****************************************************************************
//
//! Task function start the device and crete a TCP server showcasing the smart
//! plug
//!
//****************************************************************************
void TimeHandlerTask(void * param)
{
  UINT32 TimeStamp;

  while(1)
  {
    /* Wait for 1sec time stamp */
    osi_MsgQRead(&TimeHandlerTaskMsgQ,&TimeStamp,OSI_WAIT_FOREVER);

    //Report("Timer task time %d \n\r", TimeStamp);

    /* If NWP is not initialized then do not perfrom anything */
    if(0 == g_NwpInitdone)
    {
      continue;
    }

    /* Check status of relay and update in high priority */
    if(1 == GetRelayState())
    {
      UINT8 Relay_state;

      /* Override relay state */
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      if(1 == SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.Device_status)
      {
        Relay_state = 0;
      }
      else
      {
        Relay_state = 1;
      }
      SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer = 0;
      SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToAndrid = 0;
      osi_LockObjUnlock(&g_NvmemLockObj);

      /* Update based on input */
      UpdateDevStatus(Relay_state);
    }

    /* Manage time & schedule table every 30 secs */
    if(TimeStamp != 0)
    {
      if((0 == (TimeStamp % 30)) || (1 == SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer)||\
          (1 == SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToAndrid))
      {
        ManageSmartplugTime();

        if(1 == SmartPlugNvmmFile.DeviceConfigData.Schedule.Validity)
        {
          /* Perform schedule table device control */
          ManageScheduleTable();
        }
      }
    }

    /* In priority write to NVMEM if any */
    WriteSmartPlugNvmemFile();

    if(TimeStamp != 0)
    {
      /* Trigger 1sec smartplug task */
      osi_MsgQWrite(&SmartPlugTaskMsgQ,&TimeStamp,OSI_NO_WAIT);
    }
  }
}

void SmartPlugTask(void * param)
{
  UINT8 ResetNwp = 0;
  UINT8 IntervalServer, IntervalAndroid;
  UINT32 TimeStamp, Mask;
  LedIndicationName  PrevLedCurrentState;

  /* Init Smart plug first time - initiates all threads */
  InitSmartPlug();

  while (1)
  {
    /* Wait for time stamp */
    osi_MsgQRead(&SmartPlugTaskMsgQ,&TimeStamp,OSI_WAIT_FOREVER);

    /* Clear WDT every sec */
    ClearWDT();

    Report("smartplug task time %d \n\r", TimeStamp);

    Report("V-DC offset %f \n\r", MetrologyDebugParams.VChDCOffset);
    Report("I-DC offset %f \n\r", MetrologyDebugParams.IChDCOffset);
    Report("V Raw-rms %f \n\r", MetrologyDebugParams.RawRmsVoltage);
    Report("I Raw-rms %f \n\r", MetrologyDebugParams.RawRmsCurrent);
    Report("Pactive Raw %f \n\r", MetrologyDebugParams.RawActivePower);
    Report("AC Freq %f \n\r", MetrologyDebugParams.ACFrequency);

    Report("Voltage RMS %f \n\r", MetrologyDebugParams.RmsVoltage);
    Report("Current RMS %f \n\r", MetrologyDebugParams.RmsCurrent);
    Report("Active Power %f \n\r", MetrologyDebugParams.TrueEnergy);
    Report("Apperent Power %f \n\r", MetrologyDebugParams.ApparentEnergy);
    Report("Reactive Power %f \n\r", MetrologyDebugParams.ReactiveEnergy);
    Report("Power factor %f \n\r", MetrologyDebugParams.PowerFactor);

    /* Clear these in every 1 sec interval */
    ResetNwp = 0;
    g_SmartconfButtonSense = 0;
    g_SmartPlugAPProv      = 0;
    g_SmartPlugSmartconf   = 0;
    g_SmartPlugWps         = 0;

    /* Perform Metrology module integrity check */
    if(UpdateStatusOfMetrologyModule() < 0)
    {
      g_SmartplugErrorRecovery.MetrologyModuleError++;//DKS test
    }
    else
    {
      g_SmartplugErrorRecovery.MetrologyModuleError = 0;
      //g_SmartplugErrorRecovery.MetrologyModuleError++;//DKS test
    }

    /* If Metrology Module not responding for 10secs reset board */
    if(g_SmartplugErrorRecovery.MetrologyModuleError >= MAX_METROLOGY_MODULE_ERROR)
    {
      g_SmartplugLedCurrentState = DEVICE_ERROR_IND;
      //HIB reset
      Report("FATAL:Metrology Module Error HIB Reset\n\r");
      OSI_DELAY(750);//wait 750ms
      HIBReset();//DKS test
      //while(1);//DKS test
      //ResetNwp = 1;//DKS test
      //g_SmartplugErrorRecovery.MetrologyModuleError = 0;//DKS test
    }

    /* If IP is allocated initiate smartplug wireless activity*/
    if(GetCC3200State() & CC3200_GOOD_TCP)
    {
      SmartPlug1SecTask();
    }
    else
    {
      PrevLedCurrentState = g_SmartplugLedCurrentState;
      g_SmartplugLedCurrentState = DEVICE_ERROR_IND;
      /* Bad TCP */
      Report("Bad TCP \n\r");

      /* Wait till all sockets closed */
      OSI_DELAY(1000);//wait 2sec
      ClearWDT();
      g_SmartplugLedCurrentState = DEVICE_ERROR_IND;
      OSI_DELAY(1000);

      /* Check again for bad TCP */
      if(!(GetCC3200State() & CC3200_GOOD_TCP))
      {
        ClearWDT();
        /* If NWP is up perform WLAN disconnect & connect */
        if(((ExositeConStatus.g_ClientSoc < 0) && (g_ConnectionState.g_ClientSD < 0) &&\
          (g_ConnectionState.ServerSoc < 0)) && (g_BadTCPCount < MAX_BAD_TCP_NWP))
        {
          //
          // Disconnect from the AP
          //
          WlanDisconnect();

          /* Connect to default AP */
          ConnectToAP();

          osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
          SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_DATA_SEND_DEV_CFG|NVMEM_DATA_SEND_EXO_CFG|NVMEM_DATA_SEND_SCHEDULE_TABLE);//sync
          SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToAndrid = 1;
          SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer = 1;
          SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer = 1;
          SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToAndrid = 1;
          osi_LockObjUnlock(&g_NvmemLockObj);

          g_SmartplugErrorRecovery.ExositeTxThreadError = 0;
          g_SmartplugErrorRecovery.ExositeRxThreadError = 0;
          g_SmartplugErrorRecovery.AndroidTxThreadError = 0;
          g_SmartplugErrorRecovery.AndroidRxThreadError = 0;
          g_SmartplugErrorRecovery.BadTCPErrorCheckCount = 0;
          g_SmartplugErrorRecovery.MdnsError = 0;

          g_BadTCPCount++;
        }
        else
        {
          /* NWP is not responding, perform NWP reset */
          ResetNwp = 1;
          Report("TCP error count %d \n\r",g_BadTCPCount);
        }
      }
      else if(DEVICE_ERROR_IND == g_SmartplugLedCurrentState)
      {
        g_SmartplugLedCurrentState = PrevLedCurrentState;
      }
    }

    /* Error recovery check */
    g_SmartplugErrorRecovery.ExositeTxThreadError++;
    g_SmartplugErrorRecovery.ExositeRxThreadError++;
    g_SmartplugErrorRecovery.AndroidTxThreadError++;
    g_SmartplugErrorRecovery.AndroidRxThreadError++;
    g_SmartplugErrorRecovery.BadTCPErrorCheckCount++;


    if(g_SmartplugErrorRecovery.BadTCPErrorCheckCount >= MAX_BAD_TCP_ERROR_CHECK_TIME)
    {
      g_SmartplugErrorRecovery.BadTCPErrorCheckCount = 0;
      g_BadTCPCount = 0;
      g_BadNWPCount = 0;
    }

    osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
    IntervalServer = SmartPlugNvmmFile.DeviceConfigData.updateinterval.IntervalServer;
    IntervalAndroid = SmartPlugNvmmFile.DeviceConfigData.updateinterval.Interval;
    osi_LockObjUnlock(&g_NvmemLockObj);

    if(GetCC3200State() & CC3200_GOOD_INTERNET)
    {
      if(g_SmartplugErrorRecovery.ExositeRxThreadError >= MAX_EXOSITE_RX_THREAD_WAIT_TIME)
      {
        if(0 == g_ExoPriorityInverse)
        {
          Report("ExoRx Thread block %d\n\r",g_SmartplugErrorRecovery.ExositeRxThreadError);

          /* Reverse priority of Rx & Tx */
          g_ExoPriorityInverse = 1;
          Mask = osi_TaskDisable();
          osi_SetTaskPriority( &ExositeSendTaskHndl, EXOSITE_RECV_TASK_PRIO );
          osi_SetTaskPriority( &ExositeRecvTaskHndl, EXOSITE_SEND_TASK_PRIO );
          osi_TaskEnable(Mask);
        }
      }
      else if(1 == g_ExoPriorityInverse)
      {
        /* Reverse priority of Rx & Tx */
        g_ExoPriorityInverse = 0;
        Mask = osi_TaskDisable();
        osi_SetTaskPriority( &ExositeRecvTaskHndl, EXOSITE_RECV_TASK_PRIO );
        osi_SetTaskPriority( &ExositeSendTaskHndl, EXOSITE_SEND_TASK_PRIO );
        osi_TaskEnable(Mask);

        Report("ExoRx Thread Unblock %d\n\r",g_SmartplugErrorRecovery.ExositeRxThreadError);
      }
    }
    else
    {
      if(1 == g_ExoPriorityInverse)
      {
        /* Reverse priority of Rx & Tx */
        g_ExoPriorityInverse = 0;
        Mask = osi_TaskDisable();
        osi_SetTaskPriority( &ExositeRecvTaskHndl, EXOSITE_RECV_TASK_PRIO );
        osi_SetTaskPriority( &ExositeSendTaskHndl, EXOSITE_SEND_TASK_PRIO );
        osi_TaskEnable(Mask);

        Report("ExoRx Thread Unblock %d\n\r",g_SmartplugErrorRecovery.ExositeRxThreadError);
      }
      /* Clear flag if exosite is not up */
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
      SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer = 0;
      SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer = 0;
      osi_LockObjUnlock(&g_NvmemLockObj);
    }

    if(g_SmartplugErrorRecovery.ExositeTxThreadError >= (MAX_EXOSITE_TX_THREAD_ERROR+IntervalServer))
    {
      ResetNwp = 1;
      Report("FATAL:ExoTx Thread Error %d\n\r",g_SmartplugErrorRecovery.ExositeTxThreadError);
    }
    if(g_SmartplugErrorRecovery.ExositeRxThreadError >= MAX_EXOSITE_RX_THREAD_ERROR)
    {
      ResetNwp = 1;
      Report("FATAL:ExoRx Thread Error %d\n\r",g_SmartplugErrorRecovery.ExositeRxThreadError);
    }
    if(g_SmartplugErrorRecovery.AndroidTxThreadError >= (MAX_ANDROID_TX_THREAD_ERROR+IntervalAndroid))
    {
      ResetNwp = 1;
      Report("FATAL:AndTx Thread Error %d\n\r",g_SmartplugErrorRecovery.AndroidTxThreadError);
    }
    if(g_SmartplugErrorRecovery.AndroidRxThreadError >= MAX_ANDROID_RX_THREAD_ERROR)
    {
      ResetNwp = 1;
      Report("FATAL:AndRx Thread Error %d\n\r",g_SmartplugErrorRecovery.AndroidRxThreadError);
    }
    if(g_SmartplugErrorRecovery.MdnsError >= MAX_MDNS_ERROR)
    {
      ResetNwp = 1;
      Report("FATAL:mDNS Error %d\n\r",g_SmartplugErrorRecovery.MdnsError);
    }

    if((ResetNwp) && (g_BadNWPCount < MAX_BAD_TCP_NWP))
    {
      //NWP reset
      Report("FATAL:SW Error NWP Reset %d\n\r", g_BadNWPCount);
      /* Stop smartplug - NWP reset */
      DeInitSmartPlug();
      /* Wait some time */
      OSI_DELAY(500);//wait 500ms
      ClearWDT();
      g_SmartplugLedCurrentState  = DEVICE_POW_ON_IND;
      Report("Init Smartplug again... \n\r");
      /* Restart Smartplug - NWP ON */
      InitSmartPlug();
      g_BadNWPCount++;
    }
    else if(g_BadNWPCount >= MAX_BAD_TCP_NWP)
    {
      g_SmartplugLedCurrentState = DEVICE_ERROR_IND;
      //HIB reset
      Report("FATAL:TCP/NWP error HIB Reset %d\n\r", g_BadNWPCount);
      OSI_DELAY(750);//wait 750ms
      HIBReset();
    }
  }
}

void SmartPlug1SecTask( void )
{
  UINT32 CriticalMask;
  UINT32 TimeStamp, APTime, DataSender = 0;
  UINT8  IntervalServer = 0, IntervalAndroid = 0;
  int Ret_val = 0;
  t_SmartPlugSendData SmartPlugSendData;

  /* Read Latest Time stamp in critical region */
  CriticalMask = osi_EnterCritical();
    TimeStamp = g_ulDeviceTimerInSec;
  osi_ExitCritical(CriticalMask);

  memcpy((void *)&APTime,(const void *)&pMetrologyDataComm->APUpdateCount[0],4);
  if(TimeStamp >= (APTime + M_ONE_HOUR_IN_SEC))
  //if(TimeStamp >= (APTime + 5))//DKS test change later send every 5sec
  {
    UINT32 Previous, Total, Average;

    // Update metrology data to every client
    if(0 == g_SmartplugErrorRecovery.MetrologyModuleError)
    {
      Ret_val = UpdateMetrologyDataForComm();

      memcpy((void *)&Previous,(const void *)&pMetrologyDataComm->EnergyUptoLastHour[0],4);
      memcpy((void *)&Total,(const void *)&pMetrologyDataComm->TrueEnergyConsumed[0],4);

      Average = Total - Previous;//average KWh

      memcpy((void *)&pMetrologyDataComm->AveragePower[0], (void *)&Average, 4);
      memcpy((void *)&pMetrologyDataComm->AveragePower24Hr[pMetrologyDataComm->AP24HrCount][0], (void *)&Average, 4);
      pMetrologyDataComm->AP24HrCount++;
      if(pMetrologyDataComm->AP24HrCount >= 24)
      {
        pMetrologyDataComm->AP24HrCount = 0;
        pMetrologyDataComm->AP24HrRollover = 1;
      }
      memcpy((void *)&pMetrologyDataComm->EnergyUptoLastHour[0], (void *)&pMetrologyDataComm->TrueEnergyConsumed[0], 4);

      memcpy((void *)&pMetrologyDataComm->APUpdateCount[0],(const void *)&TimeStamp,4);
      memcpy((void *)&pMetrologyDataComm->UpdateTime[0],(const void *)&TimeStamp,4);
      memcpy((void *)&pMetrologyDataComm->UpdateTimeServer[0],(const void *)&TimeStamp,4);

      // Send Average power per hour
      DataSender |= SEND_AVERAGE_POWER_SERVER;
      DataSender |= SEND_AVERAGE_POWER;
      DataSender |= SEND_METROLOGY_DATA_SERVER;
      DataSender |= SEND_METROLOGY_DATA;
    }
  }
  else
  {
    osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
    IntervalServer = SmartPlugNvmmFile.DeviceConfigData.updateinterval.IntervalServer;
    IntervalAndroid = SmartPlugNvmmFile.DeviceConfigData.updateinterval.Interval;
    osi_LockObjUnlock(&g_NvmemLockObj);

    memcpy((void *)&APTime,(const void *)&pMetrologyDataComm->UpdateTimeServer[0],4);

    if(TimeStamp >= (APTime + IntervalServer))
    {
      // Update metrology data to every client
      if(0 == g_SmartplugErrorRecovery.MetrologyModuleError)
      {
        Ret_val = UpdateMetrologyDataForComm();

        DataSender |= SEND_METROLOGY_DATA_SERVER;
        memcpy((void *)&pMetrologyDataComm->UpdateTimeServer[0],(const void *)&TimeStamp,4);
        //Report("updatetime test: %d, %d\n\r",TimeStamp, *((UINT32*)&pMetrologyDataComm->UpdateTimeServer[0]));
      }
      else
      {
        Ret_val = -1;
      }
    }

    memcpy((void *)&APTime,(const void *)&pMetrologyDataComm->UpdateTime[0],4);

    if(TimeStamp >= (APTime + IntervalAndroid))
    {
      // Update metrology data to every client
      if(0 == Ret_val)
      {
        if(0 == g_SmartplugErrorRecovery.MetrologyModuleError)
        {
          Ret_val = UpdateMetrologyDataForComm();
        }
      }

      if(Ret_val > 0)
      {
        DataSender |= SEND_METROLOGY_DATA;
        memcpy((void *)&pMetrologyDataComm->UpdateTime[0],(const void *)&TimeStamp,4);
      }
    }
  }
  SmartPlugSendData.SmartPlugMetrologyData = *pMetrologyDataComm;

  /* Perform data copy in safe region */
  osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);

  //Report("NvmmFileUpdated = %x, %d, %d \n\r", SmartPlugNvmmFile.NvmmFileUpdated, \
  //SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer, SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer);

  if(SmartPlugNvmmFile.NvmmFileUpdated & NVMEM_DATA_SEND_DEV_CFG)
  {
    DataSender |= SEND_DEVICE_STATUS;
    DataSender |= SEND_DEVICE_STATUS_SERVER;
    SmartPlugNvmmFile.NvmmFileUpdated &= ~(NVMEM_DATA_SEND_DEV_CFG);
  }
  else
  {
    if(SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToAndrid == 1)
    {
      DataSender |= SEND_DEVICE_STATUS;
    }
    if(SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer == 1)
    {
      DataSender |= SEND_DEVICE_STATUS_SERVER;
    }
  }

  SmartPlugSendData.DeviceConfigData = SmartPlugNvmmFile.DeviceConfigData;

  if(SmartPlugNvmmFile.NvmmFileUpdated & NVMEM_DATA_SEND_SCHEDULE_TABLE)
  {
    DataSender |= SEND_SCHEDULE_TABLE;
    DataSender |= SEND_SCHEDULE_TABLE_SERVER;
    SmartPlugNvmmFile.NvmmFileUpdated &= ~(NVMEM_DATA_SEND_SCHEDULE_TABLE);
  }
  else
  {
    if(SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToAndrid == 1)
    {
      DataSender |= SEND_SCHEDULE_TABLE;
    }
    if(SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer == 1)
    {
      DataSender |= SEND_SCHEDULE_TABLE_SERVER;
    }
  }

  if(SmartPlugNvmmFile.NvmmFileUpdated & (NVMEM_DATA_SEND_24HR_ENERGY))
  {
    DataSender |= SEND_24HR_ENERGY;
    SmartPlugNvmmFile.NvmmFileUpdated &= ~(NVMEM_DATA_SEND_24HR_ENERGY);
  }
  if(SmartPlugNvmmFile.NvmmFileUpdated & (NVMEM_DATA_SEND_EXO_SSL_CA_DONE))
  {
    DataSender |= SEND_EXO_SSL_CA_DONE;
    SmartPlugNvmmFile.NvmmFileUpdated &= ~(NVMEM_DATA_SEND_EXO_SSL_CA_DONE);
  }

  if(SmartPlugNvmmFile.NvmmFileUpdated & (NVMEM_DATA_SEND_EXO_CFG))
  {
    DataSender |= SEND_EXOSITE_CFG;
    SmartPlugNvmmFile.NvmmFileUpdated &= ~(NVMEM_DATA_SEND_EXO_CFG);
  }
  memcpy((void *)&SmartPlugSendData.ExositeMetaData.vendorname[0], (void *)&SmartPlugNvmmFile.ExositeMetaData.vendorname[0], META_VNAME_SIZE);
  SmartPlugSendData.ExositeMetaData.vendorname[META_VNAME_SIZE] = 0;
  memcpy((void *)&SmartPlugSendData.ExositeMetaData.modelname[0], (void *)&SmartPlugNvmmFile.ExositeMetaData.modelname[0], META_MNAME_SIZE);
  SmartPlugSendData.ExositeMetaData.modelname[META_MNAME_SIZE] = 0;
  memcpy((void *)&SmartPlugSendData.ExositeMetaData.exostatus[0], (void *)&SmartPlugNvmmFile.ExositeMetaData.exo_status[0], META_EXO_STS_SIZE);
  memcpy((void *)&SmartPlugSendData.ExositeMetaData.mac_address[0], (void *)&g_MACAddress[0], 6);

  if(SmartPlugNvmmFile.NvmmFileUpdated & (NVMEM_DATA_SEND_MET_CFG))
  {
    DataSender |= SEND_METROLOGY_CFG;
    SmartPlugNvmmFile.NvmmFileUpdated &= ~(NVMEM_DATA_SEND_MET_CFG);
  }
  SmartPlugSendData.MetrologyConfigData = SmartPlugNvmmFile.MetrologyConfigData;
  osi_LockObjUnlock(&g_NvmemLockObj);

  if((DataSender & SEND_METROLOGY_DATA_SERVER)||(DataSender & SEND_METROLOGY_DATA))
  {
    float  *ThreasholdPw, *ThreasholdEn;
    UINT32 *power, *energy;
    float power_val, energy_val;
    float  *ThresholdPwServer, *ThresholdEnServer;

    ThreasholdPw = (float *) &SmartPlugSendData.DeviceConfigData.ThreasholdPower[0];
    ThresholdPwServer = (float *) &SmartPlugSendData.DeviceConfigData.ThresholdPowerServer[0];
    ThreasholdEn = (float *) &SmartPlugSendData.DeviceConfigData.ThreasholdEnergy[0];
    ThresholdEnServer = (float *) &SmartPlugSendData.DeviceConfigData.ThresholdEnergyServer[0];

    energy = (UINT32*)&SmartPlugSendData.SmartPlugMetrologyData.TrueEnergyConsumed[0];//M_SCALE_FACTOR_4_DIGIT format
    energy_val = (float)(*energy)/(float)M_SCALE_FACTOR_4_DIGIT;
    power = (UINT32*)&SmartPlugSendData.SmartPlugMetrologyData.TruePower[0];//M_SCALE_FACTOR_3_DIGIT format
    power_val = (float)(*power)/(float)M_SCALE_FACTOR_3_DIGIT;

    if(*ThreasholdEn != 0)
    {
      if(energy_val >= (*ThreasholdEn))
      {
        DataSender |= SEND_THRESHOLD_EN_WARNING;
      }
    }

    if(*ThresholdEnServer != 0)
    {
      if(energy_val >= (*ThresholdEnServer))
      {
        DataSender |= SEND_THRESHOLD_EN_WARNING_SERVER;
      }
    }

    if(*ThreasholdPw != 0)
    {
      if(power_val >= (*ThreasholdPw))
      {
        DataSender |= SEND_THRESHOLD_PW_WARNING;
      }
    }

    if(*ThresholdPwServer != 0)
    {
      if(power_val >= (*ThresholdPwServer))
      {
        DataSender |= SEND_THRESHOLD_PW_WARNING_SERVER;
      }
    }
  }

  if(g_SmartplugErrorRecovery.MetrologyModuleError >= MAX_METROLOGY_ERROR_REP_INT)//if no metrology data for 5 secs send warning
  {
    DataSender |= SEND_METROLOGY_WARNING;
  }

  memcpy((void *)&SmartPlugSendData.DataSender,(const void *)&DataSender,4);
  SmartPlugSendData.TimeStamp = TimeStamp;//This time stamp required for dev information

  if(DataSender & (SEND_SERVER_EXO_MASK))
  {
    osi_MsgQWrite(&ExositeTaskMsgQ,&SmartPlugSendData,OSI_NO_WAIT);
  }

  if(DataSender & (SEND_ANDROID_MASK))
  {
    osi_MsgQWrite(&AndroidClientTaskMsgQ,&SmartPlugSendData,OSI_NO_WAIT);
  }

  //Report("1sec test: data=%x\n\r",DataSender);
}

//****************************************************************************
//
//! Check the Nvmem for configuration parameters and write default if memory
//! is invalid
//!
//****************************************************************************
//UINT8 TestBuff[20];
void NvmemInit()
{
  signed char NvmmfilePresent = -1;
  float FVal;
  UINT32 *pFval = (UINT32*)&FVal;

  //UpdateExositeCA(&TestBuff[0], 16);

  //
  //  create a smartplug nvmem file
  //
  //NvmmfilePresent = CreatSmartPlugNvmemFile();

  /* If file is present read the nvmem file */
  //if(1 == NvmmfilePresent)
  {
    //
    // read a user file
    //
    NvmmfilePresent = ReadSmartPlugNvmemFile();
  }

  SmartPlugNvmmFile.NvmmFileUpdated = 0;//Clear flag

  //NvmmfilePresent = -1;//DKS test

  if(1 != NvmmfilePresent)
  {
    /* Initialize all data by invalidating */
    memcpy((void *)&SmartPlugNvmmFile.DeviceConfigData.mark[0],INVALIDMARK,INVALID_MARK_SIZE);
    memcpy((void *)&SmartPlugNvmmFile.ExositeMetaData.mark[0],INVALIDMARK,INVALID_MARK_SIZE);
    memcpy((void *)&SmartPlugNvmmFile.MetrologyConfigData.mark[0],INVALIDMARK,INVALID_MARK_SIZE);
  }

  if(memcmp(DEVICEMARK, (const void *)&SmartPlugNvmmFile.DeviceConfigData.mark[0], DEVICE_MARK_SIZE) != 0)
  {
    SmartPlugNvmmFile.DeviceConfigData.DevConfig.DevNameLen = strlen(SMARTPLUG_DEVICE_NAME);
    memcpy((void *)&SmartPlugNvmmFile.DeviceConfigData.DevConfig.DevName[0],SMARTPLUG_DEVICE_NAME,SmartPlugNvmmFile.DeviceConfigData.DevConfig.DevNameLen);
    SmartPlugNvmmFile.DeviceConfigData.DevConfig.DevName[SmartPlugNvmmFile.DeviceConfigData.DevConfig.DevNameLen] = 0;

    SmartPlugNvmmFile.DeviceConfigData.DevConfig.SmartconfSsidLen = 0;
    SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.Device_status = 1;//Relay ON

    FVal = MAX_POWER_WATTS;// max val
    memcpy((void*)&SmartPlugNvmmFile.DeviceConfigData.ThreasholdPower[0],(void*)pFval,4);
    memcpy((void*)&SmartPlugNvmmFile.DeviceConfigData.ThresholdPowerServer[0],(void*)pFval,4);
    FVal = MAX_ENERGY_KWH;// max val
    memcpy((void*)&SmartPlugNvmmFile.DeviceConfigData.ThreasholdEnergy[0],(void*)pFval,4);
    memcpy((void*)&SmartPlugNvmmFile.DeviceConfigData.ThresholdEnergyServer[0],(void*)pFval,4);

    SmartPlugNvmmFile.DeviceConfigData.updateinterval.IntervalServer = DEFAULT_UPDATE_INTERVAL;//1sec
    SmartPlugNvmmFile.DeviceConfigData.updateinterval.Interval = DEFAULT_UPDATE_INTERVAL;//1sec
    SmartPlugNvmmFile.DeviceConfigData.updateinterval.IsPowerSavingServer = 0;
    SmartPlugNvmmFile.DeviceConfigData.updateinterval.IsPowerSaving = 0;

    memset((void*)&SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule, 0, sizeof(t_weekschedule));//24
    SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule.mon_fri.wk_hour = 24;
    SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule.mon_fri.lv_hour = 24;
    SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule.mon_fri.rt_hour = 24;
    SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule.mon_fri.sl_hour = 24;
    SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule.sat.wk_hour = 24;
    SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule.sat.lv_hour = 24;
    SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule.sat.rt_hour = 24;
    SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule.sat.sl_hour = 24;
    SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule.sun.wk_hour = 24;
    SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule.sun.lv_hour = 24;
    SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule.sun.rt_hour = 24;
    SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule.sun.sl_hour = 24;

    SmartPlugNvmmFile.DeviceConfigData.SmartPlugDate.date  = DATE;
    SmartPlugNvmmFile.DeviceConfigData.SmartPlugDate.month = MONTH;
    SmartPlugNvmmFile.DeviceConfigData.SmartPlugDate.year  = YEAR;
    SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.day   = DAY;
    SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.hour  = HOUR;
    SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.min   = MINUTE;

    /* Update validity */
    memcpy((void *)&SmartPlugNvmmFile.DeviceConfigData.mark[0],DEVICEMARK,DEVICE_MARK_SIZE);
    SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_UPDATED_DEV_CFG;//update flag indicating NVMEM write required
  }

  //Perform Relay ON/OFF based on existing settings
  if(1 == SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.Device_status)
  {
    TurnOnDevice();
  }
  else
  {
    TurnOffDevice();
  }

  g_MetrologyCalEn = 0;
  if(memcmp(METROLOGYMARK, (const void *)&SmartPlugNvmmFile.MetrologyConfigData.mark[0], METROLOGY_MARK_SIZE) != 0)
  {
    FVal = M_RMS_NOISE_IN_V_CH;
    memcpy((void *)&SmartPlugNvmmFile.MetrologyConfigData.RmsNoiseVCh[0], (const void *)pFval, 4);
    FVal = M_RMS_NOISE_IN_I_CH;
    memcpy((void *)&SmartPlugNvmmFile.MetrologyConfigData.RmsNoiseICh[0], (const void *)pFval, 4);
    FVal = M_V_CH_SCALE_FATOR;
    memcpy((void *)&SmartPlugNvmmFile.MetrologyConfigData.VChScaleFactor[0], (const void *)pFval, 4);
    FVal = M_I_CH_SCALE_FATOR;
    memcpy((void *)&SmartPlugNvmmFile.MetrologyConfigData.IChScaleFactor[0], (const void *)pFval, 4);

    /* Update validity */
    memcpy((void *)&SmartPlugNvmmFile.MetrologyConfigData.mark[0],METROLOGYMARK,METROLOGY_MARK_SIZE);

    SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_UPDATED_MET_CFG;//update flag indicating NVMEM write required

    /* Trigger calibration if nvmm data does not present */
    //#ifdef SMARTPLUG_HW
    #if 0 //DKS not required as smartplug can not use below 10W due to noise in current channel.
    if(1 == SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.Device_status)
    {
      TurnOffDevice();
    }
    g_MetrologyCalEn = 1;
    #endif

    g_MetroCalCount = 0;
    AvgRmsNoise = 0;

    /* Disable calibration after AP config */
  }

  //Init Metrology Module
  InitMetrologyModule();

  if(memcmp(EXOMARK, (const void *)&SmartPlugNvmmFile.ExositeMetaData.mark[0], META_MARK_SIZE) != 0)
  {
    Exosite_Init(IF_WIFI, 1);//Init meta data
    SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_UPDATED_EXO_CFG;//update flag indicating NVMEM write required
  }
  else
  {
    Exosite_Init(IF_WIFI, 0);

    /* Update MAC address */
    CC3200_MAC_ADDR_GET(g_MACAddress);
    Report("MAC Address : %02x:%02x:%02x:%02x:%02x:%02x\r\n", g_MACAddress[0], g_MACAddress[1],\
            g_MACAddress[2], g_MACAddress[3], g_MACAddress[4], g_MACAddress[5]);
  }

  /* Write smartplug NVMM file if any updated data */
  WriteSmartPlugNvmemFile();
  //NvmmfilePresent = ReadSmartPlugNvmemFile();//DKS test
}

void InitSmartPlug ( void )
{
  UINT32 Mask;
  SlDateTime_t    nwp_time;
  int retVal;
  UINT32 CriticalMask;

  /* Reset Simple link state mechine */
  ResetCC3200StateMachine();

  if(0 == g_SmartplugInitDone)
  {
    /* In Fresh Init clear bad counts */
    g_BadTCPCount = 0;
    g_BadNWPCount = 0;

    /* Init DMA for simple link & Metrology module */
    UDMAInit();
  }

  /* Init Simple link driver */
  InitSimpleLink();

  /* Clear WDT */
  ClearWDT();

  /* Create lock obj for Nvmem */
  osi_LockObjCreate(&g_NvmemLockObj);

  /* Check pre configured data in NVMEM and init exosite, metrology module */
  if(0 == g_SmartplugInitDone)
  {
    NvmemInit();
    g_SmartplugErrorRecovery.MetrologyModuleError = 0;
    /* Schedule table not valid once time lost due to reset */
    SmartPlugNvmmFile.DeviceConfigData.Schedule.Validity = 0;
    /* Clear WDT */
    ClearWDT();
  }
  /* Once NWP reset happens nwp time needs to be set again */
  SmartPlugNvmmFile.DeviceConfigData.SmartPlugDate.validity = 0;

  ExositeConStatus.g_ClientSoc = -1;
  osi_LockObjCreate(&ExositeConStatus.g_ClientLockObj);

  g_ConnectionState.ServerSoc = -1;
  g_ConnectionState.g_ClientSD = -1;
  osi_LockObjCreate(&g_ConnectionState.g_ClientLockObj);

  g_ClientRxbuff.ReadIndex = 0;
  g_ClientRxbuff.WriteIndex = 0;
  g_ClientRxbuff.TimeOutStart = 0;
  g_CABinarySize = 0xFFFF;

  SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_DATA_SEND_DEV_CFG|NVMEM_DATA_SEND_EXO_CFG|NVMEM_DATA_SEND_SCHEDULE_TABLE);
  SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToAndrid = 1;
  SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer = 1;
  SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer = 1;
  SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToAndrid = 1;

  g_SmartplugErrorRecovery.ExositeTxThreadError = 0;
  g_SmartplugErrorRecovery.ExositeRxThreadError = 0;
  g_SmartplugErrorRecovery.AndroidTxThreadError = 0;
  g_SmartplugErrorRecovery.AndroidRxThreadError = 0;
  g_SmartplugErrorRecovery.BadTCPErrorCheckCount = 0;
  g_SmartplugErrorRecovery.MdnsError = 0;
  g_ExoPriorityInverse = 0;

  mDNSService.mDNSBroadcastUpdateInterval = 60;//60s reset
  mDNSService.mDNSBroadcastTime = 0;
  mDNSService.mDNSServNameUnRegLen = 0;

  /* If Metrology cal is enabled then this is the time to disable it - 5sec consumed in AP config */
  if(1 == g_MetrologyCalEn)
  {
    /* If Metrology module does not respond then WDT reset happens in 10 secs */
    while(g_MetroCalCount < M_MAX_CAL_COUNT)
    {
      DELAY(1000);//wait 1sec
    }
    g_MetrologyCalEn = 0;

    AvgRmsNoise /= (float)(g_MetroCalCount-2);

    if(AvgRmsNoise <= M_RMS_NOISE_IN_I_CH)
    {
      CriticalMask = osi_EnterCritical();
      //memcpy((void *)&SmartPlugNvmmFile.MetrologyConfigData.RmsNoiseVCh[0], (const void *)&AvgRmsNoise, 4);  //DKS
      memcpy((void *)&SmartPlugNvmmFile.MetrologyConfigData.RmsNoiseICh[0], (const void *)&AvgRmsNoise, 4);
      osi_ExitCritical(CriticalMask);
      SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_UPDATED_MET_CFG;//update flag indicating NVMEM write required
      Report("Metrology Cal Done \n\r");
    }
    if(1 == SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.Device_status)
    {
      TurnOnDevice();
    }
  }

  osi_LockObjCreate(&g_UartLockObj);

  /* Update default NWP time */
  nwp_time.sl_tm_day  = SmartPlugNvmmFile.DeviceConfigData.SmartPlugDate.date;
  nwp_time.sl_tm_mon  = SmartPlugNvmmFile.DeviceConfigData.SmartPlugDate.month;
  nwp_time.sl_tm_year = SmartPlugNvmmFile.DeviceConfigData.SmartPlugDate.year;
  nwp_time.sl_tm_hour = SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.hour;
  nwp_time.sl_tm_min  = SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.min;
  nwp_time.sl_tm_sec  = 0;

  retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                        SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                        sizeof(nwp_time),(unsigned char *)(&nwp_time));

  if(retVal >= 0)
  {
    Report("NWP Default time updated\n\r" );
  }
  else
  {
    Report("NWP Default time update Error\n\r" );
  }

  /* Set Flag indicating Smartplug init done first time */
  g_SmartplugInitDone = 1;
  g_NwpInitdone = 1;

  Report("SmartPlug Initialized \n\r");

  /* Connect to default AP - if not able to connect AP control remains inside this function */
  ConnectToAP();

  //
  // Start the Exosite & Android task
  //
  Mask = osi_TaskDisable();

  osi_MsgQCreate(&AndroidClientTaskMsgQ,"AndroidClientTaskMsgQ",sizeof(t_SmartPlugSendData),1);

  osi_MsgQCreate(&ExositeTaskMsgQ,"ExositeTaskMsgQ",sizeof(t_SmartPlugSendData),1);

  osi_TaskCreate(AndroidClientRecvTask,
                (const signed char*)"Android recv",
                6144,
                NULL,
                ANDROID_RECV_TASK_PRIO,
                &AndroidClientRecvTaskHndl );

  osi_TaskCreate(ExositeSendTask,
                (const signed char*)"exosite send",
                6144,
                NULL,
                EXOSITE_SEND_TASK_PRIO,
                &ExositeSendTaskHndl );

  osi_TaskCreate(ExositeRecvTask,
                (const signed char*)"exosite recv",
                8192,
                NULL,
                EXOSITE_RECV_TASK_PRIO,
                &ExositeRecvTaskHndl );

  osi_TaskCreate(AndroidClientTask,
                (const signed char*)"Android send",
                6144,
                NULL,
                ANDROID_SEND_TASK_PRIO,
                &AndroidClientTaskHndl );

  /* Clear WDT */
  ClearWDT();

  osi_TaskEnable(Mask);
}

void DeInitSmartPlug ( void )
{
  UINT32 Mask;
  ClearWDT();

  /* Indicate flag that NWP is going to reset */
  g_NwpInitdone = 0;

  /* Clear Good TCP & internet */
  UnsetCC3200MachineState((CC3200_GOOD_TCP|CC3200_GOOD_INTERNET));

  /* Get full control */
  Mask = osi_TaskDisable();

  g_SmartplugLedCurrentState = DEVICE_ERROR_IND;

  /* Delete all tasks */
  osi_TaskDelete(&AndroidClientTaskHndl);
  osi_TaskDelete(&ExositeRecvTaskHndl);
  osi_TaskDelete(&ExositeSendTaskHndl);
  osi_TaskDelete(&AndroidClientRecvTaskHndl);

  /* Delete all msg Q */
  osi_MsgQDelete(&ExositeTaskMsgQ);
  osi_MsgQDelete(&AndroidClientTaskMsgQ);

  /* Delete all objects */
  osi_LockObjDelete(&g_ConnectionState.g_ClientLockObj);
  osi_LockObjDelete(&ExositeConStatus.g_ClientLockObj);
  osi_LockObjDelete(&g_NvmemLockObj);
  osi_LockObjDelete(&g_UartLockObj);

  Report("DeInit Simplelink... \n\r");

  /* switch off Simple link */
  DeInitSimpleLink();

  osi_TaskEnable(Mask);
}

//*****************************************************************************
// File: clienthandler.c
//
// Description: Android & Exosite client handler functions of Smartplug gen-1 application
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

#include <stdio.h>
#include "datatypes.h"
#include "simplelink.h"
#include <string.h>
#include "cc3200.h"
#include "utils.h"
#include "systick.h"
#include "smartconfig.h"
#include "uart_logger.h"
#include <hw_types.h>
#include <uart.h>
#include <hw_memmap.h>
#include "stdlib.h"
#include "exosite_meta.h"
#include "metrology.h"
#include "smartplug_task.h"
#include "android_task.h"
#include "exosite.h"
#include "exosite_hal.h"
#include "exosite_task.h"
#include "clienthandler.h"
#include "strlib.h"
#include "nvmem_api.h"

extern OsiMsgQ_t ExositeTaskMsgQ;
extern OsiMsgQ_t MainTaskMsgQ;

#define DBG_PRINT                       Report

extern t_SmartPlugNvmmFile SmartPlugNvmmFile;
extern t_AndroidClientRxBuff   g_ClientRxbuff;
extern OsiLockObj_t        g_NvmemLockObj;
extern s_ExoConnectionStatus  ExositeConStatus;
extern volatile unsigned long g_ulDeviceTimerInSec;
extern volatile unsigned long g_MsCounter;

extern volatile UINT8 g_NwpInitdone;
extern volatile UINT8 g_MetrologyCalEn;
extern volatile UINT8 g_MetroCalCount;
extern volatile UINT8 g_MetrologyGainScalCalEn;
extern float AvgRmsVoltage, AvgRmsCurrent;
extern s_ConnectionStatus  g_ConnectionState;

UINT16 g_CABinarySize = 0xFFFF;

//****************************************************************************
//
//! Function to parse the command/data from the Android and send Response
//!
//!
//****************************************************************************

//****************************************************************************
//
//! Function to Send Metrology data to every client at regular interval
//!
//!
//****************************************************************************
void SendMetrologyData(s_ConnectionStatus *soc, t_SmartPlugSendData *pSmartPlugSendData)
{
  char pBuff[44];

  pBuff[0] = (D2A_METORLOGY_DATA >> 8) & 0xFF;
  pBuff[1] = (D2A_METORLOGY_DATA) & 0xFF;

  memcpy((void*)&pBuff[2], (void*)&pSmartPlugSendData->SmartPlugMetrologyData.TruePower[0], 4);

  memcpy((void*)&pBuff[6], (void*)&pSmartPlugSendData->SmartPlugMetrologyData.RmsVoltage[0], 4);

  memcpy((void*)&pBuff[10], (void*)&pSmartPlugSendData->SmartPlugMetrologyData.RmsCurrent[0], 4);

  memcpy((void*)&pBuff[14], (void*)&pSmartPlugSendData->SmartPlugMetrologyData.ACFrequency[0], 4);

  memcpy((void*)&pBuff[18], (void*)&pSmartPlugSendData->SmartPlugMetrologyData.ReactivePower[0], 4);

  memcpy((void*)&pBuff[22], (void*)&pSmartPlugSendData->SmartPlugMetrologyData.PowerFactor[0], 4);

  memcpy((void*)&pBuff[26], (void*)&pSmartPlugSendData->SmartPlugMetrologyData.ApparentPower[0], 4);

  memcpy((void*)&pBuff[30], (void*)&pSmartPlugSendData->SmartPlugMetrologyData.TrueEnergyConsumed[0], 4);

  memcpy((void*)&pBuff[34], (void*)&pSmartPlugSendData->SmartPlugMetrologyData.AvgTruePower[0], 4);

  memcpy((void*)&pBuff[38], (void*)&pSmartPlugSendData->TimeStamp, 4);

  ClientSocketSend(soc->g_ClientSD, &pBuff[0], 42);
}

//****************************************************************************
//
//! Function to Send Average Energy to all connected client
//! This function is called every hour
//!
//****************************************************************************
void SendAvrgEnergy(s_ConnectionStatus *soc, t_SmartPlugSendData *pSmartPlugSendData)
{
  char pBuff[12];
  //UINT32 pValueBuff;

  pBuff[0] = (D2A_AVERAGE_ENERGY >> 8) & 0xFF;
  pBuff[1] = (D2A_AVERAGE_ENERGY) & 0xFF;

  memcpy((void*)&pBuff[2], (void*)&pSmartPlugSendData->SmartPlugMetrologyData.AveragePower[0], 4);//kWh

  memcpy((void*)&pBuff[6], (void*)&pSmartPlugSendData->TimeStamp, 4);

  ClientSocketSend(soc->g_ClientSD, &pBuff[0], 10);
}

void SendDeviceStatus(s_ConnectionStatus *soc, t_SmartPlugSendData *pSmartPlugSendData)
{
  char pBuff[20];
  int ret_val;
  float pw_thd_flt, en_thd_flt;
  UINT32 pw_thd, en_thd;

  pBuff[0] = (D2A_DEVICE_STATUS >> 8) & 0xFF;
  pBuff[1] = (D2A_DEVICE_STATUS) & 0xFF;

  osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
  pBuff[2] = SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.Device_status;
  pBuff[3] = SmartPlugNvmmFile.DeviceConfigData.updateinterval.IsPowerSaving;
  pBuff[4] = SmartPlugNvmmFile.DeviceConfigData.updateinterval.Interval;
  memcpy((void*)&en_thd_flt,(void*)&SmartPlugNvmmFile.DeviceConfigData.ThreasholdEnergy[0],4);
  memcpy((void*)&pw_thd_flt,(void*)&SmartPlugNvmmFile.DeviceConfigData.ThreasholdPower[0],4);
  osi_LockObjUnlock(&g_NvmemLockObj);

  pw_thd = (UINT32)(pw_thd_flt * M_SCALE_FACTOR_3_DIGIT);
  en_thd = (UINT32)(en_thd_flt * M_SCALE_FACTOR_4_DIGIT);

  memcpy((void*)&pBuff[5], (void*)&en_thd, 4);//kWh
  memcpy((void*)&pBuff[9], (void*)&pw_thd, 4);//Watt

  memcpy((void*)&pBuff[13], (void*)&pSmartPlugSendData->TimeStamp, 4);

  ret_val = ClientSocketSend(soc->g_ClientSD, pBuff, 17);
  if(ret_val > 0)
  {
    osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
    if(pBuff[2] == SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.Device_status)
    {
      SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToAndrid = 0;
    }
    osi_LockObjUnlock(&g_NvmemLockObj);
  }
}

void sendWarning(s_ConnectionStatus *soc, t_SmartPlugSendData *pSmartPlugSendData)
{
  char pBuff[12];
  unsigned char flag = 0;

  pBuff[0] = (D2A_WARNING_MESSAGES >> 8) & 0xFF;
  pBuff[1] = (D2A_WARNING_MESSAGES) & 0xFF;

  flag = (pSmartPlugSendData->DataSender & SEND_THRESHOLD_EN_WARNING) ? 1 : 0;
  pBuff[2] = flag;

  flag = (pSmartPlugSendData->DataSender & SEND_THRESHOLD_PW_WARNING) ? 1 : 0;
  pBuff[3] = flag;

  flag = (pSmartPlugSendData->DataSender & SEND_METROLOGY_WARNING) ? 1 : 0;
  pBuff[4] = flag;

  memcpy((void*)&pBuff[5], (void*)&pSmartPlugSendData->TimeStamp, 4);

  ClientSocketSend(soc->g_ClientSD, pBuff, 9);
}

void send24HrAvgEnergy(s_ConnectionStatus *soc, t_SmartPlugSendData *pSmartPlugSendData)
{
  signed char count = 0;
  unsigned char index = 0;
  UINT32 avgEnDefault = 0;
  char pBuff[104];

  count = pSmartPlugSendData->SmartPlugMetrologyData.AP24HrCount;

  pBuff[0] = (D2A_AVERAGE_ENERGY_24HRS >> 8) & 0xFF;
  pBuff[1] = (D2A_AVERAGE_ENERGY_24HRS) & 0xFF;

  do
  {
    count--;
    if(count < 0)
    {
      if(pSmartPlugSendData->SmartPlugMetrologyData.AP24HrRollover)
      {
        count += 24;
      }
      else
      {
        break;
      }
    }

    memcpy((void*)&pBuff[2+(index*4)], (void*)&pSmartPlugSendData->SmartPlugMetrologyData.AveragePower24Hr[count][0], 4);//kWh
    index++;

  }while (index < 24);

  for(; index < 24; index++)
  {
    memcpy((void*)&pBuff[2+(index*4)], (void*)&avgEnDefault, 4);//0 kWh
  }

  memcpy((void*)&pBuff[2+(index*4)], (void*)&pSmartPlugSendData->TimeStamp, 4);//98

  ClientSocketSend(soc->g_ClientSD, pBuff, 102);
}

void sendScheduleTable(s_ConnectionStatus *soc, t_SmartPlugSendData *pSmartPlugSendData)
{
  char pBuff[32];
  int ret_val;

  pBuff[0] = (D2A_SCHEDULE_TABLE >> 8) & 0xFF;
  pBuff[1] = (D2A_SCHEDULE_TABLE) & 0xFF;

  osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
  memcpy((void*)&pBuff[2], (void*)&SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule, sizeof(t_weekschedule));//24
  memcpy((void*)&pBuff[26], (void*)&SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime, sizeof(t_smartplugtime));//3
  memcpy((void*)&pBuff[29], (void*)&SmartPlugNvmmFile.DeviceConfigData.Schedule.Validity, 1);//1
  osi_LockObjUnlock(&g_NvmemLockObj);

  ret_val = ClientSocketSend(soc->g_ClientSD, pBuff, 30);
  if(ret_val > 0)
  {
    SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToAndrid = 0;
  }
}

void sendCloudInfo(s_ConnectionStatus *soc, t_SmartPlugSendData *pSmartPlugSendData)
{
  char pBuff[44];

  pBuff[0] = (D2A_EXOSITE_REG_INFO >> 8) & 0xFF;
  pBuff[1] = (D2A_EXOSITE_REG_INFO) & 0xFF;

  memcpy((void*)&pBuff[2], (void*)&pSmartPlugSendData->ExositeMetaData, sizeof(t_exosite_meta_send));

  memcpy((void*)&pBuff[38], (void*)&pSmartPlugSendData->TimeStamp, 4);//38

  ClientSocketSend(soc->g_ClientSD, pBuff, 42);
}

void sendMetrologyCalInfo(s_ConnectionStatus *soc, t_SmartPlugSendData *pSmartPlugSendData)
{
  char pBuff[24];
  float RmsNoiseV_flt, ChannelGainV_flt, RmsNoiseI_flt, ChannelGainI_flt;
  UINT32 RmsNoiseV, ChannelGainV, RmsNoiseI, ChannelGainI;

  pBuff[0] = (D2A_METROLOGY_CAL_INFO >> 8) & 0xFF;
  pBuff[1] = (D2A_METROLOGY_CAL_INFO) & 0xFF;

  memcpy((void *)&RmsNoiseV_flt,(const void *)&pSmartPlugSendData->MetrologyConfigData.RmsNoiseVCh[0],4);
  memcpy((void *)&ChannelGainV_flt,(const void *)&pSmartPlugSendData->MetrologyConfigData.VChScaleFactor[0],4);
  memcpy((void *)&RmsNoiseI_flt,(const void *)&pSmartPlugSendData->MetrologyConfigData.RmsNoiseICh[0],4);
  memcpy((void *)&ChannelGainI_flt,(const void *)&pSmartPlugSendData->MetrologyConfigData.IChScaleFactor[0],4);

  RmsNoiseV    = (UINT32)(RmsNoiseV_flt * M_SCALE_FACTOR_5_DIGIT);
  ChannelGainV = (UINT32)(ChannelGainV_flt * M_SCALE_FACTOR_5_DIGIT);
  RmsNoiseI    = (UINT32)(RmsNoiseI_flt * M_SCALE_FACTOR_5_DIGIT);
  ChannelGainI = (UINT32)(ChannelGainI_flt * M_SCALE_FACTOR_5_DIGIT);

  memcpy((void*)&pBuff[2], (void*)&RmsNoiseV, 4);
  memcpy((void*)&pBuff[6], (void*)&RmsNoiseI, 4);
  memcpy((void*)&pBuff[10], (void*)&ChannelGainV, 4);
  memcpy((void*)&pBuff[14], (void*)&ChannelGainI, 4);

  memcpy((void*)&pBuff[18], (void*)&pSmartPlugSendData->TimeStamp, 4);

  ClientSocketSend(soc->g_ClientSD, pBuff, 22);
}

void sendCloudCAUpdateSuccess(s_ConnectionStatus *soc, t_SmartPlugSendData *pSmartPlugSendData)
{
  char pBuff[8];

  pBuff[0] = (D2A_EXO_SSL_CA_UPDATE_DONE >> 8) & 0xFF;
  pBuff[1] = (D2A_EXO_SSL_CA_UPDATE_DONE) & 0xFF;

  memcpy((void*)&pBuff[2], (void*)&pSmartPlugSendData->TimeStamp, 4);

  ClientSocketSend(soc->g_ClientSD, pBuff, 6);
}

/************************ RECEIVE **************************************************************/
void ProcessData(unsigned char * rxdata)
{
  int length, index = 0, test_i;
  UINT8 exit = 0;
  UINT16 message_type = 0;

  length = (int)g_ClientRxbuff.WriteIndex - (int)g_ClientRxbuff.ReadIndex;
  if(length < 0)
  {
    length += MAX_ANDROID_RX_BUFF;
  }

  do
  {
    DBG_PRINT("Message Length %d\n\r", length);

    /* length should be at least 2 bytes */
    if(length < 2)
    {
      break;
    }
    index = g_ClientRxbuff.ReadIndex;

    message_type = (((UINT16)rxdata[index] << 8) & 0xFF00)| rxdata[index+1];
    test_i = 0;

    DBG_PRINT("Message Type %x\n\r", message_type);

    DBG_PRINT("Message Body : ");
    if((message_type != 0) && (message_type != 0xFFFF))
    {
      while(test_i < (length-2))
      {
        DBG_PRINT("%x,", rxdata[index+2+test_i]);
        test_i++;
      }
    }
    DBG_PRINT("\n\r");

    switch(message_type)
    {
      case A2D_SET_DEV_ON_OFF:

        if(length >= A2D_SET_DEV_ON_OFF_SIZE)
        {
          if(rxdata[index+2] <= 1)//value should be 0 or 1
          {
            /* Updte based on input */
            UpdateDevStatus(rxdata[index+2]);

            DBG_PRINT("Read dev cfg client %d\n\r",rxdata[index+2]);
          }
          else
          {
            DBG_PRINT("error in dev cfg val \n\r");
          }

          g_ClientRxbuff.ReadIndex += A2D_SET_DEV_ON_OFF_SIZE;
          length -= A2D_SET_DEV_ON_OFF_SIZE;
        }
        else
        {
          exit = 1;
        }
        break;

      case A2D_SET_THRESHOLD:

        if(length >= A2D_SET_THRESHOLD_SIZE)
        {
          UINT32 en_thd, pow_thd;
          float en_thd_flt, pow_thd_flt;

          //
          //Save value in Flash and Update the global value
          //
          en_thd = *(UINT32*)&rxdata[index+2];
          pow_thd = *(UINT32*)&rxdata[index+6];
          en_thd_flt = (float)en_thd/(float)M_SCALE_FACTOR_4_DIGIT;
          pow_thd_flt = (float)pow_thd/(float)M_SCALE_FACTOR_3_DIGIT;

          osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);

          if((en_thd_flt != (*(UINT32*)&SmartPlugNvmmFile.DeviceConfigData.ThreasholdEnergy[0])) && (en_thd_flt <= MAX_ENERGY_KWH))
          {
            memcpy((void *)&SmartPlugNvmmFile.DeviceConfigData.ThreasholdEnergy[0],(const void *)&en_thd_flt, 4);
            SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_DEV_CFG|NVMEM_DATA_SEND_DEV_CFG);//update flag indicating NVMEM write required
          }
          if((pow_thd_flt != (*(UINT32*)&SmartPlugNvmmFile.DeviceConfigData.ThreasholdPower[0])) && (pow_thd_flt <= MAX_POWER_WATTS))
          {
            memcpy((void *)&SmartPlugNvmmFile.DeviceConfigData.ThreasholdPower[0],(const void *)&pow_thd_flt, 4);
            SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_DEV_CFG|NVMEM_DATA_SEND_DEV_CFG);//update flag indicating NVMEM write required
          }

          osi_LockObjUnlock(&g_NvmemLockObj);

          DBG_PRINT("Read power thresh client %f, %f\n\r",en_thd_flt,pow_thd_flt);

          g_ClientRxbuff.ReadIndex += A2D_SET_THRESHOLD_SIZE;
          length -= A2D_SET_THRESHOLD_SIZE;
        }
        else
        {
          exit = 1;
        }
      break;

      case A2D_SET_PS_MODE:

        if(length >= A2D_SET_PS_MODE_SIZE)
        {
          if(rxdata[index+2] <= 1)
          {
            osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
            //
            //Save Value in Flash and Update the global variable
            //
            if(rxdata[index+2] != SmartPlugNvmmFile.DeviceConfigData.updateinterval.IsPowerSaving)
            {
               SmartPlugNvmmFile.DeviceConfigData.updateinterval.IsPowerSaving = rxdata[index+2];
               SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_DEV_CFG|NVMEM_DATA_SEND_DEV_CFG);//update flag indicating NVMEM write required

               if(rxdata[index+2] == 1)
               {
                  if(SmartPlugNvmmFile.DeviceConfigData.updateinterval.Interval < MIN_UPDATERATE_IN_PS_MODE)
                  {
                    SmartPlugNvmmFile.DeviceConfigData.updateinterval.Interval = MIN_UPDATERATE_IN_PS_MODE;
                  }
               }
               else
               {
                 SmartPlugNvmmFile.DeviceConfigData.updateinterval.Interval = DEFAULT_UPDATE_INTERVAL;
               }
            }

            if((rxdata[index+3] != SmartPlugNvmmFile.DeviceConfigData.updateinterval.Interval) && (rxdata[index+3] <= MAX_UPDATE_INTERVAL))
            {
               if(((1 == SmartPlugNvmmFile.DeviceConfigData.updateinterval.IsPowerSaving) &&\
                 (rxdata[index+3] > MIN_UPDATERATE_IN_PS_MODE)) ||\
                   (0 == SmartPlugNvmmFile.DeviceConfigData.updateinterval.IsPowerSaving))
               {
                  SmartPlugNvmmFile.DeviceConfigData.updateinterval.Interval = rxdata[index+3];
                  SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_DEV_CFG|NVMEM_DATA_SEND_DEV_CFG);//update flag indicating NVMEM write required
               }
            }

            osi_LockObjUnlock(&g_NvmemLockObj);

            DBG_PRINT("Read power mode interval %d,%d\n\r",rxdata[index+2], rxdata[index+3]);
          }
          else
          {
            DBG_PRINT("could not read power mode interval value\n\r");
          }

          g_ClientRxbuff.ReadIndex += A2D_SET_PS_MODE_SIZE;
          length -= A2D_SET_PS_MODE_SIZE;
        }
        else
        {
          exit = 1;
        }
      break;

      case A2D_REQ_24HRS_AVG_ENERGY:

        if(length >= A2D_REQ_24HRS_AVG_ENERGY_SIZE)
        {
          osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
          SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_DATA_SEND_24HR_ENERGY;
          osi_LockObjUnlock(&g_NvmemLockObj);

          g_ClientRxbuff.ReadIndex += A2D_REQ_24HRS_AVG_ENERGY_SIZE;
          length -= A2D_REQ_24HRS_AVG_ENERGY_SIZE;
        }
        else
        {
          exit = 1;
        }
      break;

      case A2D_REQ_SCHEDULE_TABLE:

        if(length >= A2D_REQ_SCHEDULE_TABLE_SIZE)
        {
          osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
          SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_DATA_SEND_SCHEDULE_TABLE;
          osi_LockObjUnlock(&g_NvmemLockObj);

          g_ClientRxbuff.ReadIndex += A2D_REQ_SCHEDULE_TABLE_SIZE;
          length -= A2D_REQ_SCHEDULE_TABLE_SIZE;
        }
        else
        {
          exit = 1;
        }
      break;

      case A2D_REQ_METROLOGY_CAL_INFO:

        if(length >= A2D_REQ_METROLOGY_CAL_INFO_SIZE)
        {
          osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
          SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_DATA_SEND_MET_CFG;
          osi_LockObjUnlock(&g_NvmemLockObj);

          g_ClientRxbuff.ReadIndex += A2D_REQ_METROLOGY_CAL_INFO_SIZE;
          length -= A2D_REQ_METROLOGY_CAL_INFO_SIZE;
        }
        else
        {
          exit = 1;
        }
      break;

      case A2D_REQ_EXOSITE_REG_INFO:

        if(length >= A2D_REQ_EXOSITE_REG_INFO_SIZE)
        {
          osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
          SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_DATA_SEND_EXO_CFG;
          osi_LockObjUnlock(&g_NvmemLockObj);

          g_ClientRxbuff.ReadIndex += A2D_REQ_EXOSITE_REG_INFO_SIZE;
          length -= A2D_REQ_EXOSITE_REG_INFO_SIZE;
        }
        else
        {
          exit = 1;
        }
      break;

      case A2D_REQ_DEV_INFO:

        if(length >= A2D_REQ_DEV_INFO_SIZE)
        {
          osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
          SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_DATA_SEND_DEV_CFG;
          osi_LockObjUnlock(&g_NvmemLockObj);

          g_ClientRxbuff.ReadIndex += A2D_REQ_DEV_INFO_SIZE;
          length -= A2D_REQ_DEV_INFO_SIZE;
        }
        else
        {
          exit = 1;
        }
      break;

      case A2D_SET_SCHEDULE_TABLE:

        if(length >= A2D_SET_SCHEDULE_TABLE_SIZE)
        {
          if((SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer == 0)&&\
                  (SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToAndrid == 0))
          {
            osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
            memcpy((void*)&SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule, \
                (void*)&rxdata[index+2], sizeof(t_weekschedule));//24
            memcpy((void*)&SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime, (void*)&rxdata[index+26], sizeof(t_smartplugtime));
            memcpy((void*)&SmartPlugNvmmFile.DeviceConfigData.Schedule.Validity, (void*)&rxdata[index+29], 1);

            SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer = 1;
            SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToAndrid = 1;
            SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_DEV_CFG|NVMEM_DATA_SEND_SCHEDULE_TABLE);
            osi_LockObjUnlock(&g_NvmemLockObj);
          }

          g_ClientRxbuff.ReadIndex += A2D_SET_SCHEDULE_TABLE_SIZE;
          length -= A2D_SET_SCHEDULE_TABLE_SIZE;
        }
        else
        {
          exit = 1;
        }
      break;

      case A2D_SET_METROLOGY_CAL_INFO:

        if(length >= A2D_SET_METROLOGY_CAL_INFO_SIZE)
        {
          UINT32 CriticalMask, DesiredVoltage, DesiredCurrent;
          float DesiredVoltage_flt, DesiredCurrent_flt;
          float AvgVoltage_flt, AvgCurrent_flt;
          float ChannelGainV_flt = 1.0, ChannelGainI_flt = 1.0;

          memcpy((void*)&DesiredVoltage, (void*)&rxdata[index+2], 4);
          memcpy((void*)&DesiredCurrent, (void*)&rxdata[index+6], 4);

          DesiredVoltage_flt = (float)DesiredVoltage / (float)M_SCALE_FACTOR_5_DIGIT;
          DesiredCurrent_flt = (float)DesiredCurrent / (float)M_SCALE_FACTOR_5_DIGIT;

          /* wait till exosite & android finnish its activity */
          g_NwpInitdone = 0;//Disable NWP activity
          Report("wait till exosite and andriod task free \n\r");
          osi_LockObjLock(&g_ConnectionState.g_ClientLockObj, SL_OS_WAIT_FOREVER);
          osi_LockObjLock(&ExositeConStatus.g_ClientLockObj, SL_OS_WAIT_FOREVER);
          AvgRmsVoltage = 0;
          AvgRmsCurrent = 0;
          g_MetroCalCount = 0;

          CriticalMask = osi_EnterCritical();
          memcpy((void *)&SmartPlugNvmmFile.MetrologyConfigData.VChScaleFactor[0], (void *)&ChannelGainV_flt, 4);
          memcpy((void *)&SmartPlugNvmmFile.MetrologyConfigData.IChScaleFactor[0], (void *)&ChannelGainI_flt, 4);
          g_MetrologyGainScalCalEn = 1;
          osi_ExitCritical(CriticalMask);

          /* If Metrology module does not respond then WDT reset happens in 10 secs */
          while(g_MetroCalCount < M_MAX_GAIN_CAL_COUNT)
          {
            DELAY(1000);//wait 1sec
          }

          CriticalMask = osi_EnterCritical();
          g_MetrologyGainScalCalEn = 0;
          osi_ExitCritical(CriticalMask);
          AvgVoltage_flt = AvgRmsVoltage/g_MetroCalCount;
          AvgCurrent_flt = AvgRmsCurrent/g_MetroCalCount;
          if(AvgVoltage_flt > 0.0)
          {
            ChannelGainV_flt = DesiredVoltage_flt/AvgVoltage_flt;
          }
          if(AvgCurrent_flt > 0.0)
          {
            ChannelGainI_flt = DesiredCurrent_flt/AvgCurrent_flt;
          }

          //This being used in interrupt routine
          CriticalMask = osi_EnterCritical();
          if(ChannelGainV_flt >= M_V_CH_SCALE_FATOR_MIN)
          {
            memcpy((void *)&SmartPlugNvmmFile.MetrologyConfigData.VChScaleFactor[0], (void *)&ChannelGainV_flt, 4);
          }
          if(ChannelGainI_flt >= M_I_CH_SCALE_FATOR_MIN)
          {
            memcpy((void *)&SmartPlugNvmmFile.MetrologyConfigData.IChScaleFactor[0], (void *)&ChannelGainI_flt, 4);
          }
          osi_ExitCritical(CriticalMask);
          SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_MET_CFG|NVMEM_DATA_SEND_MET_CFG);

          g_NwpInitdone = 1;//Enable NWP activity
          osi_LockObjUnlock(&g_ConnectionState.g_ClientLockObj);
          osi_LockObjUnlock(&ExositeConStatus.g_ClientLockObj);

          Report("Metrology Gain scale factor calibration successful \n\r");

          g_ClientRxbuff.ReadIndex += A2D_SET_METROLOGY_CAL_INFO_SIZE;
          length -= A2D_SET_METROLOGY_CAL_INFO_SIZE;
        }
        else
        {
          exit = 1;
        }
      break;

      case A2D_SET_SECURED_EXO_CONN:

        if(length >= A2D_SET_SECURED_EXO_CONN_SIZE)
        {
          long portNum;

          if(rxdata[index+2] <= 1)
          {
            if(1 == rxdata[index+2])
            {
              portNum = EXO_PORT_SECURE;
              exosite_meta_write((unsigned char *)&portNum, META_PORT_NUM_SIZE, META_PORT_NUM);    //store port num
            }
            else
            {
              portNum = EXO_PORT_NONSEC;
              exosite_meta_write((unsigned char *)&portNum, META_PORT_NUM_SIZE, META_PORT_NUM);    //store port num
            }

            /* wait till exosite finnish its activity */
            osi_LockObjLock(&ExositeConStatus.g_ClientLockObj, SL_OS_WAIT_FOREVER);

            /* Close exosite socket to enter new port */
            if(ExositeConStatus.g_ClientSoc >= 0)
            {
              exoHAL_SocketClose(ExositeConStatus.g_ClientSoc);
            }

            osi_LockObjUnlock(&ExositeConStatus.g_ClientLockObj);
          }
          else
          {
            Report("Android read error exo ssl enable \n\r");
          }

          g_ClientRxbuff.ReadIndex += A2D_SET_SECURED_EXO_CONN_SIZE;
          length -= A2D_SET_SECURED_EXO_CONN_SIZE;
        }
        else
        {
          exit = 1;
        }
      break;

      case A2D_SET_EXO_SSL_CA_CERT:

        if((length >= A2D_SET_EXO_SSL_CA_CERT_SIZE) && (0xFFFF == g_CABinarySize))
        {
          g_CABinarySize = *((UINT16*)&rxdata[index+2]);
          exit = 1;
        }
        else if(length >= (g_CABinarySize+1+4))//including checksum
        {
          if(rxdata[index+4+g_CABinarySize] == ComputeCheckSum(&rxdata[index+4], g_CABinarySize))
          {
            /* wait till exosite finnish its activity */
            osi_LockObjLock(&ExositeConStatus.g_ClientLockObj, SL_OS_WAIT_FOREVER);

            /* Close exosite socket before updating CA */
            if(ExositeConStatus.g_ClientSoc >= 0)
            {
              exoHAL_SocketClose(ExositeConStatus.g_ClientSoc);
            }

            if(UpdateExositeCA(&rxdata[index+4], g_CABinarySize) >= 0)
            {
              osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
              SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_DATA_SEND_EXO_SSL_CA_DONE;
              osi_LockObjUnlock(&g_NvmemLockObj);
            }

            osi_LockObjUnlock(&ExositeConStatus.g_ClientLockObj);
          }

          g_ClientRxbuff.ReadIndex += (g_CABinarySize+1+4);
          length -= (g_CABinarySize+1+4);
          g_CABinarySize = 0xFFFF;
        }
        else
        {
          exit = 1;
        }
      break;

      default:
        g_ClientRxbuff.WriteIndex = 0;
        g_ClientRxbuff.ReadIndex = 0;
        exit = 1;
        g_CABinarySize = 0xFFFF;
        Report("Invalid data from Android \n\r");
      break;
    }

    if(g_ClientRxbuff.ReadIndex >= MAX_ANDROID_RX_BUFF)
    {
      g_ClientRxbuff.ReadIndex -= MAX_ANDROID_RX_BUFF;
    }

    if(exit)
    {
      g_ClientRxbuff.TimeOutStart = g_MsCounter;
      break;
    }
  } while (length > 0);

  if(length <= 0)
  {
    g_ClientRxbuff.WriteIndex = 0;
    g_ClientRxbuff.ReadIndex = 0;
    g_CABinarySize = 0xFFFF;
  }
}

/*************************************************************************************************
 * EXOSITE SEND & RECEIVE
 *************************************************************************************************/
void SendAvrgEnergyToServer(t_SmartPlugSendData *pSmartPlugSendData)
{
  char pBuff[10];
  int iRetVal;
  UINT32 pValueBuff;
  char pExoBuff[50];
  float ValueOut;

  pBuff[1] = pSmartPlugSendData->SmartPlugMetrologyData.AveragePower[0];
  pBuff[2] = pSmartPlugSendData->SmartPlugMetrologyData.AveragePower[1];
  pBuff[3] = pSmartPlugSendData->SmartPlugMetrologyData.AveragePower[2];
  pBuff[4] = pSmartPlugSendData->SmartPlugMetrologyData.AveragePower[3];

  memcpy((void *)&pValueBuff,(const void *)&pBuff[1],4);
  ValueOut = (float)pValueBuff/(float)M_SCALE_FACTOR_4_DIGIT;
  iRetVal = sprintf((char*)pExoBuff,"enr_avg=%.4f",ValueOut);

  Exosite_Write(pExoBuff, iRetVal);

  Report("send_exo avg power data\n\r");
}

void SendDeviceStatusToServer(void)
{
  char pExoBuff[256];
  int iRetVal, succs;
  int dev_state, ps_mode, interval;
  float pw_thd, en_thd;

  osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
  dev_state =  SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.Device_status;
  ps_mode   =  SmartPlugNvmmFile.DeviceConfigData.updateinterval.IsPowerSavingServer;
  interval  =  SmartPlugNvmmFile.DeviceConfigData.updateinterval.IntervalServer;
  memcpy((void*)&pw_thd,(void*)&SmartPlugNvmmFile.DeviceConfigData.ThresholdPowerServer[0],4);
  memcpy((void*)&en_thd,(void*)&SmartPlugNvmmFile.DeviceConfigData.ThresholdEnergyServer[0],4);
  osi_LockObjUnlock(&g_NvmemLockObj);

  iRetVal =  sprintf(pExoBuff,"control=%d&",dev_state);
  iRetVal += sprintf(pExoBuff+iRetVal,"powmode=%d&",ps_mode);
  iRetVal += sprintf(pExoBuff+iRetVal,"reportint=%d&",interval);
  iRetVal += sprintf(pExoBuff+iRetVal,"enthld=%.4f&",en_thd);
  iRetVal += sprintf(pExoBuff+iRetVal,"powthld=%.4f",pw_thd);

  succs = Exosite_Write(pExoBuff, iRetVal);

  if(succs == 1)
  {
    osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
    if(dev_state == SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.Device_status)
    {
      SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer = 0;
    }
    osi_LockObjUnlock(&g_NvmemLockObj);
  }
  Report("send_exo device staus data\n\r");

}

void SendCommandClearToServer(void)
{
  char pExoBuff[20];
  int iRetVal, succs;

  iRetVal = sprintf(pExoBuff,"command=%d",(int)0);

  succs = Exosite_Write(pExoBuff, iRetVal);

  if(succs == 1)
  {
    Report("send_exo command data\n\r");
  }
}

//****************************************************************************
//
//! Function to Send Metrology data to server at regular intervals
//!
//!
//****************************************************************************
void SendMetrologyDataToServer(t_SmartPlugSendData *pSmartPlugSendData)
{
  char pBuff[40];
  char pExoBuff[256];
  UINT32 pValueBuff;
  float ValueOut;
  int iRetVal;

  pBuff[1] = pSmartPlugSendData->SmartPlugMetrologyData.TruePower[0];
  pBuff[2] = pSmartPlugSendData->SmartPlugMetrologyData.TruePower[1];
  pBuff[3] = pSmartPlugSendData->SmartPlugMetrologyData.TruePower[2];
  pBuff[4] = pSmartPlugSendData->SmartPlugMetrologyData.TruePower[3];

  memcpy((void *)&pValueBuff,(const void *)&pBuff[1],4);
  ValueOut = (float)pValueBuff/(float)M_SCALE_FACTOR_3_DIGIT;
  iRetVal = sprintf(pExoBuff,"power=%.4f&",ValueOut);

  pBuff[5] = pSmartPlugSendData->SmartPlugMetrologyData.RmsVoltage[0];
  pBuff[6] = pSmartPlugSendData->SmartPlugMetrologyData.RmsVoltage[1];
  pBuff[7] = pSmartPlugSendData->SmartPlugMetrologyData.RmsVoltage[2];
  pBuff[8] = pSmartPlugSendData->SmartPlugMetrologyData.RmsVoltage[3];

  memcpy((void *)&pValueBuff,(const void *)&pBuff[5],4);
  ValueOut = (float)pValueBuff/(float)M_SCALE_FACTOR_3_DIGIT;
  iRetVal += sprintf(pExoBuff+iRetVal,"volt=%.4f&",ValueOut);

  pBuff[9] = pSmartPlugSendData->SmartPlugMetrologyData.RmsCurrent[0];
  pBuff[10] = pSmartPlugSendData->SmartPlugMetrologyData.RmsCurrent[1];
  pBuff[11] = pSmartPlugSendData->SmartPlugMetrologyData.RmsCurrent[2];
  pBuff[12] = pSmartPlugSendData->SmartPlugMetrologyData.RmsCurrent[3];

  memcpy((void *)&pValueBuff,(const void *)&pBuff[9],4);
  ValueOut = (float)pValueBuff/(float)M_SCALE_FACTOR_3_DIGIT;
  iRetVal += sprintf(pExoBuff+iRetVal,"current=%.4f&",ValueOut);

  pBuff[13] = pSmartPlugSendData->SmartPlugMetrologyData.ACFrequency[0];
  pBuff[14] = pSmartPlugSendData->SmartPlugMetrologyData.ACFrequency[1];
  pBuff[15] = pSmartPlugSendData->SmartPlugMetrologyData.ACFrequency[2];
  pBuff[16] = pSmartPlugSendData->SmartPlugMetrologyData.ACFrequency[3];

  memcpy((void *)&pValueBuff,(const void *)&pBuff[13],4);
  ValueOut = (float)pValueBuff/(float)M_SCALE_FACTOR_3_DIGIT;
  iRetVal += sprintf(pExoBuff+iRetVal,"freq=%.4f&",ValueOut);

  pBuff[17] = pSmartPlugSendData->SmartPlugMetrologyData.ReactivePower[0];
  pBuff[18] = pSmartPlugSendData->SmartPlugMetrologyData.ReactivePower[1];
  pBuff[19] = pSmartPlugSendData->SmartPlugMetrologyData.ReactivePower[2];
  pBuff[20] = pSmartPlugSendData->SmartPlugMetrologyData.ReactivePower[3];

  memcpy((void *)&pValueBuff,(const void *)&pBuff[17],4);
  ValueOut = (float)pValueBuff/(float)M_SCALE_FACTOR_3_DIGIT;
  iRetVal += sprintf(pExoBuff+iRetVal,"var=%.4f&",ValueOut);

  pBuff[21] = pSmartPlugSendData->SmartPlugMetrologyData.PowerFactor[0];
  pBuff[22] = pSmartPlugSendData->SmartPlugMetrologyData.PowerFactor[1];
  pBuff[23] = pSmartPlugSendData->SmartPlugMetrologyData.PowerFactor[2];
  pBuff[24] = pSmartPlugSendData->SmartPlugMetrologyData.PowerFactor[3];

  memcpy((void *)&pValueBuff,(const void *)&pBuff[21],4);
  ValueOut = (float)pValueBuff/(float)M_SCALE_FACTOR_4_DIGIT;
  iRetVal += sprintf(pExoBuff+iRetVal,"pf=%.4f&",ValueOut);

  pBuff[25] = pSmartPlugSendData->SmartPlugMetrologyData.ApparentPower[0];
  pBuff[26] = pSmartPlugSendData->SmartPlugMetrologyData.ApparentPower[1];
  pBuff[27] = pSmartPlugSendData->SmartPlugMetrologyData.ApparentPower[2];
  pBuff[28] = pSmartPlugSendData->SmartPlugMetrologyData.ApparentPower[3];

  memcpy((void *)&pValueBuff,(const void *)&pBuff[25],4);
  ValueOut = (float)pValueBuff/(float)M_SCALE_FACTOR_3_DIGIT;
  iRetVal += sprintf(pExoBuff+iRetVal,"va=%.4f&",ValueOut);
  //Exosite_Write(pExoBuff, strlen(pExoBuff));

  pBuff[29] = pSmartPlugSendData->SmartPlugMetrologyData.TrueEnergyConsumed[0];
  pBuff[30] = pSmartPlugSendData->SmartPlugMetrologyData.TrueEnergyConsumed[1];
  pBuff[31] = pSmartPlugSendData->SmartPlugMetrologyData.TrueEnergyConsumed[2];
  pBuff[32] = pSmartPlugSendData->SmartPlugMetrologyData.TrueEnergyConsumed[3];

  memcpy((void *)&pValueBuff,(const void *)&pBuff[29],4);
  ValueOut = (float)pValueBuff/(float)M_SCALE_FACTOR_4_DIGIT;
  iRetVal += sprintf(pExoBuff+iRetVal,"kwh=%.4f&",ValueOut);

  pBuff[33] = pSmartPlugSendData->SmartPlugMetrologyData.AvgTruePower[0];
  pBuff[34] = pSmartPlugSendData->SmartPlugMetrologyData.AvgTruePower[1];
  pBuff[35] = pSmartPlugSendData->SmartPlugMetrologyData.AvgTruePower[2];
  pBuff[36] = pSmartPlugSendData->SmartPlugMetrologyData.AvgTruePower[3];

  memcpy((void *)&pValueBuff,(const void *)&pBuff[33],4);
  ValueOut = (float)pValueBuff/(float)M_SCALE_FACTOR_3_DIGIT;
  iRetVal += sprintf(pExoBuff+iRetVal,"pow_avg=%.4f",ValueOut);

  Exosite_Write(pExoBuff, iRetVal);

  Report("send_exo metro data\n\r");

}

void sendPowerWarningToServer()
{
  char *pExoBuff = "pow_mesg=power_threshold_exceeded";

   Exosite_Write(pExoBuff, strlen(pExoBuff));

   Report("send_exo power warning\n\r");

}

void sendEnergyWarningToServer()
{
  char *pExoBuff = "enr_mesg=energy_threshold_exceeded";

   Exosite_Write(pExoBuff, strlen(pExoBuff));

   Report("send_exo energy warning\n\r");

}

void SendScheduleTableToServer()
{
  char pExoBuff[256];
  t_weekschedule WeekSchdTable;
  t_smartplugtime SmartplugTime;
  int iRetVal, succs, Validity;

  osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
  WeekSchdTable = SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule;
  SmartplugTime = SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime;
  Validity = SmartPlugNvmmFile.DeviceConfigData.Schedule.Validity;
  osi_LockObjUnlock(&g_NvmemLockObj);

  iRetVal =  sprintf(pExoBuff,"schedule=%02d%02d,",WeekSchdTable.mon_fri.wk_hour,WeekSchdTable.mon_fri.wk_min);
  iRetVal += sprintf(pExoBuff+iRetVal,"%02d%02d,",WeekSchdTable.mon_fri.lv_hour,WeekSchdTable.mon_fri.lv_min);
  iRetVal += sprintf(pExoBuff+iRetVal,"%02d%02d,",WeekSchdTable.mon_fri.rt_hour,WeekSchdTable.mon_fri.rt_min);
  iRetVal += sprintf(pExoBuff+iRetVal,"%02d%02d,",WeekSchdTable.mon_fri.sl_hour,WeekSchdTable.mon_fri.sl_min);

  iRetVal += sprintf(pExoBuff+iRetVal,"%02d%02d,",WeekSchdTable.sat.wk_hour,WeekSchdTable.sat.wk_min);
  iRetVal += sprintf(pExoBuff+iRetVal,"%02d%02d,",WeekSchdTable.sat.lv_hour,WeekSchdTable.sat.lv_min);
  iRetVal += sprintf(pExoBuff+iRetVal,"%02d%02d,",WeekSchdTable.sat.rt_hour,WeekSchdTable.sat.rt_min);
  iRetVal += sprintf(pExoBuff+iRetVal,"%02d%02d,",WeekSchdTable.sat.sl_hour,WeekSchdTable.sat.sl_min);

  iRetVal += sprintf(pExoBuff+iRetVal,"%02d%02d,",WeekSchdTable.sun.wk_hour,WeekSchdTable.sun.wk_min);
  iRetVal += sprintf(pExoBuff+iRetVal,"%02d%02d,",WeekSchdTable.sun.lv_hour,WeekSchdTable.sun.lv_min);
  iRetVal += sprintf(pExoBuff+iRetVal,"%02d%02d,",WeekSchdTable.sun.rt_hour,WeekSchdTable.sun.rt_min);
  iRetVal += sprintf(pExoBuff+iRetVal,"%02d%02d,",WeekSchdTable.sun.sl_hour,WeekSchdTable.sun.sl_min);

  iRetVal += sprintf(pExoBuff+iRetVal,"%02d%02d%02d,",SmartplugTime.day,SmartplugTime.hour,SmartplugTime.min);
  iRetVal += sprintf(pExoBuff+iRetVal,"%02d",Validity);

  succs = Exosite_Write(pExoBuff, iRetVal);

  if(succs == 1)
  {
    osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
    SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer = 0;
    osi_LockObjUnlock(&g_NvmemLockObj);
  }
  Report("send_exo schedule data\n\r");
}

void ReceiveControlCmd()
{
  t_ExositeReadData ExoReadData;
  UINT8 command = 0;

  if (Exosite_Read(EXO_READ_COMMAND, &ExoReadData))
  {
    DBG_PRINT("Read device status %d,%d\n\r",ExoReadData.control, ExoReadData.command);

    if(ExoReadData.val_flag & (1 << E_EXO_DATA_CONTROL))
    {
      /* Updte based on input */
      if(UpdateDevStatus(ExoReadData.control) >= 0)
      {
        DBG_PRINT("Relay Toggle from exo1\n\r");
      }
    }
    else
    {
      DBG_PRINT("could not read device status\n\r");
    }

    if(ExoReadData.val_flag & (1 << E_EXO_DATA_COMMAND))
    {
      command = ExoReadData.command;
    }
    else
    {
      DBG_PRINT("could not read command status\n\r");
    }
  }
  else
  {
    DBG_PRINT("could not read command device status\n\r");
  }

  /* if command is set then read dev info and schedule table */
  if(command)
  {
    /* Read all device info */
    if(Exosite_Read(EXO_READ_DEV_INFO, &ExoReadData))
    {
      if(ExoReadData.val_flag & (1 << E_EXO_DATA_CONTROL))
      {
        /* Updte based on input */
        if(UpdateDevStatus(ExoReadData.control) >= 0)
        {
          DBG_PRINT("Relay Toggle from exo2\n\r");
        }
        DBG_PRINT("Read device status %d\n\r",ExoReadData.control);
      }
      else
      {
        DBG_PRINT("could not read device status\n\r");
      }

      if(ExoReadData.val_flag & (1 << E_EXO_DATA_POWRTHD))
      {
         osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
         if((ExoReadData.powerthd != *((float*)&SmartPlugNvmmFile.DeviceConfigData.ThresholdPowerServer[0])) && (ExoReadData.powerthd <= MAX_POWER_WATTS))
         {
            memcpy((void*)&SmartPlugNvmmFile.DeviceConfigData.ThresholdPowerServer[0],(void*)&ExoReadData.powerthd,4);
            SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_DEV_CFG|NVMEM_DATA_SEND_DEV_CFG);//update flag indicating NVMEM write required
         }
         osi_LockObjUnlock(&g_NvmemLockObj);
         DBG_PRINT("Read power thresh %f\n\r",ExoReadData.powerthd);
      }
      else
      {
        DBG_PRINT("could not read power threshold value\n\r");
      }

      if(ExoReadData.val_flag & (1 << E_EXO_DATA_ENRGTHD))
      {
         osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
         if((ExoReadData.energythd != *((float*)&SmartPlugNvmmFile.DeviceConfigData.ThresholdEnergyServer[0])) && (ExoReadData.energythd <= MAX_ENERGY_KWH))
         {
            memcpy((void*)&SmartPlugNvmmFile.DeviceConfigData.ThresholdEnergyServer[0],(void*)&ExoReadData.energythd,4);
            SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_DEV_CFG|NVMEM_DATA_SEND_DEV_CFG);//update flag indicating NVMEM write required
         }
         osi_LockObjUnlock(&g_NvmemLockObj);
         DBG_PRINT("Read energy thresh %f\n\r",ExoReadData.energythd);
      }
      else
      {
        DBG_PRINT("could not read energy threshold value\n\r");
      }

      if(ExoReadData.val_flag & (1 << E_EXO_DATA_POWMODE))
      {
         osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
         if((ExoReadData.psmode != SmartPlugNvmmFile.DeviceConfigData.updateinterval.IsPowerSavingServer) && (ExoReadData.psmode <= 1))
         {
           SmartPlugNvmmFile.DeviceConfigData.updateinterval.IsPowerSavingServer = ExoReadData.psmode;
           SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_DEV_CFG|NVMEM_DATA_SEND_DEV_CFG);//update flag indicating NVMEM write required

           if(ExoReadData.psmode == 1)
           {
              if(SmartPlugNvmmFile.DeviceConfigData.updateinterval.IntervalServer < MIN_UPDATERATE_IN_PS_MODE)
              {
                SmartPlugNvmmFile.DeviceConfigData.updateinterval.IntervalServer = MIN_UPDATERATE_IN_PS_MODE;
              }
           }
           else
           {
             SmartPlugNvmmFile.DeviceConfigData.updateinterval.IntervalServer = DEFAULT_UPDATE_INTERVAL;
           }
         }
         osi_LockObjUnlock(&g_NvmemLockObj);
         DBG_PRINT("Read power mode %d\n\r",ExoReadData.psmode);
      }
      else
      {
        DBG_PRINT("could not read power mode value\n\r");
      }

      if(ExoReadData.val_flag & (1 << E_EXO_DATA_REPOINT))
      {
         osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
         if((ExoReadData.interval != SmartPlugNvmmFile.DeviceConfigData.updateinterval.IntervalServer) && (ExoReadData.interval <= MAX_UPDATE_INTERVAL))
         {
           if(((1 == SmartPlugNvmmFile.DeviceConfigData.updateinterval.IsPowerSavingServer) &&\
              (ExoReadData.interval > MIN_UPDATERATE_IN_PS_MODE)) ||\
                (0 == SmartPlugNvmmFile.DeviceConfigData.updateinterval.IsPowerSavingServer))
           {
              SmartPlugNvmmFile.DeviceConfigData.updateinterval.IntervalServer = ExoReadData.interval;
              SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_DEV_CFG|NVMEM_DATA_SEND_DEV_CFG);//update flag indicating NVMEM write required
           }
         }
         osi_LockObjUnlock(&g_NvmemLockObj);

         DBG_PRINT("Read interval value %d\n\r",ExoReadData.interval);
      }
      else
      {
        DBG_PRINT("could not read interval value\n\r");
      }

      /* read schedule table */
      if(ExoReadData.val_flag & (1 << E_EXO_DATA_SCHEDUL))
      {
         UINT32 MinsInaWeekRcvd, MinsInaWeek;
         INT32 DeltaTime = 0;

         osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
         if((SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer == 0)&&\
              (SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToAndrid == 0))
         {
           MinsInaWeekRcvd = (ExoReadData.schdTime.day*24*60) + (ExoReadData.schdTime.hour*60) + ExoReadData.schdTime.min;
           MinsInaWeek = (SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.day*24*60) +\
              (SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.hour*60) + SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.min;

           DeltaTime = MinsInaWeekRcvd - MinsInaWeek;
           if((abs(DeltaTime) > (7*24*60/2)) && (DeltaTime < 0))
           {
             DeltaTime += 7*24*60;
           }//week rollover check

           if((ExoReadData.schdValidity != SmartPlugNvmmFile.DeviceConfigData.Schedule.Validity)||\
              ((abs(DeltaTime) <= 5) && (1 == SmartPlugNvmmFile.DeviceConfigData.Schedule.Validity)))//5min transit delay
           {
              SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime = ExoReadData.schdTime;
              SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule = ExoReadData.schedule;
              SmartPlugNvmmFile.DeviceConfigData.Schedule.Validity = ExoReadData.schdValidity;
              SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer = 1;
              SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToAndrid = 1;
              SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_DEV_CFG|NVMEM_DATA_SEND_SCHEDULE_TABLE);//update flag indicating NVMEM write required
           }
         }
         osi_LockObjUnlock(&g_NvmemLockObj);

         DBG_PRINT("Read schedule validity %d, %d\n\r", ExoReadData.schdValidity, DeltaTime);
      }
      else
      {
        DBG_PRINT("could not read schedule\n\r");
      }
    }
    else
    {
      DBG_PRINT("could not read all device status\n\r");
    }

    /* Clear command status in server */
    SendCommandClearToServer();
  }
}

int UpdateDevStatus( UINT8 Relay_staus)
{
  int ret_val = -1;

  osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
  /* Updte based on input */
  if((Relay_staus != SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.Device_status) &&\
      (SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer == 0)&&\
      (SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToAndrid == 0) && (Relay_staus <= 1))
  {
    if(Relay_staus == 1)
    {
      TurnOnDevice();
    }
    else
    {
      TurnOffDevice();
    }
    //Low prio task using it
    SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer = 1;
    SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToAndrid = 1;
    SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.Device_status = Relay_staus;
    /* If relay state is midified which will override schedule table */
    SmartPlugNvmmFile.DeviceConfigData.Schedule.Validity = 0;
    SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer = 1;
    SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToAndrid = 1;

    SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_DEV_CFG|NVMEM_DATA_SEND_DEV_CFG|NVMEM_DATA_SEND_SCHEDULE_TABLE);//update flag indicating NVMEM write required
    ret_val = 1;
  }
  osi_LockObjUnlock(&g_NvmemLockObj);

  return ret_val;
}

void ManageSmartplugTime( void )
{
  UINT32 CriticalMask;
  UINT32 TimeStamp;
  static UINT32 PrevTimeStamp, TimeSec;

  /* Read Latest Time stamp in critical region */
  CriticalMask = osi_EnterCritical();
    TimeStamp = g_ulDeviceTimerInSec;
  osi_ExitCritical(CriticalMask);

  if((TimeStamp - PrevTimeStamp) >= 1)
  {
    TimeSec += (TimeStamp - PrevTimeStamp);
  }

  PrevTimeStamp = TimeStamp;

  if(TimeSec >= 60)
  {
    TimeSec -= 60;
    osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
    SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.min++;
    if(SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.min >= 60)
    {
      SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.min = 0;
      SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.hour++;
      if(SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.hour >= 24)
      {
        SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.hour = 0;
        SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.day++;
        if(SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.day >= 7)
        {
          SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime.day = 0;
        }
      }
    }
    osi_LockObjUnlock(&g_NvmemLockObj);
  }
}

void ManageScheduleTable( void )
{
  t_weekschedule *p_weekSch, WeekSchdTable;
  t_smartplugtime *p_Time, SmartplugTime;
  UINT32 MinsInaWeek_wk = 0xFFFF;//default
  UINT32 MinsInaWeek_lv = 0xFFFF;
  UINT32 MinsInaWeek_rt = 0xFFFF;
  UINT32 MinsInaWeek_sl = 0xFFFF;
  UINT32 MinsInaWeek = 0;
  UINT8 RelayState = 0xFF;

  osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
  WeekSchdTable = SmartPlugNvmmFile.DeviceConfigData.Schedule.WeekSchedule;
  SmartplugTime = SmartPlugNvmmFile.DeviceConfigData.SmartPlugTime;
  osi_LockObjUnlock(&g_NvmemLockObj);

  p_weekSch = &WeekSchdTable;
  p_Time = &SmartplugTime;
  MinsInaWeek = (p_Time->day*24*60) + (p_Time->hour*60) + p_Time->min;

  switch(p_Time->day)
  {
    /* Monday to friday */
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      if(24 != p_weekSch->mon_fri.wk_hour)
        MinsInaWeek_wk = (p_Time->day*24*60)+(p_weekSch->mon_fri.wk_hour*60) + p_weekSch->mon_fri.wk_min;
      if(24 != p_weekSch->mon_fri.lv_hour)
        MinsInaWeek_lv = (p_Time->day*24*60)+(p_weekSch->mon_fri.lv_hour*60) + p_weekSch->mon_fri.lv_min;
      if(24 != p_weekSch->mon_fri.rt_hour)
        MinsInaWeek_rt = (p_Time->day*24*60)+(p_weekSch->mon_fri.rt_hour*60) + p_weekSch->mon_fri.rt_min;
      if(24 != p_weekSch->mon_fri.sl_hour)
        MinsInaWeek_sl = (p_Time->day*24*60)+(p_weekSch->mon_fri.sl_hour*60) + p_weekSch->mon_fri.sl_min;
      break;
    /* Saturday */
    case 6:
      if(24 != p_weekSch->sat.wk_hour)
        MinsInaWeek_wk = (p_Time->day*24*60)+(p_weekSch->sat.wk_hour*60) + p_weekSch->sat.wk_min;
      if(24 != p_weekSch->sat.lv_hour)
        MinsInaWeek_lv = (p_Time->day*24*60)+(p_weekSch->sat.lv_hour*60) + p_weekSch->sat.lv_min;
      if(24 != p_weekSch->sat.rt_hour)
        MinsInaWeek_rt = (p_Time->day*24*60)+(p_weekSch->sat.rt_hour*60) + p_weekSch->sat.rt_min;
      if(24 != p_weekSch->sat.sl_hour)
        MinsInaWeek_sl = (p_Time->day*24*60)+(p_weekSch->sat.sl_hour*60) + p_weekSch->sat.sl_min;
      break;
    /* Sunday */
    case 0:
      if(24 != p_weekSch->sun.wk_hour)
        MinsInaWeek_wk = (p_weekSch->sun.wk_hour*60) + p_weekSch->sun.wk_min;
      if(24 != p_weekSch->sun.lv_hour)
        MinsInaWeek_lv = (p_weekSch->sun.lv_hour*60) + p_weekSch->sun.lv_min;
      if(24 != p_weekSch->sun.rt_hour)
        MinsInaWeek_rt = (p_weekSch->sun.rt_hour*60) + p_weekSch->sun.rt_min;
      if(24 != p_weekSch->sun.sl_hour)
        MinsInaWeek_sl = (p_weekSch->sun.sl_hour*60) + p_weekSch->sun.sl_min;
      break;
    default:
      break;
  }

  /* Check for sleep time */
  if(MinsInaWeek_wk != 0xFFFF)
  {
    if(MinsInaWeek_lv != 0xFFFF)
    {
      if(((MinsInaWeek >= MinsInaWeek_sl) && (MinsInaWeek_sl != 0xFFFF))||\
        (MinsInaWeek < MinsInaWeek_wk))
      {
        /* Switch OFF Relay */
        RelayState = 0;
      }
      /* Check for wakeup time */
      else if((MinsInaWeek >= MinsInaWeek_wk) && (MinsInaWeek < MinsInaWeek_lv))
      {
        /* Switch ON Relay */
        RelayState = 1;
      }
      /* Check for return time */
      else if((MinsInaWeek >= MinsInaWeek_lv) && \
        ((MinsInaWeek < MinsInaWeek_rt) && (MinsInaWeek_rt != 0xFFFF)))
      {
        /* Switch ON Relay */
        RelayState = 0;
      }
      /* Check for leave time */
      else if((MinsInaWeek >= MinsInaWeek_rt) && (MinsInaWeek_rt != 0xFFFF))
      {
        /* Switch OFF Relay */
        RelayState = 1;
      }
    }
    else
    {
      if(((MinsInaWeek >= MinsInaWeek_sl) && (MinsInaWeek_sl != 0xFFFF))||\
        (MinsInaWeek < MinsInaWeek_wk))
      {
        /* Switch OFF Relay */
        RelayState = 0;
      }
      /* Check for leave time */
      else if(MinsInaWeek >= MinsInaWeek_wk)
      {
        /* Switch OFF Relay */
        RelayState = 1;
      }
    }
  }
  else if(MinsInaWeek_lv != 0xFFFF)
  {
    if(((MinsInaWeek >= MinsInaWeek_rt) && (MinsInaWeek_rt != 0xFFFF))||\
      (MinsInaWeek < MinsInaWeek_lv))
    {
      /* Switch ON Relay */
      RelayState = 1;
    }
    /* Check for wakeup time */
    else if(MinsInaWeek >= MinsInaWeek_lv)
    {
      /* Switch ON Relay */
      RelayState = 0;
    }
  }

  if(1 == RelayState)
  {
    /* Switch ON Relay */
    osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
    /* Updte based on input */
    if((1 != SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.Device_status) &&\
        (SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer == 0)&&\
        (SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToAndrid == 0))
    {
      TurnOnDevice();
      //Low prio task using it
      SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer = 1;
      SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToAndrid = 1;
      SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.Device_status = 1;
      SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_DEV_CFG|NVMEM_DATA_SEND_DEV_CFG);
    }
    osi_LockObjUnlock(&g_NvmemLockObj);
  }
  else if(0 == RelayState)
  {
    /* Switch OFF Relay */
    osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
    /* Updte based on input */
    if((0 != SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.Device_status) &&\
        (SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer == 0)&&\
        (SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToAndrid == 0))
    {
      TurnOffDevice();
      //Low prio task using it
      SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToServer = 1;
      SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.SendStChangeToAndrid = 1;
      SmartPlugNvmmFile.DeviceConfigData.DeviceStatus.Device_status = 0;
      SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_DEV_CFG|NVMEM_DATA_SEND_DEV_CFG);
    }
    osi_LockObjUnlock(&g_NvmemLockObj);
  }
  else
  {
    /* At any point of time current time must be within range otherwise invalidate table */
    osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
    SmartPlugNvmmFile.DeviceConfigData.Schedule.Validity = 0;
    SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToServer = 1;
    SmartPlugNvmmFile.DeviceConfigData.Schedule.SendStChangeToAndrid = 1;

    SmartPlugNvmmFile.NvmmFileUpdated |= (NVMEM_UPDATED_DEV_CFG|NVMEM_DATA_SEND_SCHEDULE_TABLE);
    osi_LockObjUnlock(&g_NvmemLockObj);
  }
}



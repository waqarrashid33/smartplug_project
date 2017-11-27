//*****************************************************************************
// File: smartplug_task.h
//
// Description: SmartPlug tasks handler header file of Smartplug gen-1 application
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

#ifndef __SMARTPLUG_H__
#define __SMARTPLUG_H__

#define DEFAULT_UPDATE_INTERVAL                 1

#define DEVICEMARK                              "devconf!"
#define METROLOGYMARK                           "metrlgy!"
#define INVALIDMARK                             "invalid!"

#define SMARTPLUG_DEVICE_NAME                   "ti"
#define SMARTPLUG_TI_NAME                       "cc3200smartplug_"
#define SMARTPLUG_mDNS_SERV                     "._device-info._tcp.local"
#define TTL_MDNS_SERV                           180

#define MAX_DEV_NAME_SIZE                       30
#define DEVICE_MARK_SIZE                        8
#define METROLOGY_MARK_SIZE                     8
#define INVALID_MARK_SIZE                       8

/* TASK Priority */
#define SMARTPLUG_BASE_TASK_PRIO                1  /* Lowest */
#define ANDROID_SEND_TASK_PRIO                  2
#define EXOSITE_RECV_TASK_PRIO                  3
#define EXOSITE_SEND_TASK_PRIO                  4
#define ANDROID_RECV_TASK_PRIO                  5
#define TIME_HANDLER_TASK_PRIO                  6
#define SIMPLINK_SPAWN_TASK_PRIO                7  /* Highest */

/***************************************************
      Data sender task flags
***************************************************/
//16 bits for Android & 16 bits for server
#define SEND_METROLOGY_DATA                    (UINT32)0x0001
#define SEND_AVERAGE_POWER                     (UINT32)0x0002
#define SEND_THRESHOLD_PW_WARNING              (UINT32)0x0004
#define SEND_DEVICE_STATUS                     (UINT32)0x0008
#define SEND_THRESHOLD_EN_WARNING              (UINT32)0x0010
#define SEND_METROLOGY_WARNING                 (UINT32)0x0020
#define SEND_EXOSITE_CFG                       (UINT32)0x0040
#define SEND_METROLOGY_CFG                     (UINT32)0x0080
#define SEND_SCHEDULE_TABLE                    (UINT32)0x0100
#define SEND_24HR_ENERGY                       (UINT32)0x0200
#define SEND_EXO_SSL_CA_DONE                   (UINT32)0x0400
#define SEND_ANDROID_MASK                      (UINT32)0x07FF //update accordingly

#define SEND_METROLOGY_DATA_SERVER             (UINT32)0x00010000
#define SEND_AVERAGE_POWER_SERVER              (UINT32)0x00020000
#define SEND_THRESHOLD_PW_WARNING_SERVER       (UINT32)0x00040000
#define SEND_DEVICE_STATUS_SERVER              (UINT32)0x00080000
#define SEND_THRESHOLD_EN_WARNING_SERVER       (UINT32)0x00100000
#define SEND_SCHEDULE_TABLE_SERVER             (UINT32)0x00200000
#define SEND_SERVER_EXO_MASK                   (UINT32)0x003F0000 //update accordingly

#define NVMEM_UPDATED_DEV_CFG                  (UINT32)0x0001
#define NVMEM_UPDATED_EXO_CFG                  (UINT32)0x0002
#define NVMEM_UPDATED_MET_CFG                  (UINT32)0x0004

#define NVMEM_DATA_SEND_DEV_CFG                (UINT32)0x0008
#define NVMEM_DATA_SEND_EXO_CFG                (UINT32)0x0010
#define NVMEM_DATA_SEND_MET_CFG                (UINT32)0x0020
#define NVMEM_DATA_SEND_24HR_ENERGY            (UINT32)0x0040
#define NVMEM_DATA_SEND_SCHEDULE_TABLE         (UINT32)0x0080
#define NVMEM_DATA_SEND_EXO_SSL_CA_DONE        (UINT32)0x0100

#define MAX_METROLOGY_ERROR_REP_INT            3
#define MAX_METROLOGY_MODULE_ERROR             30 //DKS 10
#define MAX_EXOSITE_TX_THREAD_ERROR            60
#define MAX_EXOSITE_RX_THREAD_ERROR            60
#define MAX_ANDROID_TX_THREAD_ERROR            60
#define MAX_ANDROID_RX_THREAD_ERROR            60
#define MAX_BAD_TCP_NWP                        3
#define MAX_BAD_TCP_ERROR_CHECK_TIME           30
#define MAX_MDNS_ERROR                         1
#define MAX_EXOSITE_RX_THREAD_WAIT_TIME        5

#define MAX_POWER_WATTS                        2500.0 //2.5K watts
#define MAX_ENERGY_KWH                         429400.0 //Max range of accumulated energy
#define MAX_UPDATE_INTERVAL                    60

typedef struct
{
  UINT8 IsPowerSaving;
  UINT8 IsPowerSavingServer;
  UINT8 Interval;
  UINT8 IntervalServer;

}t_updateinterval;

typedef struct
{
  UINT8 DevName[MAX_DEV_NAME_SIZE];
  UINT8 DevNameLen;
  UINT8 SmartconfSsidLen;
  UINT8 SmartconfSsid[32];

}t_configuration;

typedef struct
{
  UINT8 Device_status;
  UINT8 SendStChangeToServer;
  UINT8 SendStChangeToAndrid;

}t_deviceStatus;

typedef struct t_smartplugtime
{
  UINT8   day;  //day in week 0-6
  UINT8   hour;
  UINT8   min;

} t_smartplugtime;

typedef struct t_smartplugdate
{
  UINT16  year;
  UINT8   month;
  UINT8   date;
  UINT8   validity;

} t_smartplugdate;

typedef struct t_switchtbl
{
  UINT8   wk_hour; //wakeup
  UINT8   wk_min;

  UINT8   lv_hour; //leave
  UINT8   lv_min;

  UINT8   rt_hour; //return
  UINT8   rt_min;

  UINT8   sl_hour; //sleep
  UINT8   sl_min;

} t_switchtbl;

typedef struct t_weekschedule
{
  t_switchtbl mon_fri; //weekdays (1-5)
  t_switchtbl sat;     //weekend  (6)
  t_switchtbl sun;     //weekend  (0)

} t_weekschedule;

typedef struct
{
  t_weekschedule WeekSchedule;
  UINT8 Validity;
  UINT8 SendStChangeToServer;
  UINT8 SendStChangeToAndrid;

}t_smartplugschedule;

typedef struct
{
  t_updateinterval      updateinterval;
  UINT8                 ThreasholdPower[4];
  UINT8                 ThreasholdEnergy[4];
  UINT8                 ThresholdPowerServer[4];
  UINT8                 ThresholdEnergyServer[4];
  t_configuration       DevConfig;
  t_deviceStatus        DeviceStatus;
  t_smartplugschedule   Schedule;
  t_smartplugdate       SmartPlugDate;
  t_smartplugtime       SmartPlugTime;
  char                  mark[8];//string holds "devconf"

}t_config;

typedef struct
{
  char    mDNSServNameUnReg[64];
  UINT32  mDNSBroadcastTime;
  UINT16  mDNSBroadcastUpdateInterval;
  UINT8   mDNSServNameUnRegLen;

}t_mDNSService;

typedef volatile struct t_SmartPlugNvmmBuffer
{
  t_config            DeviceConfigData;
  exosite_meta        ExositeMetaData;
  t_MetrologyConfig   MetrologyConfigData;
  UINT32              NvmmFileUpdated;

} t_SmartPlugNvmmFile;

typedef struct t_exosite_meta_send
{
  char vendorname[META_VNAME_SIZE+1];          // vendor name
  char modelname[META_MNAME_SIZE+1];           // model name
  char mac_address[6];                       // mac address
  char exostatus[META_EXO_STS_SIZE];         // exosite status

} t_exosite_meta_send;

typedef struct t_SmartPlugSendData
{
  t_config                     DeviceConfigData;
  t_exosite_meta_send         ExositeMetaData;
  t_MetrologyConfig           MetrologyConfigData;
  t_SmartPlugMetrologyComm    SmartPlugMetrologyData;

  /* Data sender indication */
  UINT32                      DataSender;
  UINT32                      TimeStamp;

} t_SmartPlugSendData;

typedef volatile struct t_SmartplugErrorRecovery
{
  UINT8               MetrologyModuleError; //HIB reset
  UINT8               ExositeTxThreadError; //Nwp reset
  UINT8               ExositeRxThreadError; //Nwp reset
  UINT8               AndroidTxThreadError; //Nwp reset
  UINT8               AndroidRxThreadError; //Nwp reset
  UINT8               BadTCPErrorCheckCount; //Nwp reset
  UINT8               MdnsError; //Nwp reset

} t_SmartplugErrorRecovery;

extern void SmartPlugTask(void *);
extern void TimeHandlerTask(void * param);
extern void NvmemInit(void );
extern void SmartPlug1SecTask( void );

extern void InitSmartPlug ( void );
extern void DeInitSmartPlug ( void );

#endif   //__SMARTPLUG_H__


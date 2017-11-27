//*****************************************************************************
// File: clienthandler.h
//
// Description: Android & Exosite client handler header file of Smartplug gen-1 application
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


#ifndef __CLIENT_H__
#define __CLIENT_H__


/*************************************************************
                    Device to Android MACRO
*************************************************************/
#define D2A_METORLOGY_DATA              (UINT16)0x7A01
#define D2A_AVERAGE_ENERGY              (UINT16)0x7A02
#define D2A_DEVICE_STATUS               (UINT16)0x7A04
#define D2A_WARNING_MESSAGES            (UINT16)0x7A08
#define D2A_AVERAGE_ENERGY_24HRS        (UINT16)0x7A10
#define D2A_SCHEDULE_TABLE              (UINT16)0x7A20
#define D2A_EXOSITE_REG_INFO            (UINT16)0x7A40
#define D2A_METROLOGY_CAL_INFO          (UINT16)0x7A80
#define D2A_EXO_SSL_CA_UPDATE_DONE      (UINT16)0x7A91

/*************************************************************
                    Android to Device MACRO
*************************************************************/
#define A2D_SET_DEV_ON_OFF                   (UINT16)0xA701
#define A2D_SET_THRESHOLD                    (UINT16)0xA702
#define A2D_SET_PS_MODE                      (UINT16)0xA704
#define A2D_REQ_24HRS_AVG_ENERGY             (UINT16)0xA708
#define A2D_REQ_SCHEDULE_TABLE               (UINT16)0xA710
#define A2D_REQ_METROLOGY_CAL_INFO           (UINT16)0xA720
#define A2D_REQ_EXOSITE_REG_INFO             (UINT16)0xA740
#define A2D_REQ_DEV_INFO                     (UINT16)0xA780
#define A2D_SET_SCHEDULE_TABLE               (UINT16)0xA791
#define A2D_SET_METROLOGY_CAL_INFO           (UINT16)0xA792
#define A2D_SET_SECURED_EXO_CONN             (UINT16)0xA794
#define A2D_SET_EXO_SSL_CA_CERT              (UINT16)0xA798

#define A2D_SET_DEV_ON_OFF_SIZE              (UINT8)3
#define A2D_SET_THRESHOLD_SIZE               (UINT8)10
#define A2D_SET_PS_MODE_SIZE                 (UINT8)4
#define A2D_REQ_24HRS_AVG_ENERGY_SIZE        (UINT8)2
#define A2D_REQ_SCHEDULE_TABLE_SIZE          (UINT8)2
#define A2D_REQ_METROLOGY_CAL_INFO_SIZE      (UINT8)2
#define A2D_REQ_EXOSITE_REG_INFO_SIZE        (UINT8)2
#define A2D_REQ_DEV_INFO_SIZE                (UINT8)2
#define A2D_SET_SCHEDULE_TABLE_SIZE          (UINT8)30
#define A2D_SET_METROLOGY_CAL_INFO_SIZE      (UINT8)10
#define A2D_SET_SECURED_EXO_CONN_SIZE        (UINT8)3
#define A2D_SET_EXO_SSL_CA_CERT_SIZE         (UINT8)4 //variable length

#define MIN_UPDATERATE_IN_PS_MODE            5

extern void ProcessData(unsigned char * aucrecvdata);
extern void SendMetrologyData(s_ConnectionStatus *, t_SmartPlugSendData *);
extern void SendAvrgEnergy(s_ConnectionStatus *, t_SmartPlugSendData *);
extern void SendDeviceStatus(s_ConnectionStatus *, t_SmartPlugSendData *);
extern void sendWarning(s_ConnectionStatus *, t_SmartPlugSendData *);
extern void RequestConfiguration(s_ConnectionStatus *);
extern void SendAvrgEnergyToServer(t_SmartPlugSendData *);
extern void SendDeviceStatusToServer(void);
extern void SendMetrologyDataToServer(t_SmartPlugSendData *);
extern void sendPowerWarningToServer();
extern void sendEnergyWarningToServer();
extern void SendCommandClearToServer(void);
extern void ReceiveControlCmd();
extern void send24HrAvgEnergy(s_ConnectionStatus *soc, t_SmartPlugSendData *pSmartPlugSendData);
extern void sendScheduleTable(s_ConnectionStatus *soc, t_SmartPlugSendData *pSmartPlugSendData);
extern void sendCloudInfo(s_ConnectionStatus *soc, t_SmartPlugSendData *pSmartPlugSendData);
extern void sendMetrologyCalInfo(s_ConnectionStatus *soc, t_SmartPlugSendData *pSmartPlugSendData);
extern void sendCloudCAUpdateSuccess(s_ConnectionStatus *soc, t_SmartPlugSendData *pSmartPlugSendData);
extern void SendScheduleTableToServer();
extern int  UpdateDevStatus( UINT8 Relay_staus);
extern void ManageSmartplugTime( void );
extern void ManageScheduleTable( void );

#endif   //__CLIENT_H__

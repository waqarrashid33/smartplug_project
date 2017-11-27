//*****************************************************************************
// File: cc3200.h
//
// Description: cc3200 network & device handler header file of Smartplug gen-1 application
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

#ifndef __CC3200_H__
#define __CC3200_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#endif

//*****************************************************************************
// State Machine values
//*****************************************************************************

#define DATE        7     // Current Date
#define MONTH       12    // Month 1-12
#define YEAR        2014  // Current year
#define HOUR        7     // Time - hours
#define MINUTE      10    // Time - minutes
#define DAY         1     // Time – day in week

#define M_ONE_HOUR_IN_SEC               3600

#define METROLOGY_SFT_INT_NUM           25   // Software Interrupt 25 for metrology module

  //*************** clock frequency 80Mhz   ******************//
#define MAP_SysCtlClockGet              80000000
#define MILLISECONDS_TO_TICKS(ms)       ((MAP_SysCtlClockGet / 1000) * (ms))
#define WDT_PERIOD_IN_MS                5000 /* For 10 secs set half of that - WDT reset happens in 2nd interrupt */

#define DELAY(x)                        UtilsDelay(((MAP_SysCtlClockGet/2000)*x)/3)  //80MHz/2 - ram frequency
#define OSI_DELAY(x)                    osi_Sleep(x); //os idle

#define WLAN_DEL_ALL_PROFILES           0xFF
#define SMARTPLUG_1SEC_TASK_MOD_VAL     (10)

//*****************************************************************************
// Defining Timer Load Value. Corresponds to 1ms for time stamp
//*****************************************************************************
#define PERIODIC_TIME_STAMP_CYCLES      (MAP_SysCtlClockGet/1000) //80M/1000

//*****************************************************************************
// Defining Timer Load Value. Corresponds to 100 ms.
//*****************************************************************************
#define PERIODIC_BROADCAST_CYCLES       (MAP_SysCtlClockGet/SMARTPLUG_1SEC_TASK_MOD_VAL) //80M/10

//*****************************************************************************
// CC3200 State Machine Definitions
//*****************************************************************************
enum cc3200StateEnum
{
  CC3200_UNINIT           = 0x0,  // CC3200 Driver Uninitialized
  CC3200_INIT             = 0x1,  // CC3200 Driver Initialized
  CC3200_ASSOC            = 0x2,  // CC3200 Associated to AP
  CC3200_IP_ALLOC         = 0x4,  // CC3200 has IP Address
  CC3200_GOOD_TCP         = 0x8,  // CC3200 has good TCP connection
  CC3200_GOOD_INTERNET    = 0x10 // CC3200 has good internet connection
};

typedef enum
{
  NO_BUTTON   = 0x0,  // No button pressed
  SMART_CONF_BUTTON,  // smartconf pressed
  APPROV_BUTTON,      // APProv pressed
  WPS_BUTTON          // WPS pressed
} ButtonState;

#define CC3200_STA_P2P_CL_IPV4_ADDR_GET(ip,mask,gateway,dns,isDHCP)    {  \
                     unsigned char len = sizeof(_NetCfgIpV4AP_Args_t);                       \
                     unsigned char dhcpIsOn = 0;                                              \
                     _NetCfgIpV4AP_Args_t ipV4 = {0};                                               \
                     sl_NetCfgGet(SL_IPV4_STA_P2P_CL_GET_INFO,&dhcpIsOn,&len,(unsigned char *)&ipV4);     \
                     *ip= *((UINT32*)&ipV4.ipV4[0]);                                                       \
                         *mask= *((UINT32*)ipV4.ipV4Mask);                                                 \
                         *gateway= *((UINT32*)ipV4.ipV4Gateway);                                           \
                         *dns= *((UINT32*)ipV4.ipV4DnsServer);                                             \
                     *isDHCP = dhcpIsOn;                                                  \
                                                            }
#define CC3200_MAC_ADDR_GET(currentMacAddress)    { \
                    unsigned char macAddressLen = SL_MAC_ADDR_LEN; \
                    sl_NetCfgGet(SL_MAC_ADDRESS_GET,NULL,&macAddressLen,(unsigned char *)currentMacAddress); \
                                               }

extern void LedConfiguration( void );
extern char IsSmartConfButtonPressed();
extern char IsAPProvButtonPressed();
extern char IsWpsButtonPressed();
extern ButtonState IsButtonPressed(void);
extern char GetRelayState();
extern void ManageSwitches(void);

extern void TimerInit();

#ifdef USE_FREERTOS
extern void vAssertCalled( const char *pcFile, unsigned long ulLine );
extern void vApplicationIdleHook( void );
extern void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName );
#endif //#ifdef USE_FREERTOS

extern void EnableTimer();
extern void DisableTimer();
extern void EnableBroadcastTimer();
extern void DisableBroadcastTimer();
extern void EnableButtonSenseTimer();
extern void DisableButtonSenseTimer();
extern void EnableTimeStampTimer();
extern void DisableTimeStampTimer();

extern void InitSimpleLink(void);
extern void DeInitSimpleLink(void);

extern void UnsetCC3200MachineState(unsigned char stat);
extern void SetCC3200MachineState(unsigned char stat);
extern void ResetCC3200StateMachine();
extern unsigned char GetCC3200State();

extern void SimpleLinkWlanEventHandler(SlWlanEvent_t *pArgs);
extern void SimpleLinkNetAppEventHandler(SlNetAppEvent_t* pArgs);
extern void SimpleLinkHttpServerCallback(SlHttpServerEvent_t \
            *pSlHttpServerEvent, SlHttpServerResponse_t *pSlHttpServerResponse);
extern void SimpleLinkSockEventHandler(SlSockEvent_t *pSock);

extern void ConnectToAP(void);
extern int  WaitToConnect();

extern void TurnOnDevice();
extern void TurnOffDevice();

extern int IpConfigGet(unsigned long *aucIP, unsigned long *aucSubnetMask,
                unsigned long *aucDefaultGateway, unsigned long *aucDNSServer);

extern int mDNSBroadcast(unsigned char RegOrUnReg);
extern void UDMAInit();
extern void WatchdogIntHandler(void);
extern void ClearWDT(void);
extern void WDTInit(void);
extern int WlanDisconnect(void);
extern void HIBReset(void);
extern void ManageLEDIndication( void );
extern void ToggleLEDIndication( void );
extern void AndroidDataTranLEDInd( void );
extern void ExositeCIKFailLEDInd( void );
extern void SoftwareIntRegister(unsigned long ulSftIntNo, unsigned char ucPriority, void (*pfnHandler)(void));
extern void SoftwareIntTrigger(unsigned long ulSftIntNo);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif
#endif //  __3200_H__

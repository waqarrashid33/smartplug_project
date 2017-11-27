//*****************************************************************************
// File: cc3200.c
//
// Description: cc3200 network & device handler functions of Smartplug gen-1 application
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
#include <string.h>
#include "hw_types.h"
#include "cpu.h"
#include <osi.h>
#include "user.h"
#include "gpio_if.h"
#include <prcm.h>
#include <rom_map.h>
#include "hw_memmap.h"
#include "timer.h"
#include "simplelink.h"
#include <protocol.h>
#include <gpio.h>
#include <udma.h>
#include "utils.h"
#include "wlan.h"
#include "inc/hw_ints.h"
#include "inc/hw_wdt.h"
#include "wdt.h"
#include "hw_gpio.h"
#include "rom_map.h"
#include "pin.h"
#include "prcm.h"
#include "uart_logger.h"
#include "smartconfig.h"
#include "exosite_task.h"
#include "interrupt.h"
#include "metrology.h"
#include "exosite_meta.h"
#include "smartplug_task.h"
#include "android_task.h"
#include "clienthandler.h"
#include "cc3200.h"
#include "pinmux.h"

//*****************************************************************************
// Variables to store TIMER Port,Pin values
//*****************************************************************************
#define NOT_USED(x)             x=x


extern volatile unsigned char g_smartConfigDone;
volatile UINT32 TimerMsCount = 0;
		 UINT32 g_SmartPlugIpAdd = 0;
volatile UINT8  g_ClearWatchdog = 1;
volatile char g_SmartPlugWps = 0;
volatile char g_SmartPlugSmartconf = 0;
volatile char g_SmartPlugAPProv = 0;
volatile char g_SmartPlugRelayState = 0;
volatile UINT32 g_SmartconfButtonSense = 0;
volatile UINT32 g_RelayStateButtonSense = 0;
volatile unsigned long g_ulDeviceTimerInSec = 0;
volatile unsigned long g_MsCounter = 0;
volatile UINT8  g_StationConnToAP = 0;
volatile unsigned char g_cc3200state = CC3200_UNINIT;
extern   t_SmartPlugNvmmFile SmartPlugNvmmFile;

volatile LedIndicationName  g_SmartplugLedCurrentState  = NO_LED_IND;
volatile LedIndicationName  g_SmartplugLedPreviousState = NO_LED_IND;
extern   SmartPlugGpioConfig_t  SmartPlugGpioConfig;
extern   t_mDNSService mDNSService;
extern   t_SmartplugErrorRecovery g_SmartplugErrorRecovery;

/****************************************************************************
                              Synch Objects
****************************************************************************/
extern OsiMsgQ_t           TimeHandlerTaskMsgQ;
extern OsiLockObj_t        g_NvmemLockObj;

#define DBG_PRINT               Report

/* ******* DMA initialization for simple link and metrology modules ***** */
#define MAX_NUM_CH                64  //32*2 entries
#define CTL_TBL_SIZE              64  //32*2 entries

#define UDMA_CH5_BITID            (1<<5)

#ifdef ccs
#pragma DATA_ALIGN(gpCtlTbl, 1024)
#else
#pragma data_alignment=1024
#endif
tDMAControlTable gpCtlTbl[CTL_TBL_SIZE];
/* ******* DMA initialization for simple link and metrology modules ***** */

void DmaErrIntHandler(void);
/* ********************************************************************** */

#ifdef USE_FREERTOS
//*****************************************************************************
//! Application defined hook (or callback) function - the tick hook.
//! The tick interrupt can optionally call this
//! \param  none
//! \return none
//*****************************************************************************
void
vApplicationTickHook( void )
{
}

void vApplicationMallocFailedHook( void )
{
  Report("Malloc Failed \n\r");
  while(1)
  {
  }
}

//*****************************************************************************
//! Application defined hook (or callback) function - assert
//! \param  none
//! \return none
//*****************************************************************************
void
vAssertCalled( const char *pcFile, unsigned long ulLine )
{
  Report("Assert Called \n\r");
  while(1)
  {
  }
}

//*****************************************************************************
//! Application defined idle task hook
//! \param  none
//! \return none
//*****************************************************************************
void
vApplicationIdleHook( void )
{
  // Request Sleep
  //CPUwfi();//DKS debug
}

//*****************************************************************************
//! Application provided stack overflow hook function.
//! \param  handle of the offending task
//! \param  name  of the offending task
//! \return none
//*****************************************************************************
void
vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
{
  Report("Stack Overflow \n\r");
  for( ;; );
}
#endif //#ifdef USE_FREERTOS

//*****************************************************************************
//! This function handles socket events indication
//! \param[in]      pSock - Pointer to Socket Event Info
//! \return None
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
}

//*****************************************************************************
//! IpConfigGet  Get the IP Address of the device.
//! \param  pulIP IP Address of Device
//! \param  pulSubnetMask Subnetmask of Device
//! \param  pulDefaultGateway Default Gateway value
//! \param  pulDNSServer DNS Server
//! \return none
//*****************************************************************************
int
IpConfigGet(unsigned long *pulIP,
			unsigned long *pulSubnetMask,
            unsigned long *pulDefaultGateway,
			unsigned long *pulDNSServer)
{
  unsigned char isDhcp = 0;
  CC3200_STA_P2P_CL_IPV4_ADDR_GET(pulIP,pulSubnetMask,pulDefaultGateway,pulDNSServer,&isDhcp);

  Report("IP address %x\n\r", *pulIP);

  NOT_USED(isDhcp);
  return 1;
}

//*****************************************************************************
//!  \brief  sets a state from the state machine
//!  \param  cStat Status of State Machine
//!  \return none
//*****************************************************************************
void
SetCC3200MachineState(unsigned char cStat)
{
  unsigned char cBitmask = cStat;
  g_cc3200state |= cBitmask;
}

//*****************************************************************************
//!  \brief  Unsets a state from the state machine
//!  \param  cStat Status of State Machine
//!  \return none
//*****************************************************************************
void
UnsetCC3200MachineState(unsigned char cStat)
{
  unsigned char cBitmask = cStat;
  g_cc3200state &= ~cBitmask;
}

//*****************************************************************************
//! InitCallBack Function. After sl_Start InitCall back function gets triggered
//! \param None
//! \return None
//*****************************************************************************
void
InitCallBack(UINT32 Input)
{
  SetCC3200MachineState(CC3200_INIT);
}

//*****************************************************************************
//! On Successful completion of Wlan Connect, This function triggers Connection
//! status to be set.
//! \param  WlanEvent pointer indicating Event type
//! \return None
//*****************************************************************************
void
SimpleLinkWlanEventHandler(SlWlanEvent_t *pArgs)
{
  // Handle the WLAN event appropriately
  SlWlanEvent_t *pEvent = pArgs;

  switch(((SlWlanEvent_t*)pArgs)->Event)
  {
    case SL_WLAN_CONNECT_EVENT:

    SetCC3200MachineState(CC3200_ASSOC);
    g_SmartplugLedCurrentState = WLAN_CONNECT_OPER_IND;
    Report("WLAN association done \n\r");
    break;

    case SL_WLAN_DISCONNECT_EVENT:

    UnsetCC3200MachineState((CC3200_ASSOC|CC3200_IP_ALLOC|CC3200_GOOD_TCP|CC3200_GOOD_INTERNET));
    g_SmartplugLedCurrentState = WLAN_CONNECT_OPER_IND;
    Report("WLAN disconnected \n\r");
    break;

    case SL_WLAN_STA_CONNECTED_EVENT:

    g_StationConnToAP = 1;
    Report("STA connected to Simplelink AP \n\r");
    break;

    case SL_WLAN_STA_DISCONNECTED_EVENT:

    g_StationConnToAP = 0;
    Report("STA Disconnected from Simplelink AP \n\r");
    break;

    case SL_WLAN_SMART_CONFIG_COMPLETE_EVENT:
    /* SmartConfig operation finished */
    /* The new SSID that was acquired is: pWlanEventHandler->EventData.smartConfigStartResponse.ssid */
    /* We have the possiblity that also a private token was sent to the Host:
     *  if (pWlanEventHandler->EventData.smartConfigStartResponse.private_token_len)
     *    then the private token is populated: pWlanEventHandler->EventData.smartConfigStartResponse.private_token
     */
    Report("smartconf ssid %s\n\r",pEvent->EventData.smartConfigStartResponse.ssid);
    Report("smartconf dev len %d\n\r",pEvent->EventData.smartConfigStartResponse.private_token_len);
    Report("smartconf dev name %s\n\r",pEvent->EventData.smartConfigStartResponse.private_token);

    /* Store new device name */
    if(pEvent->EventData.smartConfigStartResponse.private_token_len)
    {
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);

      /* Unregister current dev name in mDNS broadcast */
      mDNSBroadcast(0);

      if(pEvent->EventData.smartConfigStartResponse.private_token_len <= (MAX_DEV_NAME_SIZE-1))
      {
        SmartPlugNvmmFile.DeviceConfigData.DevConfig.DevNameLen = \
            pEvent->EventData.smartConfigStartResponse.private_token_len;
      }
      else
      {
        SmartPlugNvmmFile.DeviceConfigData.DevConfig.DevNameLen = MAX_DEV_NAME_SIZE-1;
      }
      memcpy((void *)&SmartPlugNvmmFile.DeviceConfigData.DevConfig.DevName[0],\
            pEvent->EventData.smartConfigStartResponse.private_token,\
                SmartPlugNvmmFile.DeviceConfigData.DevConfig.DevNameLen);
      SmartPlugNvmmFile.DeviceConfigData.DevConfig.DevName[SmartPlugNvmmFile.DeviceConfigData.DevConfig.DevNameLen] = 0;
      SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_UPDATED_DEV_CFG;

      osi_LockObjUnlock(&g_NvmemLockObj);
    }

    /* Store new SSID name */
    if(pEvent->EventData.smartConfigStartResponse.ssid_len)
    {
      osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);

      SmartPlugNvmmFile.DeviceConfigData.DevConfig.SmartconfSsidLen =\
        pEvent->EventData.smartConfigStartResponse.ssid_len;

      memcpy((void *)&SmartPlugNvmmFile.DeviceConfigData.DevConfig.SmartconfSsid[0],\
            pEvent->EventData.smartConfigStartResponse.ssid,\
                SmartPlugNvmmFile.DeviceConfigData.DevConfig.SmartconfSsidLen);
      SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_UPDATED_DEV_CFG;

      osi_LockObjUnlock(&g_NvmemLockObj);
    }

    g_smartConfigDone = 1;
    break;

    case SL_WLAN_SMART_CONFIG_STOP_EVENT:
    /* SmartConfig stop operation was completed */
    g_smartConfigDone = 2;
    break;
  }
}

//*****************************************************************************
//! This function gets triggered when device acquires IP
//! status to be set. When Device is in DHCP mode recommended to use this.
//! \param NetAppEvent Pointer indicating device acquired IP
//! \return None
//*****************************************************************************
void
SimpleLinkNetAppEventHandler(SlNetAppEvent_t* pArgs)
{
  switch((pArgs)->Event)
  {
      case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
      case SL_NETAPP_IPV6_IPACQUIRED_EVENT:
          SetCC3200MachineState(CC3200_IP_ALLOC);
          g_SmartplugLedCurrentState = CONNECTED_TO_AP_IND;
          Report("IP Allocated \n\r");
          break;
      default:
          break;
  }
}

//*****************************************************************************
//!  \brief  Reset state from the state machine
//!  \param  None
//!  \return none
//*****************************************************************************
void
ResetCC3200StateMachine()
{
  g_cc3200state = CC3200_UNINIT;
}

unsigned char GetCC3200State()
{
  return g_cc3200state;
}

//*****************************************************************************
//! De-Initialize the uDMA controller
//! \param None
//! \return None.
//*****************************************************************************
void UDMADeInit()
{
  // UnRegister interrupt handlers
  
  //MAP_uDMAIntUnregister(UDMA_INT_SW);
  #ifdef SL_PLATFORM_MULTI_THREADED
    osi_InterruptDeRegister(UDMA_INT_ERR);
  #else
    MAP_uDMAIntUnregister(UDMA_INT_ERR);
  #endif

  // Disable the uDMA
  MAP_uDMADisable();
}

//*****************************************************************************
//! DMA error interrupt handler. This function invokes when DMA operation is in 
//! error
//! \param None
//! \return None.
//*****************************************************************************
void DmaErrIntHandler(void)
{
  HWREG(0x4402609c) = (3<<10);
  MAP_uDMAIntClear(MAP_uDMAIntStatus());

  g_SmartplugLedCurrentState = DEVICE_ERROR_IND;

  //Should not reach here
  while(1)
  {
  }
}

//*****************************************************************************
//! Initialize the DMA controller. This function initializes the McASP module
//! \param None
//! \return None.
//*****************************************************************************
void UDMAInit()
{
  //unsigned int uiLoopCnt;
  //
  // Enable McASP at the PRCM module
  //
  PRCMPeripheralClkEnable(PRCM_UDMA,PRCM_RUN_MODE_CLK|PRCM_SLP_MODE_CLK);
  PRCMPeripheralReset(PRCM_UDMA);

  //
  // Register interrupt handlers
  //
  #ifdef SL_PLATFORM_MULTI_THREADED
    osi_InterruptRegister(UDMA_INT_ERR, (P_OSI_INTR_ENTRY)DmaErrIntHandler, INT_PRIORITY_LVL_7);
  #else
    IntPendClear(UDMA_INT_ERR);

    //
    // Set the priority
    //
    IntPrioritySet(UDMA_INT_ERR, INT_PRIORITY_LVL_7);

    MAP_uDMAIntRegister(UDMA_INT_ERR, DmaErrIntHandler);
  #endif

  //
  // Enable uDMA using master enable
  //
  MAP_uDMAEnable();

  //
  // Set Control Table
  //
  memset(gpCtlTbl,0,sizeof(tDMAControlTable)*CTL_TBL_SIZE);
  MAP_uDMAControlBaseSet(gpCtlTbl);
}

//*****************************************************************************
//! initDriver
//! The function initializes a CC3200 device and triggers it to start operation
//! \param  None
//! \return none
//*****************************************************************************
void InitSimpleLink(void)
{
  long lMode = -1;

  //
  // Start the SimpleLink Host
  //
  VStartSimpleLinkSpawnTask(SIMPLINK_SPAWN_TASK_PRIO);

  //
  // Start the simplelink host
  //
  /* Clear WDT */
  ClearWDT();
  lMode = sl_Start(NULL,NULL,NULL);
  if(ROLE_STA != lMode)
  {
    Report("Smartplug Wrongly in AP Mode \n\r");

    /* Clear WDT */
    ClearWDT();

    if(ROLE_AP == lMode)
    {
      /* Clear WDT */
      ClearWDT();
      while(!(g_cc3200state & CC3200_IP_ALLOC))
      {
        ClearWDT();
      }
    }

    Report("Enter back Station Mode \n\r");

    /* Enter back to STA Mode */
    sl_WlanSetMode(ROLE_STA);

    /* Restart Network processor */
    sl_Stop(SL_STOP_TIMEOUT);

    /* Clear WDT */
    ClearWDT();
    OSI_DELAY(200);

    if(ROLE_STA != sl_Start(NULL,NULL,NULL))
    {
      Report("STA Mode enter failed : FATAL\n\r");
      while(1);
    }
    else
    {
      SetCC3200MachineState(CC3200_INIT);
    }
  }
  else
  {
    SetCC3200MachineState(CC3200_INIT);
  }
  /* Clear WDT */
  ClearWDT();

  /* Enable auto connect (connection to stored profiles according to priority) */
  sl_WlanPolicySet(SL_POLICY_CONNECTION , SL_CONNECTION_POLICY(1, 0, 0, 0, 0), 0, 0);

  //while(!(g_cc3200state & CC3200_INIT)); //It should happen within 9 secs

  //sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID); //Disable HTTP server DKS

  DBG_PRINT("simplelink started\n\r");
}

//*****************************************************************************
//! DeInitSimpleLink
//! The function de-initializes a CC3200 device
//! \param  None
//! \return none
//*****************************************************************************
void DeInitSimpleLink(void)
{
  // Delete Spawn task
  VDeleteSimpleLinkSpawnTask();

  // Clear WDT
  ClearWDT();

  // Stop the simplelink host
  sl_Stop(0);

  // Clear WDT
  ClearWDT();

  // Reset the state to uninitialized
  g_cc3200state = CC3200_UNINIT;

  DBG_PRINT("simplelink Closed\n\r");
}

ButtonState IsButtonPressed(void)
{
  int counter = 0;
  UINT32 SmartconfButtonSense;
  UINT32 CriticalMask;

  while(counter <= 4)
  {
    // Disable interrupt https://e2e.ti.com/support/wireless_connectivity/simplelink_wifi_cc31xx_cc32xx/f/968/t/438726
    CriticalMask = osi_EnterCritical();
    SmartconfButtonSense = g_SmartconfButtonSense;

    // Enable interrupt https://e2e.ti.com/support/wireless_connectivity/simplelink_wifi_cc31xx_cc32xx/f/968/t/438726
    osi_ExitCritical(CriticalMask);

    if(SmartconfButtonSense)
    {
      counter = 0;
    }
    else
    {
      if(IsSmartConfButtonPressed())
      {
        return SMART_CONF_BUTTON;
      }
      else if(IsAPProvButtonPressed())
      {
        return APPROV_BUTTON;
      }
      else if(IsWpsButtonPressed())
      {
        return WPS_BUTTON;
      }
    }

    OSI_DELAY(1000);
    /* Clear WDT */
    ClearWDT();
    counter++;
  }
  return NO_BUTTON;
}

int WaitToConnect()
{
  int wlanDiscon = 0, counter = 0;
  ButtonState ButtonPressed;

  ClearWDT();
  /* Check if smartconfig/WPS button is pressed */
  Report("Waiting for smartconfig/APProv/WPS \n\r");
  ButtonPressed = IsButtonPressed();

  if(SMART_CONF_BUTTON == ButtonPressed)
  {
    /* Perform smartconfig */
    StartSmartConfig();
  }
  else if(APPROV_BUTTON == ButtonPressed)
  {
    /* Perform AP Provisioning */
    StartApProvConfig();
  }
  else if(WPS_BUTTON == ButtonPressed)
  {
    /* Perform WPS */
    StartWpsConfig();
  }

  Report("Waiting to connect to AP \n\r");
  while((!(g_cc3200state & CC3200_ASSOC)) && (counter < 200)) //wait max 2sec for association
  {
    OSI_DELAY(10);
    /* Clear WDT */
    ClearWDT();
    counter++;
  }

  while(!(g_cc3200state & CC3200_IP_ALLOC))
  {
    if(!(g_cc3200state & CC3200_ASSOC))
    {
      wlanDiscon = 1;
      break;
    }

    OSI_DELAY(100);
    /* Clear WDT */
    ClearWDT();
  }

  if(0 == wlanDiscon)
  {
    return 0;
  }
  else
  {
    return -1;
  }
}

int WlanDisconnect(void)
{
  //
  // Disconnect from the AP
  //
  sl_WlanDisconnect();
  /* Clear WDT */
  ClearWDT();

  while(g_cc3200state & CC3200_ASSOC);//It should happen within 9 secs

  return 1;
}

//*****************************************************************************
//! ConnectToAP  Connect to an Access Point using SmartConfig
//! \param  none
//! \return none
//*****************************************************************************
void ConnectToAP(void)
{
	unsigned long ulSubNetMask,ulDefaultGateway,ulDNSServer;

	g_SmartplugLedCurrentState = WLAN_CONNECT_OPER_IND;

	do
	{
		if(WaitToConnect() >= 0)
		{
			ClearWDT();
			// Get the IP address of smartplug
			IpConfigGet(&g_SmartPlugIpAdd,&ulSubNetMask,&ulDefaultGateway,&ulDNSServer);

			if(g_SmartPlugIpAdd != 0)
			{
				int status;
				/* Set Good TCP if valid IP present */
				SetCC3200MachineState(CC3200_GOOD_TCP);
				/* Perform mDNS device info broadcast */
				status = mDNSBroadcast(1);
				/* Notify Error handler */
				if(status != 0)
				{
					g_SmartplugErrorRecovery.MdnsError = MAX_MDNS_ERROR;
				}
				else
				{
					g_SmartplugErrorRecovery.MdnsError = 0;
				}
				break;
			}
			else
			{
				// Disconnect from the AP
				WlanDisconnect();
			}
		}
	} while(1);
}

void TurnOffDevice()
{
	MAP_GPIODirModeSet(SmartPlugGpioConfig.RelayCtrlPort, SmartPlugGpioConfig.RelayCtrlPad, GPIO_DIR_MODE_OUT);//output//DKS
	GPIO_IF_Set(SmartPlugGpioConfig.RelayCtrlPort, SmartPlugGpioConfig.RelayCtrlPad, 0);//Disable relay by negative pulse
	DELAY(1);//1ms
	Report("Turn OFF device \n\r");
	MAP_GPIODirModeSet(SmartPlugGpioConfig.RelayCtrlPort, SmartPlugGpioConfig.RelayCtrlPad, GPIO_DIR_MODE_IN);//tri state
}

void TurnOnDevice()
{
	MAP_GPIODirModeSet(SmartPlugGpioConfig.RelayCtrlPort, SmartPlugGpioConfig.RelayCtrlPad, GPIO_DIR_MODE_OUT);//output
	GPIO_IF_Set(SmartPlugGpioConfig.RelayCtrlPort, SmartPlugGpioConfig.RelayCtrlPad, 1);//Enable relay by positive pulse
	DELAY(1);//1ms//DKS
	Report("Turn ON device \n\r");
	MAP_GPIODirModeSet(SmartPlugGpioConfig.RelayCtrlPort, SmartPlugGpioConfig.RelayCtrlPad, GPIO_DIR_MODE_IN);//tri state
}

int mDNSBroadcast(unsigned char RegOrUnReg)
{
    char mDNSServName[64], SmartPlugDevName[32];
    unsigned char Dev_len, Serv_len;
    int status = -1, retry_count = 0;

    Serv_len = strlen(SMARTPLUG_TI_NAME);
    memcpy((void *)&mDNSServName[0],SMARTPLUG_TI_NAME,Serv_len);
    mDNSServName[Serv_len] = 0; //end with \0

    if(SmartPlugNvmmFile.DeviceConfigData.DevConfig.DevNameLen > 0)
    {
      Dev_len = SmartPlugNvmmFile.DeviceConfigData.DevConfig.DevNameLen;
      memcpy((void *)&SmartPlugDevName[0],(void *)&SmartPlugNvmmFile.DeviceConfigData.DevConfig.DevName[0],Dev_len);
    }
    else
    {
      Dev_len = strlen(SMARTPLUG_DEVICE_NAME);
      memcpy((void *)&SmartPlugDevName[0],SMARTPLUG_DEVICE_NAME,Dev_len);
    }
    SmartPlugDevName[Dev_len] = 0;//end with \0

    strcat((void *)&mDNSServName[0], SmartPlugDevName);
    strcat((void *)&mDNSServName[0], SMARTPLUG_mDNS_SERV);
    Serv_len = strlen(mDNSServName);

    /* Register/unregister can be performed only after AP connect */
    if(1 == RegOrUnReg)
    {
		//sl_NetAppMDNSUnRegisterService("cc3200smartplug1._device-info._tcp.local",(unsigned char)strlen("cc3200smartplug1._device-info._tcp.local"));
		//sl_NetAppMDNSUnRegisterService("cc3200smartplug._ipp._tcp.local",(unsigned char)strlen("cc3200smartplug._ipp._tcp.local"));
		//sl_NetAppMDNSUnRegisterService("cc3200smartplug._uart._tcp.local",(unsigned char)strlen("cc3200smartplug._uart._tcp.local"));
		//sl_NetAppMDNSUnRegisterService("cc3200smartplug._http._tcp.local",(unsigned char)strlen("cc3200smartplug._http._tcp.local"));

		if(mDNSService.mDNSServNameUnRegLen)
		{
			while ((status != 0) && (retry_count < 2))
			{
				ClearWDT();
				status = sl_NetAppMDNSUnRegisterService((signed char const*)&mDNSService.mDNSServNameUnReg[0],\
                          (unsigned char)mDNSService.mDNSServNameUnRegLen);
				if(status == 0)
				{
					Report("mDNS UnReg Done\n\r");
				}
				else
				{
					retry_count++;
					Report("mDNS UnReg Fail %d\n\r", status);
				}
			}
			mDNSService.mDNSServNameUnRegLen = 0;
		}
		//Registering for the mDNS service.
		retry_count = 0;
		status = -1;
		while ((status != 0) && (retry_count < 4))
		{
			ClearWDT();
			sl_NetAppMDNSUnRegisterService((signed char *)&mDNSServName[0],(unsigned char)Serv_len);
			status = sl_NetAppMDNSRegisterService((signed char *)&mDNSServName[0],(unsigned char)Serv_len,\
				(signed char *)&SmartPlugDevName[0],(unsigned char)Dev_len,APP_TCP_PORT,TTL_MDNS_SERV,1);

			if(status == 0)
			{
				Report("mDNS Broadcast Done\n\r");
			}
			else
			{
				retry_count++;
				Report("mDNS Broadcast Fail %d\n\r", status);
			}
		}
    }
    else
    {
		mDNSService.mDNSServNameUnRegLen = Serv_len;
		memcpy((void *)&mDNSService.mDNSServNameUnReg[0],&mDNSServName[0],Serv_len);
    }

    return status;
}

//*****************************************************************************
//! Periodic Timer Interrupt Handler for Broadcast
//! \param None
//! \return None
//*****************************************************************************
void BroadcastTimerIntHandler(void)
{
	UINT32 temp = 0, ulInts;
	// Clear all pending interrupts from the timer we are
	// currently using.
	ulInts = MAP_TimerIntStatus(TIMERA0_BASE, true);
	MAP_TimerIntClear(TIMERA0_BASE, ulInts);

	g_MsCounter++;//100ms counter
	if(0 == (g_MsCounter%SMARTPLUG_1SEC_TASK_MOD_VAL))
	{
		//Inc counter every 1sec
		g_ulDeviceTimerInSec++;
		temp = g_ulDeviceTimerInSec;
	}

	ManageSwitches();

	if((temp != 0) || (1 == g_SmartPlugRelayState))
	{
		/* Trigger 1sec timer task */
		osi_MsgQWrite(&TimeHandlerTaskMsgQ,&temp,OSI_NO_WAIT);
	}

	ManageLEDIndication();
}

void TimeStampTimerIntHandler(void)
{
	unsigned long ulInts;

	// Clear all pending interrupts from the timer we are currently using.
	ulInts = MAP_TimerIntStatus(TIMERA3_BASE, true);
	MAP_TimerIntClear(TIMERA3_BASE, ulInts);

	// Increment the timestamp counter
	TimerMsCount++;
}

//*****************************************************************************
//! The interrupt handler for the watchdog timer
//! \param  None
//! \return None
//*****************************************************************************
void WatchdogIntHandler(void)
{
	// If we have been told to stop feeding the watchdog, return immediately
	// without clearing the interrupt.  This will cause the system to reset
	// next time the watchdog interrupt fires.
	if(!g_ClearWatchdog)
	{
		g_SmartplugLedCurrentState  = DEVICE_ERROR_IND;
		DELAY(750);//wait 750ms
		HIBReset();
		return;
	}

	// Clear the watchdog interrupt.
	MAP_WatchdogIntClear(WDT_BASE);
	g_ClearWatchdog = 0;
	//Report("WDT cleared \n\r");
}

//*****************************************************************************
//! Clear watchdog timer
//! \param  None
//! \return None
//*****************************************************************************
void ClearWDT(void)
{
	g_ClearWatchdog = 1;
}

//****************************************************************************
//! Initialize the watchdog timer
//! \param none
//! \return None.
//****************************************************************************
void WDTInit(void)
{
	UINT8 Retcode;

	// Enabling WDT0 peripheral clock
	// HWREG(SYSTEM_CONTROL_BASE + 0x0000600) = 0x0000001;
//	volatile unsigned long* ptr = (unsigned long *)(SYSTEM_CONTROL_BASE + 0x0000600);
//	(*ptr) = (*ptr) | 0x00000001;
//	Report((unsigned char) *ptr);

	// Enable the peripherals used by this example.
    //
    //MAP_PRCMPeripheralClkEnable(PRCM_WDT, PRCM_RUN_MODE_CLK);

	// Unlock to be able to configure the registers
	MAP_WatchdogUnlock(WDT_BASE);
	//MAP_WatchdogUnlock(0x50000000);

	// Register the interrupt handler with lowest priority
#ifdef SL_PLATFORM_MULTI_THREADED
    osi_InterruptRegister(INT_WDT, (P_OSI_INTR_ENTRY)WatchdogIntHandler, INT_PRIORITY_LVL_7);
#else
    IntRegister(INT_WDT, WatchdogIntHandler);
    // Set the priority
    IntPrioritySet(INT_WDT, INT_PRIORITY_LVL_7);
    IntPendClear(INT_WDT);
    IntEnable(INT_WDT);
#endif

	// Enable the interrupt
	MAP_WatchdogEnable(WDT_BASE);

	// Enable stalling of the watchdog timer during debug events
	MAP_WatchdogStallEnable(WDT_BASE);

	// Set the watchdog timer reload value
	MAP_WatchdogReloadSet(WDT_BASE,MILLISECONDS_TO_TICKS(WDT_PERIOD_IN_MS));

	// Start the timer. Once the timer is started, it cannot be disable.
	MAP_WatchdogEnable(WDT_BASE);

	Retcode = MAP_WatchdogRunning(WDT_BASE);
	if(!Retcode)
	{
		Report("Error in WDT Enable \n\r");
	}
	else
	{
		Report("WDT Initialized \n\r");
	}
}

//****************************************************************************
//! Enter the HIBernate mode configuring the wakeup timer - HIBReset
//! This function
//!    1. Sets up the wakeup RTC timer
//!    2. Enables the RTC
//!    3. Enters into HIBernate
//! \param none
//! \return None.
//****************************************************************************
void HIBReset(void)
{
	// Configure the HIB module RTC wake time
	MAP_PRCMHibernateIntervalSet((32768>>2)); //250ms wakeup

	// Enable the HIB RTC
	MAP_PRCMHibernateWakeupSourceEnable(PRCM_HIB_SLOW_CLK_CTR);

	DELAY(10);

	// Enter HIBernate mode
	MAP_PRCMHibernateEnter();

	//wait in infinet loop
	while(1);
}

//*****************************************************************************
//! Periodic Timer Initialization, configuration and Registering Interrupt
//! handler
//! \param  None
//! \return None
//*****************************************************************************
void TimerInit()
{
	// Timer for GPIO sense, LED manage & smartplug data broadcast
	//MAP_PRCMPeripheralClkEnable(PRCM_TIMERA0, PRCM_RUN_MODE_CLK|PRCM_SLP_MODE_CLK); // Solves 2
    HWREG(0x44025090) |= 0x00000101;

	PRCMPeripheralReset(PRCM_TIMERA0);
	MAP_TimerConfigure(TIMERA0_BASE, TIMER_CFG_PERIODIC);
	MAP_TimerLoadSet(TIMERA0_BASE, TIMER_A, PERIODIC_BROADCAST_CYCLES);

#ifdef SL_PLATFORM_MULTI_THREADED
    osi_InterruptRegister(INT_TIMERA0A, (P_OSI_INTR_ENTRY)BroadcastTimerIntHandler, INT_PRIORITY_LVL_4);
#else

    // Set the priority
    IntPrioritySet(INT_TIMERA0A, INT_PRIORITY_LVL_4);
    IntPendClear(INT_TIMERA0A);
    MAP_TimerIntRegister(TIMERA0_BASE, TIMER_A,BroadcastTimerIntHandler);
#endif

	MAP_TimerIntEnable(TIMERA0_BASE,  TIMER_TIMA_TIMEOUT );

#ifdef TIME_STAMP_EN
	//Timer for mips calculation - time stamp
	MAP_PRCMPeripheralClkEnable(PRCM_TIMERA3, PRCM_RUN_MODE_CLK|PRCM_SLP_MODE_CLK);
	PRCMPeripheralReset(PRCM_TIMERA3);
	MAP_TimerConfigure(TIMERA3_BASE, TIMER_CFG_PERIODIC);
	MAP_TimerLoadSet(TIMERA3_BASE, TIMER_A, PERIODIC_TIME_STAMP_CYCLES);
#ifdef SL_PLATFORM_MULTI_THREADED
    osi_InterruptRegister(INT_TIMERA3A, (P_OSI_INTR_ENTRY)TimeStampTimerIntHandler, INT_PRIORITY_LVL_0);
#else
	
    // Set the priority
    IntPrioritySet(INT_TIMERA3A, INT_PRIORITY_LVL_0);
    IntPendClear(INT_TIMERA3A);
    MAP_TimerIntRegister(TIMERA3_BASE, TIMER_A, TimeStampTimerIntHandler);
#endif

	MAP_TimerIntEnable(TIMERA3_BASE,  TIMER_TIMA_TIMEOUT );
#endif
}

//*****************************************************************************
//! Wrapper for broadcast timer enable
//! \param  None
//! \return None
//*****************************************************************************
void EnableBroadcastTimer()
{
	MAP_TimerEnable(TIMERA0_BASE, TIMER_A);
	MAP_TimerIntEnable(TIMERA0_BASE,  TIMER_TIMA_TIMEOUT );
}

//*****************************************************************************
//
//! wrapper for broadcast timer disable
//! \param  None
//! \return None
//
//*****************************************************************************
void DisableBroadcastTimer()
{
	MAP_TimerDisable(TIMERA0_BASE, TIMER_A);
	MAP_TimerIntDisable(TIMERA0_BASE, TIMER_TIMA_TIMEOUT );
}

#ifdef TIME_STAMP_EN
//*****************************************************************************
//! Wrapper for timer enable
//! \param  None
//! \return None
//*****************************************************************************
void EnableTimeStampTimer()
{
	MAP_TimerEnable(TIMERA3_BASE, TIMER_A);
	MAP_TimerIntEnable(TIMERA3_BASE,  TIMER_TIMA_TIMEOUT );
	TimerMsCount =0;
}

//*****************************************************************************
//! wrapper for timer disable
//! \param  None
//! \return None
//*****************************************************************************
void DisableTimeStampTimer()
{
	MAP_TimerDisable(TIMERA3_BASE, TIMER_A);
	MAP_TimerIntDisable(TIMERA3_BASE, TIMER_TIMA_TIMEOUT );
}
#endif

//*****************************************************************************
//! GPIO Enable & Configuration
//! \param  None
//! \return None
//*****************************************************************************
void LedConfiguration( void )
{
	// Initial LED configuration
	g_SmartplugLedCurrentState = DEVICE_POW_ON_IND;
	GPIO_IF_LedOn(RED_LED);
}

char IsSmartConfButtonPressed()
{
	char SmartPlugSmartconf;
	UINT32 CriticalMask;

	CriticalMask = osi_EnterCritical();
	SmartPlugSmartconf = g_SmartPlugSmartconf;
	g_SmartPlugSmartconf = 0;
	osi_ExitCritical(CriticalMask);

	return SmartPlugSmartconf;
}

char IsAPProvButtonPressed()
{
	char SmartPlugApProv;
	UINT32 CriticalMask;

	CriticalMask = osi_EnterCritical();
	SmartPlugApProv = g_SmartPlugAPProv;
	g_SmartPlugAPProv = 0;
	osi_ExitCritical(CriticalMask);

	return SmartPlugApProv;
}

char IsWpsButtonPressed()
{
	char SmartPlugWps;
	UINT32 CriticalMask;

	CriticalMask = osi_EnterCritical();
	SmartPlugWps = g_SmartPlugWps;
	g_SmartPlugWps = 0;
	osi_ExitCritical(CriticalMask);

	return SmartPlugWps;
}

char GetRelayState()
{
	char SmartPlugRelayState;
	UINT32 CriticalMask;

	CriticalMask = osi_EnterCritical();
	SmartPlugRelayState = g_SmartPlugRelayState;
	g_SmartPlugRelayState = 0;
	osi_ExitCritical(CriticalMask);

	return SmartPlugRelayState;
}

void ManageSwitches(void)
{
	unsigned long ulPinState, PrevSmartconfButtonSense;;

	unsigned long PrevRelayStateButtonSense;

	PrevRelayStateButtonSense = g_RelayStateButtonSense;

	/* Read Relay control pin */
	ulPinState = GPIO_IF_Get(SmartPlugGpioConfig.RelayStatePort, SmartPlugGpioConfig.RelayStatePad);

#ifdef SMARTPLUG_HW //smartplug has reverse logic - (!ulPinState)
	if(!ulPinState)
#else
	if(ulPinState)
#endif
	{
		g_RelayStateButtonSense++;
	}
	else
	{
		g_RelayStateButtonSense = 0;
	}

	if(PrevRelayStateButtonSense > g_RelayStateButtonSense)
	{
		if(PrevRelayStateButtonSense >= 1)//100ms
		{
			/* Toggle Relay */
			g_SmartPlugRelayState = 1;
		}
	}

	/* Read Smart config pin */
	PrevSmartconfButtonSense = g_SmartconfButtonSense;

	ulPinState = GPIO_IF_Get(SmartPlugGpioConfig.SmartConfgPort, SmartPlugGpioConfig.SmartConfgPad);

#ifdef SMARTPLUG_HW //smartplug has reverse logic - (!ulPinState)
	if(!ulPinState)
#else
	if(ulPinState)
#endif
	{
		g_SmartconfButtonSense++;
		//Report("smartconfig button pressed \n");//waqar
	}
	else
	{
		g_SmartconfButtonSense = 0;
		//Report("smartconfig button not pressed \n");//waqar
	}

	/* Perform Smartconfig/WPS sense only in boot */
	if(PrevSmartconfButtonSense > g_SmartconfButtonSense)
	{
	    //Report("Time is %d  \n", PrevSmartconfButtonSense);//waqar
	    //g_SmartPlugSmartconf = 1; // NEW ADDED WAQAR
// COMMENTED OUT TO DISABLE OTHER INTERFACES
		if(PrevSmartconfButtonSense >= 70)//7sec
		{
			/* WPS Initiated */
			g_SmartPlugWps = 1;
		}
		else if(PrevSmartconfButtonSense >= 40)//4sec
		{
			/* AP provisioning Initiated */
			g_SmartPlugAPProv = 1;
		}
		else if(PrevSmartconfButtonSense >= 10)//1sec
		{
			/* Smartconfig Initiated */
			g_SmartPlugSmartconf = 1;
		}
	}
}

void ManageLEDIndication( void )
{
  static LedColors LedColorSt = NO_LED;

  switch(g_SmartplugLedCurrentState)
  {
    case NO_LED_IND:
      if(NO_LED_IND != g_SmartplugLedPreviousState)
      {
        GPIO_IF_LedOn(NO_LED);
        g_SmartplugLedPreviousState = g_SmartplugLedCurrentState;
        LedColorSt = NO_LED;
      }
      break;

    case DEVICE_POW_ON_IND:
      if(DEVICE_POW_ON_IND != g_SmartplugLedPreviousState)
      {
        GPIO_IF_LedOn(RED_LED);
        g_SmartplugLedPreviousState = g_SmartplugLedCurrentState;
        LedColorSt = RED_LED;
      }
      else if(0 == (g_MsCounter%DEVICE_POW_ON_TOGGLE_TIME))
      {
        if(RED_LED == LedColorSt)
        {
          GPIO_IF_LedOn(NO_LED);
          LedColorSt = NO_LED;
        }
        else
        {
          GPIO_IF_LedOn(RED_LED);
          LedColorSt = RED_LED;
        }
      }
      break;

    case DEVICE_ERROR_IND:
      if(DEVICE_ERROR_IND != g_SmartplugLedPreviousState)
      {
        GPIO_IF_LedOn(RED_LED);
        g_SmartplugLedPreviousState = g_SmartplugLedCurrentState;
        LedColorSt = RED_LED;
      }
      else if(0 == (g_MsCounter%DEVICE_ERROR_TOGGLE_TIME))
      {
        if(RED_LED == LedColorSt)
        {
          GPIO_IF_LedOn(NO_LED);
          LedColorSt = NO_LED;
        }
        else
        {
          GPIO_IF_LedOn(RED_LED);
          LedColorSt = RED_LED;
        }
      }
      break;

    case SMART_CONFIG_OPER_IND:
      if(SMART_CONFIG_OPER_IND != g_SmartplugLedPreviousState)
      {
        GPIO_IF_LedOn(BLUE_LED);
        g_SmartplugLedPreviousState = g_SmartplugLedCurrentState;
        LedColorSt = BLUE_LED;
      }
      else if(0 == (g_MsCounter%SMART_CONFIG_OPER_TOGGLE_TIME))
      {
        if(BLUE_LED == LedColorSt)
        {
          GPIO_IF_LedOn(NO_LED);
          LedColorSt = NO_LED;
        }
        else
        {
          GPIO_IF_LedOn(BLUE_LED);
          LedColorSt = BLUE_LED;
        }
      }
      break;

    case WLAN_CONNECT_OPER_IND:
      if(WLAN_CONNECT_OPER_IND != g_SmartplugLedPreviousState)
      {
        GPIO_IF_LedOn(BLUE_LED);
        g_SmartplugLedPreviousState = g_SmartplugLedCurrentState;
        LedColorSt = BLUE_LED;
      }
      else if(0 == (g_MsCounter%WLAN_CONNECT_OPER_TOGGLE_TIME))
      {
        if(BLUE_LED == LedColorSt)
        {
          GPIO_IF_LedOn(NO_LED);
          LedColorSt = NO_LED;
        }
        else
        {
          GPIO_IF_LedOn(BLUE_LED);
          LedColorSt = BLUE_LED;
        }
      }
      break;

    case CONNECTED_TO_AP_IND:
      if(CONNECTED_TO_AP_IND != g_SmartplugLedPreviousState)
      {
        GPIO_IF_LedOn(BLUE_LED);
        g_SmartplugLedPreviousState = g_SmartplugLedCurrentState;
        LedColorSt = BLUE_LED;
      }
      break;

    case CONNECTED_ANDROID_IND:
      if(CONNECTED_ANDROID_IND != g_SmartplugLedPreviousState)
      {
        GPIO_IF_LedOn(MAGENTA_LED);
        g_SmartplugLedPreviousState = g_SmartplugLedCurrentState;
        LedColorSt = MAGENTA_LED;
      }
      break;

    case ANDROID_DATA_TRANSFER_IND:
      if(ANDROID_DATA_TRANSFER_IND != g_SmartplugLedPreviousState)
      {
        GPIO_IF_LedOn(MAGENTA_LED);
        g_SmartplugLedPreviousState = g_SmartplugLedCurrentState;
        LedColorSt = MAGENTA_LED;
      }
      else if(0 == (g_MsCounter%ANDROID_DATA_TRANSFER_TOGGLE_TIME))
      {
        if(MAGENTA_LED == LedColorSt)
        {
          GPIO_IF_LedOn(NO_LED);
          LedColorSt = NO_LED;
        }
        else
        {
          GPIO_IF_LedOn(MAGENTA_LED);
          LedColorSt = MAGENTA_LED;
        }
      }
      break;

    case CONNECTED_CLOUD_IND:
      if(CONNECTED_CLOUD_IND != g_SmartplugLedPreviousState)
      {
        GPIO_IF_LedOn(GREEN_LED);
        g_SmartplugLedPreviousState = g_SmartplugLedCurrentState;
        LedColorSt = GREEN_LED;
      }
      break;

    case CLOUD_DATA_TRANSFER_IND:
      if(CLOUD_DATA_TRANSFER_IND != g_SmartplugLedPreviousState)
      {
        GPIO_IF_LedOn(GREEN_LED);
        g_SmartplugLedPreviousState = g_SmartplugLedCurrentState;
        LedColorSt = GREEN_LED;
      }
      else if(0 == (g_MsCounter%CLOUD_DATA_TRANSFER_TOGGLE_TIME))
      {
        if(GREEN_LED == LedColorSt)
        {
          GPIO_IF_LedOn(NO_LED);
          LedColorSt = NO_LED;
        }
        else
        {
          GPIO_IF_LedOn(GREEN_LED);
          LedColorSt = GREEN_LED;
        }
      }
      break;
  }
}

void ToggleLEDIndication( void )
{
  /* Switch OFF all LED it will be turned ON after 100ms */
  g_SmartplugLedPreviousState = NO_LED_IND;
  GPIO_IF_LedOn(NO_LED);
}

void AndroidDataTranLEDInd( void )
{
  switch(g_SmartplugLedCurrentState)
  {
    case CONNECTED_ANDROID_IND:
      ToggleLEDIndication();
      break;

    case CONNECTED_CLOUD_IND:
      g_SmartplugLedPreviousState = ANDROID_DATA_TRANSFER_IND;
      GPIO_IF_LedOn(MAGENTA_LED);
      break;
  }
}

void ExositeCIKFailLEDInd( void )
{
  g_SmartplugLedPreviousState = DEVICE_ERROR_IND;
  GPIO_IF_LedOn(RED_LED);
}

//*****************************************************************************
//
//! Enables software interrupt handler for specified channel
//!
//! \param ulSftIntNo is one of the valid SFT interrupt
//! \param ucPriority is priority of channel
//! \param pfnHandler is a pointer to the function to be called when the
//! SFT channel interrupt occurs.
//!
//! This function enables SFT interrupt handler for specified
//! channel. It is the interrupt handler's responsibility to clear
//! the interrupt source.
//!
//!
//! \return None.
//
//*****************************************************************************
void SoftwareIntRegister(unsigned long ulSftIntNo, unsigned char ucPriority, void (*pfnHandler)(void))
{
  //
  // Register the interrupt handler
  //
  IntRegister(ulSftIntNo,pfnHandler);

  //
  // Set priority
  //
  IntPrioritySet(ulSftIntNo, ucPriority);

  IntPendClear(ulSftIntNo);

  //
  // Enable ADC interrupt
  //
  IntEnable(ulSftIntNo);
}

void SoftwareIntTrigger(unsigned long ulSftIntNo)
{
  IntPendSet(ulSftIntNo);
}

#if 0 //DKS test code
//*****************************************************************************
//
//! This function demonstrates how certificate can be used with SSL.
//! The procedure includes the following steps:
//! 1) connect to an open AP
//! 2) get the server name via a DNS request
//! 3) define all socket options and point to the CA certificate
//! 4) connect to the server via TCP
//!
//! \param None
//!
//! \return  LED1 is turned solid in case of success
//!    LED2 is turned solid in case of failure
//!
//*****************************************************************************
#define SERVER_NAME "m2.exosite.com"
#define DST_PORT    443
#define SL_SSL_CA_CERT    "/cert/exositeca.der" /* CA certificate file ID */

int    iRetVal,g_SockID;
char    *g_Host = SERVER_NAME;

int
ssl(void *pvParameters)
{
  SlSockAddrIn_t    Addr;
  //_SlFd_t *pIfHdl = NULL;
  int    iAddrSize;
  unsigned int uiIP;

  iRetVal=sl_NetAppDnsGetHostByName(g_Host, strlen(g_Host),
                                    (unsigned long*)&uiIP, SL_AF_INET);
  if( iRetVal < 0 )
  {
      return -1;
  }
  //uiIP = 0xADFFD11C;
  Addr.sin_family = SL_AF_INET;
  Addr.sin_port = sl_Htons(DST_PORT);
  Addr.sin_addr.s_addr = sl_Htonl(uiIP);
  iAddrSize = sizeof(Addr);
  //
  // opens a secure socket
  //
  g_SockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, SL_SEC_SOCKET);
  if( g_SockID < 0 )
  {
      sl_Close(g_SockID);
      return -1;
  }

  iRetVal = sl_SetSockOpt(g_SockID, SL_SOL_SOCKET, SL_SO_SECURE_FILES_CA_FILE_NAME,SL_SSL_CA_CERT, strlen(SL_SSL_CA_CERT));
  if( iRetVal < 0 )
  {
      sl_Close(g_SockID);
      return -1;
  }

  /* connect to the peer device - exosite server */
  iRetVal = sl_Connect(g_SockID, ( SlSockAddr_t *)&Addr, iAddrSize);

   if (iRetVal < 0 )
   {
       Report(" SSL error %d\n\r", iRetVal);
       sl_Close(g_SockID);

       return -1;
   }
   Report(" SSL connect success \n\r");

   sl_Close(g_SockID);
   return 1;
}

#endif

//*****************************************************************************
// File: smartconfig.c
//
// Description: SmartConfig function of Smartplug gen-1 application
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
#include "protocol.h"
#include "cc3200.h"
#include "gpio_if.h"
#include "utils.h"
#include "smartconfig.h"
#include "exosite_task.h"
#include "exosite_meta.h"
#include "exosite_hal.h"
#include "metrology.h"
#include "smartplug_task.h"
#include "uart_logger.h"
#include "gpio_if.h"

Sl_WlanNetworkEntry_t g_NetEntries[SCAN_TABLE_SIZE];
char g_token_get[TOKEN_ARRAY_SIZE][STRING_TOKEN_SIZE] = {"__SL_G_US0",
                                        "__SL_G_US1", "__SL_G_US2","__SL_G_US3",
                                                    "__SL_G_US4", "__SL_G_US5"};
SlSecParams_t g_SecParams;
unsigned char g_ucPriority = 0;

volatile unsigned char g_smartConfigDone = 0,
                       g_ConnectedToConfAP = 0;
extern volatile unsigned char g_cc3200state;
extern volatile UINT8  g_StationConnToAP;
extern t_SmartPlugNvmmFile SmartPlugNvmmFile;
extern volatile LedIndicationName  g_SmartplugLedCurrentState;
extern OsiLockObj_t        g_NvmemLockObj;

//*****************************************************************************
//
//! This function gets triggered when HTTP Server receives Application
//! defined GET and POST HTTP Tokens.
//!
//! \param pSlHttpServerEvent - Pointer indicating http server event
//! \param pSlHttpServerResponse - Pointer indicating http server response
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pSlHttpServerEvent,
        SlHttpServerResponse_t *pSlHttpServerResponse)
{
  signed char WlanSecurityKey[50];

    switch (pSlHttpServerEvent->Event)
    {
        case SL_NETAPP_HTTPGETTOKENVALUE_EVENT:
        {

            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                            g_token_get[0], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                if(g_ConnectedToConfAP == 1)
                {
                    // Important - Connection Status
                    memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                            "TRUE",strlen("TRUE"));
                    pSlHttpServerResponse->ResponseData.token_value.len = \
                                                                strlen("TRUE");
                }
                else
                {
                    // Important - Connection Status
                    memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                            "FALSE",strlen("FALSE"));
                    pSlHttpServerResponse->ResponseData.token_value.len = \
                                                                strlen("FALSE");
                }
            }

            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                            g_token_get [1], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                        g_NetEntries[0].ssid,g_NetEntries[0].ssid_len);
                pSlHttpServerResponse->ResponseData.token_value.len = \
                                                      g_NetEntries[0].ssid_len;
            }
            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                            g_token_get [2], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                        g_NetEntries[1].ssid,g_NetEntries[1].ssid_len);
                pSlHttpServerResponse->ResponseData.token_value.len = \
                                                       g_NetEntries[1].ssid_len;
            }
            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                            g_token_get [3], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                        g_NetEntries[2].ssid,g_NetEntries[2].ssid_len);
                pSlHttpServerResponse->ResponseData.token_value.len = \
                                                       g_NetEntries[2].ssid_len;
            }
            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                            g_token_get [4], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                        g_NetEntries[3].ssid,g_NetEntries[3].ssid_len);
                pSlHttpServerResponse->ResponseData.token_value.len = \
                                                       g_NetEntries[3].ssid_len;
            }
            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                            g_token_get [5], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                        g_NetEntries[4].ssid,g_NetEntries[4].ssid_len);
                pSlHttpServerResponse->ResponseData.token_value.len = \
                                                      g_NetEntries[4].ssid_len;
            }

            else
                break;

        }
        break;

        case SL_NETAPP_HTTPPOSTTOKENVALUE_EVENT:
        {

            if((0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                          "__SL_P_USC", \
                 pSlHttpServerEvent->EventData.httpPostData.token_name.len)) && \
            (0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                     "Add", \
                     pSlHttpServerEvent->EventData.httpPostData.token_value.len)))
            {
                g_smartConfigDone = 1;
                Report("Ap Provisioning: Profile added Done in HTTP \n\r");

            }
            if(0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                     "__SL_P_USD", \
                     pSlHttpServerEvent->EventData.httpPostData.token_name.len))
            {
                /* Store new SSID name */
                if(pSlHttpServerEvent->EventData.httpPostData.token_value.len)
                {
                  osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);

                  SmartPlugNvmmFile.DeviceConfigData.DevConfig.SmartconfSsidLen =\
                    pSlHttpServerEvent->EventData.httpPostData.token_value.len;

                  memcpy((void *)&SmartPlugNvmmFile.DeviceConfigData.DevConfig.SmartconfSsid[0],\
                        pSlHttpServerEvent->EventData.httpPostData.token_value.data,\
                            SmartPlugNvmmFile.DeviceConfigData.DevConfig.SmartconfSsidLen);
                  SmartPlugNvmmFile.NvmmFileUpdated |= NVMEM_UPDATED_DEV_CFG;

                  osi_LockObjUnlock(&g_NvmemLockObj);
                }
                Report("Profile added - SSID in HTTP \n\r");
            }

            if(0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                         "__SL_P_USE", \
                         pSlHttpServerEvent->EventData.httpPostData.token_name.len))
            {

                if(pSlHttpServerEvent->EventData.httpPostData.token_value.data[0] \
                                                                        == '0')
                {
                    g_SecParams.Type =  SL_SEC_TYPE_OPEN;//SL_SEC_TYPE_OPEN

                }
                else if(pSlHttpServerEvent->EventData.httpPostData.token_value.data[0] \
                                                                        == '1')
                {
                    g_SecParams.Type =  SL_SEC_TYPE_WEP;//SL_SEC_TYPE_WEP

                }
                else if(pSlHttpServerEvent->EventData.httpPostData.token_value.data[0] == '2')
                {
                    g_SecParams.Type =  SL_SEC_TYPE_WPA;//SL_SEC_TYPE_WPA

                }
                else
                {
                    g_SecParams.Type =  SL_SEC_TYPE_OPEN;//SL_SEC_TYPE_OPEN
                }
                Report("Profile added - security type in HTTP \n\r");
            }
            if(0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                         "__SL_P_USF", \
                         pSlHttpServerEvent->EventData.httpPostData.token_name.len))
            {
                memcpy(WlanSecurityKey, \
                    pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                    pSlHttpServerEvent->EventData.httpPostData.token_value.len);
                WlanSecurityKey[pSlHttpServerEvent->EventData.httpPostData.token_value.len] = 0;
                g_SecParams.Key = WlanSecurityKey;
                g_SecParams.KeyLen = pSlHttpServerEvent->EventData.httpPostData.token_value.len;
                Report("Profile added - security key in HTTP \n\r");
            }
            if(0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                        "__SL_P_USG", \
                        pSlHttpServerEvent->EventData.httpPostData.token_name.len))
            {
                g_ucPriority = pSlHttpServerEvent->EventData.httpPostData.token_value.data[0] - 48;
                Report("Profile added - security priority in HTTP \n\r");
            }
            if(0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                         "__SL_P_US0", \
                         pSlHttpServerEvent->EventData.httpPostData.token_name.len))
            {
                g_smartConfigDone = 1;//DKS this has been removed from html
                Report("AP provisioning Done in HTTP \n\r");
            }
        }
        break;

      default:
          break;
    }
}

void StartApProvConfig()
{
  unsigned long IntervalVal = 60, counter = 0;
  int lRetVal = -1;
  char SSID_name[32];
  int Ssid_len;

  g_smartConfigDone = 0;
  g_StationConnToAP = 0;
  g_ConnectedToConfAP = 0;
  memset(&g_SecParams, 0, sizeof(SlSecParams_t));
  g_ucPriority = 0;
  memset(&g_NetEntries[0], 0, sizeof(g_NetEntries[SCAN_TABLE_SIZE]));

  /* Clear WDT */
  ClearWDT();

  /* Reset policy settings */
  sl_WlanPolicySet(SL_POLICY_CONNECTION , SL_CONNECTION_POLICY(0, 0, 0, 0, 0), 0, 0);

  WlanDisconnect();

  /* Clear WDT */
  ClearWDT();

  g_SmartplugLedCurrentState = SMART_CONFIG_OPER_IND;

  Report("AP provisioning Init \n\r");

  /* Start AP provisioning */

  Report("Scan Init.. \n\r");

  /* set scan policy - this starts the scan */
  sl_WlanPolicySet(SL_POLICY_SCAN , SL_SCAN_POLICY(1),
                          (unsigned char *)(IntervalVal), sizeof(IntervalVal));

  while(counter < 5) //be there in loop if provisioning fails
  {
    Report("Scan %d \n\r", counter);

    /* Get AP Scan Result for provisioning */

    /* delay 2 second to verify scan is started */
    OSI_DELAY(2000);

    /* Clear WDT */
    ClearWDT();

    /* lRetVal indicates the valid number of entries The scan results are occupied in g_NetEntries[] */
    lRetVal = sl_WlanGetNetworkList(0, SCAN_TABLE_SIZE, g_NetEntries);

    if(lRetVal > 0)
    {
      break;
    }
    counter++;
  }

  /* Disable scan - set scan policy - this stops the scan */
  sl_WlanPolicySet(SL_POLICY_SCAN , SL_SCAN_POLICY(0),
                        (unsigned char *)(IntervalVal), sizeof(IntervalVal));

  if(lRetVal > 0)
  {
    Report("Enter AP Mode \n\r");

    /* Enter AP Mode for provisioning */
    sl_WlanSetMode(ROLE_AP);

    /* Restart Network processor */
    sl_Stop(SL_STOP_TIMEOUT);

    /* Clear WDT */
    ClearWDT();
    OSI_DELAY(200);

    if(ROLE_AP == sl_Start(NULL,NULL,NULL))
    {
      /* wait till a station connected to this AP */
      while(!(g_StationConnToAP))
      {
        /* Clear WDT */
        ClearWDT();
        OSI_DELAY(200);
      }

      /* Stop Internal HTTP Server */
      sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);

      /* Start Internal HTTP Server */
      sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);

     /*
      * Wait for AP Configuraiton, Open Browser and Configure AP
      */
      while(!g_smartConfigDone)
      {
        /* Clear WDT */
        ClearWDT();
        OSI_DELAY(200);
      }

      Report("Enter Station Mode \n\r");

      /* Enter back to STA Mode */
      sl_WlanSetMode(ROLE_STA);

      /* Restart Network processor */
      sl_Stop(SL_STOP_TIMEOUT);

      /* Clear WDT */
      ClearWDT();
      OSI_DELAY(200);

      if(ROLE_STA != sl_Start(NULL,NULL,NULL))
      {
        Report("STA Mode enter failed \n\r");
        g_smartConfigDone = 0;
      }
      else
      {
        /* Delete all profiles (0xFF) stored */
        sl_WlanProfileDel(WLAN_DEL_ALL_PROFILES);

        /* Clear WDT */
        ClearWDT();

        /* Add Profile */
        Ssid_len = SmartPlugNvmmFile.DeviceConfigData.DevConfig.SmartconfSsidLen;
        memcpy((void *)&SSID_name[0],(void const*)&SmartPlugNvmmFile.DeviceConfigData.DevConfig.SmartconfSsid[0],Ssid_len);
        SSID_name[Ssid_len] = 0;

        Report("SSID of AP %s\n\r", SSID_name);

        sl_WlanProfileAdd((signed char*)SSID_name, Ssid_len, 0, &g_SecParams, 0,g_ucPriority,0);

        Report("Profile stored in Flash \n\r");
      }
    }
    else
    {
      Report("AP Mode enter failed \n\r");
    }
  }

  if(1 == g_smartConfigDone)
  {
    /* Enable auto connect (connection to stored profiles according to priority) */
    sl_WlanPolicySet(SL_POLICY_CONNECTION , SL_CONNECTION_POLICY(1, 0, 0, 0, 0), 0, 0);
    Report("AP provisioning is Done \n\r");
  }
  else
  {
    Report("Error in AP provisioning \n\r");
  }
  g_smartConfigDone = 0;
  g_StationConnToAP = 0;
  g_SmartplugLedCurrentState = WLAN_CONNECT_OPER_IND;
}

void StartSmartConfig()
{
  g_smartConfigDone = 0;
  /* Delete all profiles (0xFF) stored */
  sl_WlanProfileDel(WLAN_DEL_ALL_PROFILES);
  osi_LockObjLock(&g_NvmemLockObj, SL_OS_WAIT_FOREVER);
  SmartPlugNvmmFile.DeviceConfigData.DevConfig.SmartconfSsidLen = 0;
  osi_LockObjUnlock(&g_NvmemLockObj);

  /* Clear WDT */
  ClearWDT();

  /* Set Auto policy settings + smart config */
  sl_WlanPolicySet(SL_POLICY_CONNECTION , SL_CONNECTION_POLICY(1, 0, 0, 0, 1), 0, 0);

  WlanDisconnect();

  /* Clear WDT */
  ClearWDT();

  g_SmartplugLedCurrentState = SMART_CONFIG_OPER_IND;

  Report("SmartConf Init \n\r");

  /* Start SmartConfig
   * This example uses the unsecured SmartConfig method
   */
  sl_WlanSmartConfigStart(0,                            //groupIdBitmask
                           SMART_CONFIG_CIPHER_NONE,    //cipher
                           0,                           //publicKeyLen
                           0,                           //group1KeyLen
                           0,                           //group2KeyLen
                           (const unsigned char*)"",                          //publicKey
                           (const unsigned char*)"",                          //group1Key
                           (const unsigned char*)"");                         //group2Key

  while(g_smartConfigDone == 0)
  {
    OSI_DELAY(200);
    /* Clear WDT */
    ClearWDT();
  }

  if(1 == g_smartConfigDone)
  {
    /* Enable auto connect (connection to stored profiles according to priority) */
    sl_WlanPolicySet(SL_POLICY_CONNECTION , SL_CONNECTION_POLICY(1, 0, 0, 0, 0), 0, 0);
    Report("SmartConf Done \n\r");
  }
  else
  {
    Report("SmartConf Stoped \n\r");
  }
  g_smartConfigDone = 0;
  g_SmartplugLedCurrentState = WLAN_CONNECT_OPER_IND;
}

void StartWpsConfig(void)
{
  SlSecParams_t secParamsOut;
  char SSID_name[32];
  int Ssid_len;

  /* Get existing profile SSID */
  if(0 == SmartPlugNvmmFile.DeviceConfigData.DevConfig.SmartconfSsidLen )
  {
    Report("WPS SSID name get error\n\r");
    return;
  }

  /* Clear WDT */
  ClearWDT();

  Ssid_len = SmartPlugNvmmFile.DeviceConfigData.DevConfig.SmartconfSsidLen;
  memcpy((void *)&SSID_name[0],(void const*)&SmartPlugNvmmFile.DeviceConfigData.DevConfig.SmartconfSsid[0],Ssid_len);

  /* Reset policy settings */
  sl_WlanPolicySet(SL_POLICY_CONNECTION , SL_CONNECTION_POLICY(0, 0, 0, 0, 0), 0, 0);

  WlanDisconnect();

  /* Clear WDT */
  ClearWDT();

  g_SmartplugLedCurrentState = SMART_CONFIG_OPER_IND;

  Report("WPS Init... \n\r");

  secParamsOut.Key = (signed char*)"";
  secParamsOut.KeyLen = 0;
  secParamsOut.Type = SL_SEC_TYPE_WPS_PBC;

  sl_WlanConnect((signed char*)SSID_name, Ssid_len, 0, &secParamsOut,0);

  OSI_DELAY(1000);//wait 1sec to connect

  /* Enable auto connect (connection to stored profiles according to priority) */
  sl_WlanPolicySet(SL_POLICY_CONNECTION , SL_CONNECTION_POLICY(1, 0, 0, 0, 0), 0, 0);
}


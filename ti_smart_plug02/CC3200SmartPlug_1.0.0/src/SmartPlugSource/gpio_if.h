//*****************************************************************************
// File: gpio_if.h
//
// Description: GPIO handling header file of Smartplug gen-1 application
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

#ifndef __GPIOIF_H__
#define __GPIOIF_H__

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

typedef enum
{
    NO_LED = 0,        //R =0, G = 0, B =0
    RED_LED,           //R =1, G = 0, B =0
    GREEN_LED,         //R =0, G = 1, B =0
    BLUE_LED,          //R =0, G = 0, B =1
    MAGENTA_LED        //R =1, G = 0, B =1

} LedColors;

typedef enum
{
    NO_LED_IND  = 0,              //blank
    DEVICE_POW_ON_IND,            //red steady
    DEVICE_ERROR_IND,             //red blink quickly
    SMART_CONFIG_OPER_IND,        //blue blink quickly
    WLAN_CONNECT_OPER_IND,        //blue blink slowely
    CONNECTED_TO_AP_IND,          //blue steady
    CONNECTED_ANDROID_IND,        //magenta steady
    ANDROID_DATA_TRANSFER_IND,    //magenta blink quickly
    CONNECTED_CLOUD_IND,          //green steady
    CLOUD_DATA_TRANSFER_IND       //green blink quickly

} LedIndicationName;

/* Considering 100ms duty cycle */
typedef enum
{
    DEVICE_POW_ON_TOGGLE_TIME           = 5,    //red slow blink
    DEVICE_ERROR_TOGGLE_TIME            = 1,    //red blink quickly
    SMART_CONFIG_OPER_TOGGLE_TIME       = 1,    //blue blink quickly
    WLAN_CONNECT_OPER_TOGGLE_TIME       = 5,    //blue blink slowely
    ANDROID_DATA_TRANSFER_TOGGLE_TIME   = 1,    //magenta blink quickly
    CLOUD_DATA_TRANSFER_TOGGLE_TIME     = 1     //green blink quickly

} LedToggleTime;

//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern void GPIO_IF_GetPortNPin(unsigned char ucPin,
                     UINT32 *puiGPIOPort,
                     unsigned char *pucGPIOPin);

extern void GPIO_IF_Set(unsigned int uiGPIOPort,
             unsigned char ucGPIOPin,
             unsigned char ucGPIOValue);

extern unsigned char GPIO_IF_Get( unsigned int uiGPIOPort,
             unsigned char ucGPIOPin);

extern void GPIO_IF_LedOn(LedColors ledNum);

extern void GPIO_IF_EnableClock(unsigned int uiGPIOPort);

extern void GPIO_IF_ConfigureNIntEnable(unsigned int uiGPIOPort,
                                  unsigned char ucGPIOPin,
                                  unsigned int uiIntType,
                                  void (*pfnIntHandler)(void));

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __GPIOIF_H__


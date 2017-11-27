//*****************************************************************************
// File: pinmux.h
//
// Description: PINMUX header file of GPIOs of Smartplug gen-1 application
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

#ifndef __PINMUX_H__
#define __PINMUX_H__

/* LED Pin & GPIOs - all output */
#define SMARTPLUG_LED_RED_PIN              PIN_64  //Output
#define SMARTPLUG_LED_RED_GPIO             9
#define SMARTPLUG_LED_GREEN_PIN            PIN_01  //Output
#define SMARTPLUG_LED_GREEN_GPIO           10
#define SMARTPLUG_LED_BLUE_PIN             PIN_02  //Output
#define SMARTPLUG_LED_BLUE_GPIO            11

/* RELAY Pin & GPIOs */
#define SMARTPLUG_RELAY_STATE_PIN          PIN_15  //Input //SW2
#define SMARTPLUG_RELAY_STATE_GPIO         22
#define SMARTPLUG_RELAY_CTRL_PIN           PIN_07  //Output
#define SMARTPLUG_RELAY_CTRL_GPIO          16

/* SMARTCONFIG & WAC Pin & GPIOs */
#ifdef SMARTPLUG_HW
  #define SMARTPLUG_SMARTCNFIG_PIN         PIN_08  //Input
  #define SMARTPLUG_SMARTCNFIG_GPIO        17
 #define SMARTPLUG_SMARTCNFIG_PIN         PIN_04  //Input //SW3
  #define SMARTPLUG_SMARTCNFIG_GPIO        13
#else
  #define SMARTPLUG_SMARTCNFIG_PIN         PIN_04  //Input //SW3
  #define SMARTPLUG_SMARTCNFIG_GPIO        13
#endif
#define SMARTPLUG_WAC_PIN                  PIN_18  //Input
#define SMARTPLUG_WAC_GPIO                 28

/* METROLOGY Wh pulse output Pin & GPIOs */
#define SMARTPLUG_WH_PULSE_PIN             PIN_60  //Output
#define SMARTPLUG_WH_PULSE_GPIO            5

typedef struct SmartPlugGpioConfig_t
{
  /* LED, RELAY, SMARTCONFIG & WAC, METROLOGY PULSE - Port*/
  UINT32 LedRedPort;
  UINT32 LedGreenPort;
  UINT32 LedBluePort;
  UINT32 RelayStatePort;
  UINT32 RelayCtrlPort;
  UINT32 SmartConfgPort;
  UINT32 WacPort;
  UINT32 WhPulsePort;

  /* LED, RELAY, SMARTCONFIG & WAC, METROLOGY PULSE - Pad*/
  UINT8  LedRedPad;
  UINT8  LedGreenPad;
  UINT8  LedBluePad;
  UINT8  RelayStatePad;
  UINT8  RelayCtrlPad;
  UINT8  SmartConfgPad;
  UINT8  WacPad;
  UINT8  WhPulsePad;
}
SmartPlugGpioConfig_t;

extern void PinMuxConfig(void);
extern void LedConfiguration( void );
extern void RelayConfiguration( void );

#endif //  __PINMUX_H__

//*****************************************************************************
// File: pinmux.c
//
// Description: PINMUX functions of GPIOs of Smartplug gen-1 application
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
#include "pinmux.h"
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_gpio.h"
#include "rom_map.h"
#include "pin.h"
#include "gpio.h"
#include "prcm.h"
#include "uart.h"
#include "gpio_if.h"

SmartPlugGpioConfig_t  SmartPlugGpioConfig;

void PinMuxConfig(void)
{
  //
  // Enable Peripheral Clocks - Uart, WDT
  //
  MAP_PRCMPeripheralClkEnable(PRCM_SSPI,PRCM_RUN_MODE_CLK|PRCM_SLP_MODE_CLK);//clock for serial flash
  MAP_PRCMPeripheralClkEnable(PRCM_UARTA0,PRCM_RUN_MODE_CLK|PRCM_SLP_MODE_CLK);
  MAP_PRCMPeripheralClkEnable(PRCM_WDT, PRCM_RUN_MODE_CLK|PRCM_SLP_MODE_CLK); // PRCM_WDT is the index WDT in PRCM_PeriphRegsList array of structures

  //
  // Configure PIN_55 (GP1) for UART0 AUART0_TX
  //
  MAP_PinTypeUART(PIN_55, PIN_MODE_3);

  //
  // Configure PIN_57 (GP2) for UART0 AUART0_RX
  //
  //MAP_PinTypeUART(PIN_57, PIN_MODE_3);//Rx UART line used for ADC Ch 0

  //
  //Get port & pad from GPIO number
  //
  GPIO_IF_GetPortNPin(SMARTPLUG_LED_RED_GPIO,
                        &SmartPlugGpioConfig.LedRedPort,
                        &SmartPlugGpioConfig.LedRedPad);

  GPIO_IF_GetPortNPin(SMARTPLUG_LED_GREEN_GPIO,
                        &SmartPlugGpioConfig.LedGreenPort,
                        &SmartPlugGpioConfig.LedGreenPad);

  GPIO_IF_GetPortNPin(SMARTPLUG_LED_BLUE_GPIO,
                        &SmartPlugGpioConfig.LedBluePort,
                        &SmartPlugGpioConfig.LedBluePad);

  GPIO_IF_GetPortNPin(SMARTPLUG_RELAY_STATE_GPIO,
                        &SmartPlugGpioConfig.RelayStatePort,
                        &SmartPlugGpioConfig.RelayStatePad);

  GPIO_IF_GetPortNPin(SMARTPLUG_RELAY_CTRL_GPIO,
                        &SmartPlugGpioConfig.RelayCtrlPort,
                        &SmartPlugGpioConfig.RelayCtrlPad);

  GPIO_IF_GetPortNPin(SMARTPLUG_SMARTCNFIG_GPIO,
                        &SmartPlugGpioConfig.SmartConfgPort,
                        &SmartPlugGpioConfig.SmartConfgPad);

#ifdef WAC_EN
  GPIO_IF_GetPortNPin(SMARTPLUG_WAC_GPIO,
                        &SmartPlugGpioConfig.WacPort,
                        &SmartPlugGpioConfig.WacPad);
#endif

  GPIO_IF_GetPortNPin(SMARTPLUG_WH_PULSE_GPIO,
                        &SmartPlugGpioConfig.WhPulsePort,
                        &SmartPlugGpioConfig.WhPulsePad);

  //
  // Enable Peripheral Clocks - GPIO
  //
  GPIO_IF_EnableClock(SmartPlugGpioConfig.LedRedPort);
  GPIO_IF_EnableClock(SmartPlugGpioConfig.LedGreenPort);
  GPIO_IF_EnableClock(SmartPlugGpioConfig.LedBluePort);

  GPIO_IF_EnableClock(SmartPlugGpioConfig.RelayStatePort);
  GPIO_IF_EnableClock(SmartPlugGpioConfig.RelayCtrlPort);

  GPIO_IF_EnableClock(SmartPlugGpioConfig.SmartConfgPort);
#ifdef WAC_EN
  GPIO_IF_EnableClock(SmartPlugGpioConfig.WacPort);
#endif
  GPIO_IF_EnableClock(SmartPlugGpioConfig.WhPulsePort);

  //
  // Configure PIN_07 for GPIOOutput - RELAY CTRL tristate output = input mode
  //
  MAP_PinTypeGPIO(SMARTPLUG_RELAY_CTRL_PIN, PIN_MODE_0, false);
  MAP_GPIODirModeSet(SmartPlugGpioConfig.RelayCtrlPort, SmartPlugGpioConfig.RelayCtrlPad, GPIO_DIR_MODE_IN);//Tri-state
  //MAP_GPIODirModeSet(SmartPlugGpioConfig.RelayCtrlPort, SmartPlugGpioConfig.RelayCtrlPad, GPIO_DIR_MODE_OUT);//DKS

  //
  // Configure PIN_64 for GPIOOutput - RED LED
  //
  MAP_PinTypeGPIO(SMARTPLUG_LED_RED_PIN, PIN_MODE_0, false);
  MAP_GPIODirModeSet(SmartPlugGpioConfig.LedRedPort, SmartPlugGpioConfig.LedRedPad, GPIO_DIR_MODE_OUT);

  //
  // Configure PIN_01 for GPIOOutput - GREEN LED (ORRANGE in LP)
  //
  MAP_PinTypeGPIO(SMARTPLUG_LED_GREEN_PIN, PIN_MODE_0, false);
  MAP_GPIODirModeSet(SmartPlugGpioConfig.LedGreenPort, SmartPlugGpioConfig.LedGreenPad, GPIO_DIR_MODE_OUT);

  //
  // Configure PIN_02 for GPIOOutput - BLUE LED  (GREEN in LP)
  //
  MAP_PinTypeGPIO(SMARTPLUG_LED_BLUE_PIN, PIN_MODE_0, false);
  MAP_GPIODirModeSet(SmartPlugGpioConfig.LedBluePort, SmartPlugGpioConfig.LedBluePad, GPIO_DIR_MODE_OUT);

  //
  // Configure PIN_15 for GPIOInput - RELAY state (SW2 in LP)
  //
  MAP_PinTypeGPIO(SMARTPLUG_RELAY_STATE_PIN, PIN_MODE_0, false);
  MAP_GPIODirModeSet(SmartPlugGpioConfig.RelayStatePort, SmartPlugGpioConfig.RelayStatePad, GPIO_DIR_MODE_IN);

  //
  // Configure PIN_08 for GPIOInput - SMARTCONFIG (SW3 in LP)
  //
  MAP_PinTypeGPIO(SMARTPLUG_SMARTCNFIG_PIN, PIN_MODE_0, false);
  MAP_GPIODirModeSet(SmartPlugGpioConfig.SmartConfgPort, SmartPlugGpioConfig.SmartConfgPad, GPIO_DIR_MODE_IN);

#ifdef WAC_EN
  //
  // Configure PIN_18 for GPIOInput - WAC
  //
  MAP_PinTypeGPIO(SMARTPLUG_WAC_PIN, PIN_MODE_0, false);
  MAP_GPIODirModeSet(SmartPlugGpioConfig.WacPort, SmartPlugGpioConfig.WacPad, GPIO_DIR_MODE_IN);
#endif

  //
  // Configure PIN_60 for GPIOOutput - METROLOGY WH PULSE
  //
#ifdef SMARTPLUG_HW
  MAP_PinTypeGPIO(SMARTPLUG_WH_PULSE_PIN, PIN_MODE_0, false);
  MAP_GPIODirModeSet(SmartPlugGpioConfig.WhPulsePort, SmartPlugGpioConfig.WhPulsePad, GPIO_DIR_MODE_OUT);
  GPIO_IF_Set(SmartPlugGpioConfig.WhPulsePort, SmartPlugGpioConfig.WhPulsePad, 0);//low pulse default
#else
  //Pinmux for Network Logs for debug
  PinModeSet(PIN_60,PIN_MODE_1);
  PinModeSet(PIN_62,PIN_MODE_1);
#endif
  //
  // Configure LEDs
  //
  LedConfiguration();
}

//*****************************************************************************
// File: gpio_if.c
//
// Description: GPIO handling functions of Smartplug gen-1 application
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
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "interrupt.h"
#include "pin.h"
#include "gpio.h"
#include "hw_apps_rcm.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "gpio_if.h"
#include <osi.h>
#include "pinmux.h"

//****************************************************************************
//                      GLOBAL VARIABLES
//****************************************************************************
static unsigned long ulReg[]=
{
    GPIOA0_BASE,
    GPIOA1_BASE,
    GPIOA2_BASE,
    GPIOA3_BASE
};

//*****************************************************************************
// Variables to store TIMER Port,Pin values
//*****************************************************************************
UINT8 GpioA0ClkEn = 0, GpioA1ClkEn = 0, GpioA2ClkEn = 0, GpioA3ClkEn = 0;

extern SmartPlugGpioConfig_t  SmartPlugGpioConfig;

//****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS
//****************************************************************************

//****************************************************************************
//
//! Get the port and pin of a given GPIO
//!
//! \param ucPin is the pin to be set-up as a GPIO (0:39)
//! \param puiGPIOPort is the pointer to store GPIO port address return value
//! \param pucGPIOPin is the pointer to store GPIO pin return value
//!
//! This function
//!    1. Return the GPIO port address and pin for a given external pin number
//!
//! \return None.
//
//****************************************************************************
void
GPIO_IF_GetPortNPin(unsigned char ucPin,
        UINT32 *puiGPIOPort,
          unsigned char *pucGPIOPin)
{
  //
  // Get the GPIO pin from the external Pin number
  //
  *pucGPIOPin = 1 << (ucPin % 8);
  //
  // Get the GPIO port from the external Pin number
  //
  *puiGPIOPort = ulReg[(ucPin / 8)];
}

//*****************************************************************************
//
//! GPIO_IF_EnableClock
//!
//! \param  port number
//!
//! \return none
//!
//! \brief  Turns a specific clock
//
//*****************************************************************************
void GPIO_IF_EnableClock(unsigned int uiGPIOPort)
{
  switch(uiGPIOPort)
  {
    case GPIOA0_BASE:
        if(0 == GpioA0ClkEn)
        {
          MAP_PRCMPeripheralClkEnable(PRCM_GPIOA0, PRCM_RUN_MODE_CLK|PRCM_SLP_MODE_CLK);
          GpioA0ClkEn = 1;
        }
        break;
    case GPIOA1_BASE:
        if(0 == GpioA1ClkEn)
        {
          MAP_PRCMPeripheralClkEnable(PRCM_GPIOA1, PRCM_RUN_MODE_CLK|PRCM_SLP_MODE_CLK);
          GpioA1ClkEn = 1;
        }
        break;
    case GPIOA2_BASE:
        if(0 == GpioA2ClkEn)
        {
          MAP_PRCMPeripheralClkEnable(PRCM_GPIOA2, PRCM_RUN_MODE_CLK|PRCM_SLP_MODE_CLK);
          GpioA2ClkEn = 1;
        }
        break;
    case GPIOA3_BASE:
        if(0 == GpioA3ClkEn)
        {
          MAP_PRCMPeripheralClkEnable(PRCM_GPIOA3, PRCM_RUN_MODE_CLK|PRCM_SLP_MODE_CLK);
          GpioA3ClkEn = 1;
        }
        break;
  }
}

//*****************************************************************************
//
//! Turn LED On
//!
//! \param  ledNum is the LED Number
//!
//! \return none
//!
//! \brief  Turns a specific LED Off
//
//*****************************************************************************
void
GPIO_IF_LedOn(LedColors ledname)
{
  switch(ledname)
  {
    case NO_LED:
    {
      /* Switch OFF all LEDs */
      #ifdef SMARTPLUG_HW //drive low makes LED glow
      GPIO_IF_Set(SmartPlugGpioConfig.LedRedPort, SmartPlugGpioConfig.LedRedPad, 1);
      GPIO_IF_Set(SmartPlugGpioConfig.LedGreenPort, SmartPlugGpioConfig.LedGreenPad, 1);
      GPIO_IF_Set(SmartPlugGpioConfig.LedBluePort, SmartPlugGpioConfig.LedBluePad, 1);
      #else
      GPIO_IF_Set(SmartPlugGpioConfig.LedRedPort, SmartPlugGpioConfig.LedRedPad, 0);
      GPIO_IF_Set(SmartPlugGpioConfig.LedGreenPort, SmartPlugGpioConfig.LedGreenPad, 0);
      GPIO_IF_Set(SmartPlugGpioConfig.LedBluePort, SmartPlugGpioConfig.LedBluePad, 0);
      #endif
      break;
    }
    case RED_LED:
    {
      /* Switch ON RED LED */
      #ifdef SMARTPLUG_HW //drive low makes LED glow
      GPIO_IF_Set(SmartPlugGpioConfig.LedRedPort, SmartPlugGpioConfig.LedRedPad, 0);
      GPIO_IF_Set(SmartPlugGpioConfig.LedGreenPort, SmartPlugGpioConfig.LedGreenPad, 1);
      GPIO_IF_Set(SmartPlugGpioConfig.LedBluePort, SmartPlugGpioConfig.LedBluePad, 1);
      #else
      GPIO_IF_Set(SmartPlugGpioConfig.LedRedPort, SmartPlugGpioConfig.LedRedPad, 1);
      GPIO_IF_Set(SmartPlugGpioConfig.LedGreenPort, SmartPlugGpioConfig.LedGreenPad, 0);
      GPIO_IF_Set(SmartPlugGpioConfig.LedBluePort, SmartPlugGpioConfig.LedBluePad, 0);
      #endif
      break;
    }
    case GREEN_LED:
    {
      /* Switch ON GREEN LED */
      #ifdef SMARTPLUG_HW //drive low makes LED glow
      GPIO_IF_Set(SmartPlugGpioConfig.LedRedPort, SmartPlugGpioConfig.LedRedPad, 1);
      GPIO_IF_Set(SmartPlugGpioConfig.LedGreenPort, SmartPlugGpioConfig.LedGreenPad, 0);
      GPIO_IF_Set(SmartPlugGpioConfig.LedBluePort, SmartPlugGpioConfig.LedBluePad, 1);
      #else
      GPIO_IF_Set(SmartPlugGpioConfig.LedRedPort, SmartPlugGpioConfig.LedRedPad, 0);
      GPIO_IF_Set(SmartPlugGpioConfig.LedGreenPort, SmartPlugGpioConfig.LedGreenPad, 1);
      GPIO_IF_Set(SmartPlugGpioConfig.LedBluePort, SmartPlugGpioConfig.LedBluePad, 0);
      #endif
      break;
    }
    case BLUE_LED:
    {
      /* Switch ON BLUE LED */
      #ifdef SMARTPLUG_HW //drive low makes LED glow
      GPIO_IF_Set(SmartPlugGpioConfig.LedRedPort, SmartPlugGpioConfig.LedRedPad, 1);
      GPIO_IF_Set(SmartPlugGpioConfig.LedGreenPort, SmartPlugGpioConfig.LedGreenPad, 1);
      GPIO_IF_Set(SmartPlugGpioConfig.LedBluePort, SmartPlugGpioConfig.LedBluePad, 0);
      #else
      GPIO_IF_Set(SmartPlugGpioConfig.LedRedPort, SmartPlugGpioConfig.LedRedPad, 0);
      GPIO_IF_Set(SmartPlugGpioConfig.LedGreenPort, SmartPlugGpioConfig.LedGreenPad, 0);
      GPIO_IF_Set(SmartPlugGpioConfig.LedBluePort, SmartPlugGpioConfig.LedBluePad, 1);
      #endif
      break;
    }
    case MAGENTA_LED:
    {
      /* Switch ON BLUE LED */
      #ifdef SMARTPLUG_HW //drive low makes LED glow
      GPIO_IF_Set(SmartPlugGpioConfig.LedRedPort, SmartPlugGpioConfig.LedRedPad, 0);
      GPIO_IF_Set(SmartPlugGpioConfig.LedGreenPort, SmartPlugGpioConfig.LedGreenPad, 1);
      GPIO_IF_Set(SmartPlugGpioConfig.LedBluePort, SmartPlugGpioConfig.LedBluePad, 0);
      #else
      GPIO_IF_Set(SmartPlugGpioConfig.LedRedPort, SmartPlugGpioConfig.LedRedPad, 1);
      GPIO_IF_Set(SmartPlugGpioConfig.LedGreenPort, SmartPlugGpioConfig.LedGreenPad, 0);
      GPIO_IF_Set(SmartPlugGpioConfig.LedBluePort, SmartPlugGpioConfig.LedBluePad, 1);
      #endif
      break;
    }
    default:
      break;
  }
}

//****************************************************************************
//
//! Set a value to the specified GPIO pin
//!
//! \param ucPin is the GPIO pin to be set (0:39)
//! \param uiGPIOPort is the GPIO port address
//! \param ucGPIOPin is the GPIO pin of the specified port
//! \param ucGPIOValue is the value to be set
//!
//! This function
//!    1. Sets a value to the specified GPIO pin
//!
//! \return None.
//
//****************************************************************************
void
GPIO_IF_Set( unsigned int uiGPIOPort,
             unsigned char ucGPIOPin,
             unsigned char ucGPIOValue)
{
  //
  // Set the corresponding bit in the bitmask
  //
  if(ucGPIOValue)
  {
    ucGPIOValue = ucGPIOPin;
  }
  else
  {
    ucGPIOValue = 0;
  }
  //
  // Invoke the API to set the value
  //
  MAP_GPIOPinWrite(uiGPIOPort,ucGPIOPin,ucGPIOValue);
}

//****************************************************************************
//
//! Set a value to the specified GPIO pin
//!
//! \param ucPin is the GPIO pin to be set (0:39)
//! \param uiGPIOPort is the GPIO port address
//! \param ucGPIOPin is the GPIO pin of the specified port
//!
//! This function
//!    1. Gets a value of the specified GPIO pin
//!
//! \return value of the GPIO pin
//
//****************************************************************************
unsigned char
GPIO_IF_Get( unsigned int uiGPIOPort,
             unsigned char ucGPIOPin
             )
{
  unsigned char ucGPIOValue;
  long lGPIOStatus;

  //
  // Invoke the API to Get the value
  //

  lGPIOStatus =  MAP_GPIOPinRead(uiGPIOPort,ucGPIOPin);

  //
  // Set the corresponding bit in the bitmask
  //
  ucGPIOValue = lGPIOStatus & ucGPIOPin;
  ucGPIOValue = ucGPIOValue ? 1:0;

  return ucGPIOValue;
}

#if 0
//****************************************************************************
//
//! Configures the GPIO selected as input to generate interrupt on activity
//!
//! \param uiGPIOPort is the GPIO port address
//! \param ucGPIOPin is the GPIO pin of the specified port
//! \param uiIntType is the type of the interrupt (refer gpio.h)
//! \param pfnIntHandler is the interrupt handler to register
//!
//! This function
//!    1. Sets GPIO interrupt type
//!    2. Registers Interrupt handler
//!    3. Enables Interrupt
//!
//! \return None
//
//****************************************************************************
void
GPIO_IF_ConfigureNIntEnable(unsigned int uiGPIOPort,
                                  unsigned char ucGPIOPin,
                                  unsigned int uiIntType,
                                  void (*pfnIntHandler)(void))
{
  UINT32 ulIntNum;
  //
  // Set GPIO interrupt type
  //
  MAP_GPIOIntTypeSet(uiGPIOPort,ucGPIOPin,uiIntType);
  //
  // Register Interrupt handler
  //
  //
  // Get the interrupt number associated with the specified GPIO.
  //
  ulIntNum = MAP_GPIOGetIntNumber(uiGPIOPort);

  #ifdef SL_PLATFORM_MULTI_THREADED
      osi_InterruptRegister(ulIntNum, (P_OSI_INTR_ENTRY)pfnIntHandler, INT_PRIORITY_LVL_7);
  #else
      //
      // Set the priority
      //
      IntPrioritySet(ulIntNum, INT_PRIORITY_LVL_7);

      IntPendClear(ulIntNum);

      MAP_GPIOIntRegister(uiGPIOPort,pfnIntHandler);
  #endif

  //
  // Enable Interrupt
  //
  MAP_GPIOIntClear(uiGPIOPort,ucGPIOPin);
  MAP_GPIOIntEnable(uiGPIOPort,ucGPIOPin);
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

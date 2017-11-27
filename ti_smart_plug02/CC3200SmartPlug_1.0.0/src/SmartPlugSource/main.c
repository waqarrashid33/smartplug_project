//*****************************************************************************
// File: main.c
//
// Description: Main function of Smartplug gen-1 application
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
#undef SMARTPLUG_HW // Added Waqar

#include <hw_types.h>
#include <hw_ints.h>
#include <datatypes.h>
#include <interrupt.h>
#include "simplelink.h"
#include <pinmux.h>
#include <osi.h>
#include <prcm.h>
#include "uart_logger.h"
#include "exosite_meta.h"
#include "metrology.h"
#include "smartplug_task.h"
#include "rom_map.h"
#include "cc3200.h"


#ifdef USE_FREERTOS
  #ifdef ccs
  extern void (* const g_pfnVectors[])(void);
  extern void InitNvicCCS( void );
  #else
  extern uVectorEntry __vector_table;
  extern void InitNvicEwarm( void );
  #endif
#endif //#ifdef USE_FREERTOS

extern OsiMsgQ_t SmartPlugTaskMsgQ;
extern OsiMsgQ_t TimeHandlerTaskMsgQ;
OsiTaskHandle SmartPlugTaskHndl;
OsiTaskHandle TimeHandlerTaskHndl;

extern UINT8 g_SmartplugInitDone;
extern volatile UINT8 g_NwpInitdone;

void main()
{
  int param = 0;
  //
  // Setup the interrupt vector table for Free RTOS
  //
  #ifdef USE_FREERTOS
      #ifdef ccs
        //! Sets the NVIC VTable base.
        //! \param ulVtableBase specifies the new base address of VTable
        //! This function is used to specify a new base address for the VTable.
        //! This function must be called before using IntRegister() for registering
        //! any interrupt handler.
        IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
        InitNvicCCS();
      #else
        IntVTableBaseSet((unsigned long)&__vector_table);
        InitNvicEwarm();
      #endif
  //
  // Enable the SYSTICK interrupt
  //
  IntEnable(FAULT_SYSTICK);

  IntMasterEnable(); //Change
  #endif //#ifdef USE_FREERTOS

  //
  // Defualt Board Initialization
  //
  PRCMCC3200MCUInit();

  //
  // Pin mux configuration for smartplug, includes initial LED & Relay settings
  //
  PinMuxConfig();

  //
  // Uart Initialization
  //
  InitTerm();

  //
  // Check the wakeup source.
  //

  if(MAP_PRCMSysResetCauseGet() == PRCM_WDT_RESET)
  {
    Report("WDT Reset Wakeup \n\r");
    /* Perform a Hib Reset after WDT reset - workaround for WDT reset */
    HIBReset();
  }
  else if(MAP_PRCMSysResetCauseGet() == PRCM_HIB_EXIT)
  {
    Report("HIB Reset Wakeup \n\r");
  }
  else
  {
    Report("Power ON Wakeup \n\r");
  }

  // Solution Problem 1, Manually writing to Gate enable register of WDT
  //unsigned int * ptr = (unsigned int *)0x44025078;
  //Report("Address = %X\n\r", ptr);
  //Report("Value at address = %X\n\r", *ptr);
  //*ptr = (unsigned int )0x00000101;
  HWREG(0x44025078) |= 0x00000101;
  //Report("Value at address = %X\n\r", *ptr);

  #ifdef SMARTPLUG_HW //smartplug has reverse logic - (!ulPinState)
      Report("SMARTPLUG_HW Defined");
  #else
      Report("SMARTPLUG_HW not Defined");
  #endif

  WDTInit();//DKS test

  /* Init Timers for smartplug */
  TimerInit();

#ifdef TIME_STAMP_EN
  EnableTimeStampTimer();
#endif

  //
  // Start the SmartPlugTask task
  //
  g_SmartplugInitDone = 0; //Flag for first time init indication
  /* Indicate flag with NWP reset */
  g_NwpInitdone = 0;

  osi_MsgQCreate(&TimeHandlerTaskMsgQ,"TimeHandlerTaskMsgQ",sizeof(int),1);
  osi_MsgQCreate(&SmartPlugTaskMsgQ,"SmartPlugTaskMsgQ",sizeof(int),1);

  osi_TaskCreate( TimeHandlerTask,
                  (const signed char*)"TimeHandler",
                  4096,
                  (void *) &param,
                  TIME_HANDLER_TASK_PRIO, /* highest priority */
                  &TimeHandlerTaskHndl );

  osi_TaskCreate( SmartPlugTask,
                  (const signed char*)"Smart Plug",
                  6144,
                  (void *) &param,
                  SMARTPLUG_BASE_TASK_PRIO, /* lowest priority */
                  &SmartPlugTaskHndl );

  /* Enable 1sec timer */
  EnableBroadcastTimer(); // Check Enabling function above for error as in case of WDT
                          // Read more details about this timer to find out where is it initialized first

  //
  // start OS schedular
  //
  osi_start();
}

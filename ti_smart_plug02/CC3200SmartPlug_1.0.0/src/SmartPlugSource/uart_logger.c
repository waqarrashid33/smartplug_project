//*****************************************************************************
// File: uart_logger.c
//
// Description: UART logger functions of Smartplug gen-1 application
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

//*****************************************************************************
//
//! \addtogroup uart_demo
//! @{
//
//*****************************************************************************
//*****************************************************************************
//
// uart_logger.c - API(s) for UARTLogger
//
//*****************************************************************************
#include "datatypes.h"
#include "simplelink.h"
#include <hw_types.h>
#include <hw_memmap.h>
#include <prcm.h>
#include <pin.h>
#include <uart.h>

#include <stdarg.h>
#include<stdlib.h>
#include <stdio.h>
#include "rom_map.h"
#include <uart_logger.h>
#include <osi.h>

extern volatile UINT8 g_NwpInitdone;
OsiLockObj_t  g_UartLockObj;

//*****************************************************************************
//
//! Initialisation
//!
//! This function
//!   1. Configures the UART to be used.
//!
//! \return none
//
//*****************************************************************************
void InitTerm()
{
  MAP_UARTConfigSetExpClk(CONSOLE,SYSCLK, UART_BAUD_RATE,
                                (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                                 UART_CONFIG_PAR_NONE));

  ClearTerm();
  Report("UART Initialized \n\r");
}

//*****************************************************************************
//
//! Outputs a character string to the console
//!
//! \param str is the pointer to the string to be printed
//!
//! This function
//!   1. prints the input string character by character on to the console.
//!
//! \return none
//
//*****************************************************************************
void Message(char *str)
{
    if(g_NwpInitdone)
    {
      osi_LockObjLock(&g_UartLockObj, SL_OS_WAIT_FOREVER);
    }

    while(*str!='\0')
    {
        MAP_UARTCharPut(CONSOLE,*str++);
    }

    if(g_NwpInitdone)
    {
      osi_LockObjUnlock(&g_UartLockObj);
    }
}

//*****************************************************************************
//
//! Clear the console window
//!
//! This function
//!   1. clears the console window.
//!
//! \return none
//
//*****************************************************************************
void ClearTerm()
{
  Message("\33[2J\r");
}

//*****************************************************************************
//
//! prints the formatted string on to the console
//!
//! \param format is a pointer to the character string specifying the format in
//!      the following arguments need to be interpreted.
//! \param [variable number of] arguments according to the format in the first
//!         parameters
//! This function
//!   1. prints the formatted error statement.
//!
//! \return none
//
//*****************************************************************************
void Report(char *pcFormat, ...)
{
    char *pcBuff;
    unsigned long iSize = 511;
    int iRet;
    char pcBuffer[512];
    //pcBuff=mem_Malloc(iSize);
    pcBuff = &pcBuffer[0];
    va_list list;
    while(1)
    {
        va_start(list,pcFormat);
        iRet = vsnprintf(pcBuff,iSize,pcFormat,list);
        va_end(list);
        if(iRet > -1 && iRet < iSize)
        {
            break;
        }
        else
        {
          return;
#if 0
          iSize*=2;
            if((pcTemp=realloc(pcBuff,iSize))==NULL)
            {
                Message("Could not reallocate memory\n\r");
                break;
            }
            else
            {
               pcBuff=pcTemp;
            }
#endif
       }
    }
    Message(pcBuff);
    //free(pcBuff);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

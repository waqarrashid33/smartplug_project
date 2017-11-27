//*****************************************************************************
// File: udmadrv.c
//
// Description: Micro DMA driver of metology module of Smartplug gen-1 application
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

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "interrupt.h"
#include "prcm.h"
#include "udma.h"
#include "udmadrv.h"
#include "debug.h"
#include "rom.h"
#include "rom_map.h"

//*****************************************************************************
//
//! Setup a uDMA transfer.
//!
//! This function will perform all the steps necessary to set up a DMA transfer.
//! in burst mode.
//! \return None.
//
//*****************************************************************************
void
UDMASetupTransfer(unsigned long ulChannel, unsigned long ulMode,
              unsigned long ulItemCount,
              unsigned long ulItemSize, unsigned long ulArbSize,
              void *pvSrcBuf, unsigned long ulSrcInc,
              void *pvDstBuf, unsigned long ulDstInc)
{
    MAP_uDMAChannelAssign(ulChannel);

    //
    // Set up the transfer.
    //
    MAP_uDMAChannelControlSet(ulChannel,
                              ulItemSize | ulSrcInc | ulDstInc | ulArbSize);

    MAP_uDMAChannelTransferSet(ulChannel, ulMode,
                               pvSrcBuf, pvDstBuf, ulItemCount);

    //
    // Set burst mode
    //
    MAP_uDMAChannelAttributeEnable(ulChannel, UDMA_ATTR_USEBURST);

    //
    // Enable the udma channel.
    //
    MAP_uDMAChannelEnable(ulChannel);

}




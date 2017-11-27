//*****************************************************************************
// File: metrology.c
//
// Description: Metrology Module of Smartplug gen-1 application
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
#include "stdio.h"
#include "math.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_adc.h"
#include "inc/hw_gprcm.h"
#include "inc/hw_udma.h"
#include "interrupt.h"
#include "adc.h"
#include "prcm.h"
#include "udmadrv.h"
#include "udma.h"
#include "uart.h"
#include "pin.h"
#include "rom.h"
#include "rom_map.h"
#include "debug.h"
#include "uart_logger.h"
#include "simplelink.h"
#include "pinmux.h"
#include "gpio_if.h"
#include "utils.h"
#include "cc3200.h"

#define extern
#include "metrology.h"
#undef extern

#include "exosite_meta.h"
#include "smartplug_task.h"

extern t_SmartPlugNvmmFile SmartPlugNvmmFile;
extern SmartPlugGpioConfig_t  SmartPlugGpioConfig;

#ifdef METROLOGY_DEBUG
short unsigned int bDmaDoneV, pingV, pongV ;
short unsigned int bDmaDoneI, pingI, pongI ;
#endif

//short unsigned int ArrayIndexV, BufferV[1280], BufferI[1280];

t_MetrologyDebugParams MetrologyDebugParams;

extern volatile UINT32 TimerMsCount;
UINT32 MipsVCh = 0, MipsICh = 0, MipsComm = 0;
volatile UINT8 g_MetrologyCalEn = 0;
volatile UINT8 g_MetroCalCount = 0;
volatile UINT8 g_MetrologyGainScalCalEn = 0;
float AvgRmsVoltage, AvgRmsCurrent, AvgRmsNoise;

void InitMetrologyModule( void )
{
  /* Init data pointers */
  pChannelVoltage         = &ChannelVoltage;
  pChannelCurrent         = &ChannelCurrent;
  pMutualVIData           = &MutualVIData;
  pSmartPlugMetrologyData = &SmartPlugMetrologyData;
  pMetrologyDataComm      = &MetrologyDataComm;

#if 0 //DKS for test
  /* Voltage Channel */
  pChannelVoltage->AdcChannelNum = ADC_CH_2;
  pChannelVoltage->DmaChannelNum = UDMA_CH16_ADC_CH2;

  /* Current Channel */
  pChannelCurrent->AdcChannelNum = ADC_CH_0;
  pChannelCurrent->DmaChannelNum = UDMA_CH14_ADC_CH0;
#else
  /* Voltage Channel */
 // pChannelVoltage->AdcChannelNum = ADC_CH_0;
  //ChannelVoltage->DmaChannelNum = UDMA_CH14_ADC_CH0;
 pChannelVoltage->AdcChannelNum = ADC_CH_3;
 pChannelVoltage->DmaChannelNum = UDMA_CH17_ADC_CH3;
  /* Current Channel */
  pChannelCurrent->AdcChannelNum = ADC_CH_2;
  pChannelCurrent->DmaChannelNum = UDMA_CH16_ADC_CH2;
#endif

  //
  // Get the interrupt number associted with the specified channel
  //
  pChannelVoltage->AdcIntNum = (pChannelVoltage->AdcChannelNum == ADC_CH_0)? INT_ADCCH0 :
                                (pChannelVoltage->AdcChannelNum == ADC_CH_1)? INT_ADCCH1 :
                                  (pChannelVoltage->AdcChannelNum == ADC_CH_2)? INT_ADCCH2 : INT_ADCCH3;
  //pChannelVoltage->AdcIntNum =INT_ADCCH3 ;
  pChannelCurrent->AdcIntNum = (pChannelCurrent->AdcChannelNum == ADC_CH_0)? INT_ADCCH0 :
                                (pChannelCurrent->AdcChannelNum == ADC_CH_1)? INT_ADCCH1 :
                                  (pChannelCurrent->AdcChannelNum == ADC_CH_2)? INT_ADCCH2 : INT_ADCCH3;
  //
  // Initialize global params at the start of smartplug
  //
  pMutualVIData->FirstZeroCrossDetected = 0;
  pMutualVIData->ZeroCrossingCount = 0;
  pMutualVIData->NumOfSamplesInTotalFullCycles = 0;
  pMutualVIData->TotalDecSamplesPerCycle = (float)0.0;
  pMutualVIData->TotalDecSamplesUsedForDcOffsetCal = (M_TOTAL_DMA_BLOCKS_PER_OFFSET_CAL*M_MIN_DECEM_SAMPLES_PER_DMA_BLOCK);
  pMutualVIData->AccPowerVal = 0;
  pMutualVIData->TotalPowerSamples = 0;

  pSmartPlugMetrologyData->InputACFrequency = (float)0.0;
  pSmartPlugMetrologyData->GrandTotalTrueEnergy = (double)0.0;
  pSmartPlugMetrologyData->GrandTotalApparentEnergy = (double)0.0;
  pSmartPlugMetrologyData->NumOfWhPulses = 0;

  pSmartPlugMetrologyData->DataValid = 0;
  pSmartPlugMetrologyData->DataValidCountInSecs = 0;//number of 1.024 secs

  /* Channel Voltage Initialization */
  pChannelVoltage->DmaBlockCount = 0;
  pChannelVoltage->ResidualSamplesLength = 0;
  pChannelVoltage->FirFilterStartInit = 0;
  pChannelVoltage->DcOffsetAccThrshReached = 0;
  pChannelVoltage->DcOffsetCount = 0;
  pChannelVoltage->DcOffsetAccVal = 0;
  pChannelVoltage->DcOffset1SecMoAvAccThrshReached = 0;
  pChannelVoltage->DcOffset1SecMoAvCount = 0;
  pChannelVoltage->DcOffsetMoAvAccVal = 0;
  pChannelVoltage->DcOffset1SecCalInit = 0;
  pChannelVoltage->TotalSumSquareSamples = 0;
  pChannelVoltage->AccSumSquareVal = 0;

  /* Channel Current Initialization */
  pChannelCurrent->DmaBlockCount = 0;

  pChannelCurrent->ResidualSamplesLength = 0;
  pChannelCurrent->FirFilterStartInit = 0;
  pChannelCurrent->DcOffsetAccThrshReached = 0;
  pChannelCurrent->DcOffsetCount = 0;
  pChannelCurrent->DcOffsetAccVal = 0;
  pChannelCurrent->DcOffset1SecMoAvAccThrshReached = 0;
  pChannelCurrent->DcOffset1SecMoAvCount = 0;
  pChannelCurrent->DcOffsetMoAvAccVal = 0;
  pChannelCurrent->DcOffset1SecCalInit = 0;
  pChannelCurrent->TotalSumSquareSamples = 0;
  pChannelCurrent->AccSumSquareVal = 0;

#ifdef METROLOGY_DEBUG
  bDmaDoneV = 0;
  bDmaDoneI = 0;
#endif

  memset((void *)pMetrologyDataComm->EnergyUptoLastHour, 0, 4);
  memset((void *)pMetrologyDataComm->AveragePower, 0, 4);
  memset((void *)pMetrologyDataComm->UpdateTime, 0, 4);
  memset((void *)pMetrologyDataComm->UpdateTimeServer, 0, 4);
  memset((void *)pMetrologyDataComm->APUpdateCount, 0, 4);
  pMetrologyDataComm->AP24HrCount = 0;
  pMetrologyDataComm->AP24HrRollover = 0;

  //
  // Initialize ADC & DMA for metrology module
  //
  InitMetrologyAdcDma();//DKS test
}

void InitMetrologyAdcDma( void )
{
  UINT16 Status;

#ifdef CC3200_ES_1_2_1
  //
  // Enable ADC clocks.###IMPORTANT###Need to be removed for PG 1.32
  //
  HWREG(GPRCM_BASE + GPRCM_O_ADC_CLK_CONFIG) = 0x00000043;
  HWREG(ADC_BASE + ADC_O_ADC_CTRL) = 0x00000004;
  HWREG(ADC_BASE + ADC_O_ADC_SPARE0) = 0x00000100;
  HWREG(ADC_BASE + ADC_O_ADC_SPARE1) = 0x0355AA00;
#endif

  //
  // Pinmux for the selected ADC input pin
  //
  PinTypeADC(PIN_57,0xFF); //ADC ch 0
  PinTypeADC(PIN_59,0xFF); //ADC ch 2

  //
  // Setup a Transfer of Voltage ADC ch
  //

  UDMASetupTransfer(pChannelVoltage->DmaChannelNum|UDMA_PRI_SELECT, UDMA_MODE_PINGPONG,
              M_TOTAL_DMA_SAMPLES,
              UDMA_SIZE_32, UDMA_ARB_1,
              (void *)(ADC_BASE_FIFO_DATA+pChannelVoltage->AdcChannelNum), UDMA_SRC_INC_NONE,
              (void *)&(pChannelVoltage->DmaDataDumpPing[0]), UDMA_DST_INC_32);

  UDMASetupTransfer(pChannelVoltage->DmaChannelNum|UDMA_ALT_SELECT, UDMA_MODE_PINGPONG,
              M_TOTAL_DMA_SAMPLES,
              UDMA_SIZE_32, UDMA_ARB_1,
              (void *)(ADC_BASE_FIFO_DATA+pChannelVoltage->AdcChannelNum), UDMA_SRC_INC_NONE,
              (void *)&(pChannelVoltage->DmaDataDumpPong[0]), UDMA_DST_INC_32);

  //
  // Enable UDMA
  //
  ADCDMAEnable(ADC_BASE, pChannelVoltage->AdcChannelNum);

  //
  // Setup a Transfer of Current ADC ch
  //
  UDMASetupTransfer(pChannelCurrent->DmaChannelNum|UDMA_PRI_SELECT, UDMA_MODE_PINGPONG,
              M_TOTAL_DMA_SAMPLES,
              UDMA_SIZE_32, UDMA_ARB_1,
              (void *)(ADC_BASE_FIFO_DATA+pChannelCurrent->AdcChannelNum), UDMA_SRC_INC_NONE,
              (void *)&(pChannelCurrent->DmaDataDumpPing[0]), UDMA_DST_INC_32);

  UDMASetupTransfer(pChannelCurrent->DmaChannelNum|UDMA_ALT_SELECT, UDMA_MODE_PINGPONG,
              M_TOTAL_DMA_SAMPLES,
              UDMA_SIZE_32, UDMA_ARB_1,
              (void *)(ADC_BASE_FIFO_DATA+pChannelCurrent->AdcChannelNum), UDMA_SRC_INC_NONE,
              (void *)&(pChannelCurrent->DmaDataDumpPong[0]), UDMA_DST_INC_32);

  //
  // Enable UDMA
  //
  ADCDMAEnable(ADC_BASE, pChannelCurrent->AdcChannelNum);

  //
  // Setup Voltage ADC
  //

  #ifdef SL_PLATFORM_MULTI_THREADED
      osi_InterruptRegister(pChannelVoltage->AdcIntNum, (P_OSI_INTR_ENTRY)ADCIntHandlerChV, INT_PRIORITY_LVL_2);
  #else
      //
      // Set the priority
      //
      IntPrioritySet(pChannelVoltage->AdcIntNum, INT_PRIORITY_LVL_2);

      IntPendClear(pChannelVoltage->AdcIntNum);

      ADCIntRegister(ADC_BASE, pChannelVoltage->AdcChannelNum,ADCIntHandlerChV);
  #endif

  Status = ADCIntStatus(ADC_BASE, pChannelVoltage->AdcChannelNum);
  ADCIntClear(ADC_BASE, pChannelVoltage->AdcChannelNum,Status|ADC_DMA_DONE);
  ADCIntEnable(ADC_BASE, pChannelVoltage->AdcChannelNum,ADC_DMA_DONE);
  ADCChannelEnable(ADC_BASE, pChannelVoltage->AdcChannelNum);

  //
  // Setup Current ADC
  //
  #ifdef SL_PLATFORM_MULTI_THREADED
      osi_InterruptRegister(pChannelCurrent->AdcIntNum, (P_OSI_INTR_ENTRY)ADCIntHandlerChI, INT_PRIORITY_LVL_2);
  #else
      //
      // Set the priority
      //
      IntPrioritySet(pChannelCurrent->AdcIntNum, INT_PRIORITY_LVL_2);

      IntPendClear(pChannelCurrent->AdcIntNum);

      ADCIntRegister(ADC_BASE, pChannelCurrent->AdcChannelNum,ADCIntHandlerChI);
  #endif

  Status = ADCIntStatus(ADC_BASE, pChannelVoltage->AdcChannelNum);
  ADCIntClear(ADC_BASE, pChannelCurrent->AdcChannelNum,Status|ADC_DMA_DONE);
  ADCIntEnable(ADC_BASE, pChannelCurrent->AdcChannelNum,ADC_DMA_DONE);
  ADCChannelEnable(ADC_BASE, pChannelCurrent->AdcChannelNum);

  //
  // Setup a Software interrupt handler for processing the metro data
  //
  #ifdef SL_PLATFORM_MULTI_THREADED
      osi_InterruptRegister(METROLOGY_SFT_INT_NUM, (P_OSI_INTR_ENTRY)ComputeSmartPlugMetrology, INT_PRIORITY_LVL_3);
  #else
      SoftwareIntRegister(METROLOGY_SFT_INT_NUM, INT_PRIORITY_LVL_3, ComputeSmartPlugMetrology);//1.024 sec interrupt
  #endif

  //
  // Enable ADC
  //
  ADCTimerConfig(ADC_BASE,2^17);
  ADCTimerEnable(ADC_BASE);
  ADCEnable(ADC_BASE);

}

void ADCIntHandlerChV(void)
{
  unsigned long ulChannelStructIndex, ulMode, ulControl;
  tDMAControlTable *pControlTable;
  unsigned long *pDataDumpBuff = NULL;
  short unsigned int ChannelOffset;
  UINT16 Status;
  UINT32 TimerMsCountInit;

  TimerMsCountInit = TimerMsCount;

#ifdef METROLOGY_DEBUG
  bDmaDoneV++;
#endif

  Status = ADCIntStatus(ADC_BASE, pChannelVoltage->AdcChannelNum);
  ADCIntClear(ADC_BASE, pChannelVoltage->AdcChannelNum,Status|ADC_DMA_DONE);
  //
  // Check the DMA control table to see if the ping-pong "A" transfer is
  // complete.  The "A" transfer uses receive buffer "A", and the primary
  // control structure.
  //
  ulMode = MAP_uDMAChannelModeGet(pChannelVoltage->DmaChannelNum | UDMA_PRI_SELECT);
  //
  // If the primary control structure indicates stop, that means the "A"
  // receive buffer is done.  The uDMA controller should still be receiving
  // data into the "B" buffer.
  //
  if(ulMode == UDMA_MODE_STOP)
  {
    ulChannelStructIndex = pChannelVoltage->DmaChannelNum | UDMA_PRI_SELECT;
    #ifdef METROLOGY_DEBUG
    pingV++;
    #endif

    pDataDumpBuff = &(pChannelVoltage->DmaDataDumpPing[0]);
  }
  else
  {
    //
    // Check the DMA control table to see if the ping-pong "A" transfer is
    // complete.  The "A" transfer uses receive buffer "A", and the primary
    // control structure.
    //
    ulMode = MAP_uDMAChannelModeGet(pChannelVoltage->DmaChannelNum | UDMA_ALT_SELECT);
    //
    // If the primary control structure indicates stop, that means the "A"
    // receive buffer is done.  The uDMA controller should still be receiving
    // data into the "B" buffer.
    //
    if(ulMode == UDMA_MODE_STOP)
    {
      ulChannelStructIndex = pChannelVoltage->DmaChannelNum | UDMA_ALT_SELECT;
      #ifdef METROLOGY_DEBUG
      pongV++;
      #endif

      pDataDumpBuff = &(pChannelVoltage->DmaDataDumpPong[0]);
    }
  }

  if(pDataDumpBuff != NULL)
  {
    pChannelVoltage->DmaBlockCount++;

    /* Decimate Input samples */
    DecimateAdcSamples(pDataDumpBuff, &pChannelVoltage->ResidualSumInDmaBlock, M_TOTAL_DMA_SAMPLES, M_DECIMATION_RATIO, \
                       &pChannelVoltage->ResidualSamplesLength, &pChannelVoltage->DecimatedSamples[0], &pChannelVoltage->DecimatedTotalSamples);

    /* first time populate some value in past input */
    if(0 == pChannelVoltage->FirFilterStartInit)
    {
      pChannelVoltage->FirFilterStartInit = 1;
      pChannelVoltage->FirZSamples[0] = pChannelVoltage->DecimatedSamples[0];
      pChannelVoltage->FirZSamples[1] = pChannelVoltage->DecimatedSamples[0];
      pChannelVoltage->FirZSamples[2] = pChannelVoltage->DecimatedSamples[0];
      pChannelVoltage->FirZSamples[3] = pChannelVoltage->DecimatedSamples[0];
      pChannelVoltage->FirZSamples[4] = pChannelVoltage->DecimatedSamples[0];
    }

    /* Filter input sample with cut off frquency 300Hz & 5th order FIR filter */
    FirFilter(&pChannelVoltage->DecimatedSamples[0], pChannelVoltage->DecimatedTotalSamples, &pChannelVoltage->FilteredSamples[0], &pChannelVoltage->FirZSamples[0]);

    /* Convert ADC filtered samples into absolute analog value */
    ConvertSampleCodeWordsToValues(&pChannelVoltage->FilteredSamples[0], pChannelVoltage->DecimatedTotalSamples, &pChannelVoltage->FilteredValues[0]);//scaled by 12 bits

    /* Compute offset in ADC channel */
    ChannelOffset = ComputeChannelOffset(&pChannelVoltage->FilteredValues[0], pChannelVoltage );//scaled by 12 bits

    /* First 1sec use instantaneous offset value and after that use previous sec offset value */
    if(0 == pChannelVoltage->DcOffset1SecCalInit)
    {
      pChannelVoltage->DcChannelOffset = ChannelOffset;
    }

    /* Compute sum square */
    ComputeSumSquare(&pChannelVoltage->FilteredValues[0], pChannelVoltage->DecimatedTotalSamples, \
                     &pChannelVoltage->AccSumSquareVal, &pChannelVoltage->TotalSumSquareSamples, pChannelVoltage->DcChannelOffset);

    /* Compute Energy if both V & I samples match */
    if(pChannelVoltage->DmaBlockCount == pChannelCurrent->DmaBlockCount)
    {
      ComputeTrueEnergy(&pChannelVoltage->FilteredValues[0], &pChannelCurrent->FilteredValues[0], pChannelVoltage->DecimatedTotalSamples,\
          &pMutualVIData->AccPowerVal, &pMutualVIData->TotalPowerSamples, pChannelVoltage->DcChannelOffset, pChannelCurrent->DcChannelOffset);
    }

    /* Compute zero crossing for frequency compuation - perform it only for voltage */
    ComputeZeroCrossing(&pChannelVoltage->FilteredValues[0], pChannelVoltage->DecimatedTotalSamples);

    /* Compute Raw metrology params every sec */
    if(M_TOTAL_DMA_BLOCKS_PER_SEC == pChannelVoltage->DmaBlockCount)
    {
      /* Compute avg channel offset */
      if(1 == pChannelVoltage->DcOffset1SecMoAvAccThrshReached)
      {
        pChannelVoltage->DcOffsetMoAvAccVal -= pChannelVoltage->DcOffset1SecMoAvArray[pChannelVoltage->DcOffset1SecMoAvCount];
        pChannelVoltage->DcOffsetMoAvAccVal += ChannelOffset;
        pChannelVoltage->DcOffset1SecMoAvArray[pChannelVoltage->DcOffset1SecMoAvCount++] = ChannelOffset;

        ChannelOffset = pChannelVoltage->DcOffsetMoAvAccVal/M_TOTAL_SECS_PER_OFFSET_CAL;

        if(pChannelVoltage->DcOffset1SecMoAvCount >= M_TOTAL_SECS_PER_OFFSET_CAL)
        {
          pChannelVoltage->DcOffset1SecMoAvCount = 0;
        }
      }
      else
      {
        pChannelVoltage->DcOffset1SecMoAvArray[pChannelVoltage->DcOffset1SecMoAvCount++] = ChannelOffset;
        pChannelVoltage->DcOffsetMoAvAccVal += ChannelOffset;
        ChannelOffset = pChannelVoltage->DcOffsetMoAvAccVal/pChannelVoltage->DcOffset1SecMoAvCount;

        if(pChannelVoltage->DcOffset1SecMoAvCount >= M_TOTAL_SECS_PER_OFFSET_CAL)
        {
          pChannelVoltage->DcOffset1SecMoAvCount = 0;
          pChannelVoltage->DcOffset1SecMoAvAccThrshReached = 1;
        }
      }

      pChannelVoltage->DcOffsetAccThrshReached = 0;
      pChannelVoltage->DcOffsetCount = 0;
      pChannelVoltage->DcOffsetAccVal = 0;
      pChannelVoltage->DcChannelOffsetLast1Sec = ChannelOffset;
      pChannelVoltage->DcOffset1SecCalInit = 1;

      /* zero crossing details */
      pMutualVIData->NumOfSamplesIn1SecFullCycles = pMutualVIData->NumOfSamplesInTotalFullCycles;
      pMutualVIData->NumOfZeroCrossingIn1Sec = pMutualVIData->ZeroCrossingCount;

      pMutualVIData->NumOfSamplesInTotalFullCycles = 0;
      pMutualVIData->ZeroCrossingCount = 0;
      pMutualVIData->FirstZeroCrossDetected = 0;

      /* Raw rms voltage */
      pChannelVoltage->AccSumSquare1SecVal = pChannelVoltage->AccSumSquareVal/pChannelVoltage->TotalSumSquareSamples;//scaled by 20 bits

      pChannelVoltage->AccSumSquareVal = 0;
      pChannelVoltage->TotalSumSquareSamples = 0;

      /* Compute Raw True Energy per sec if both V & I samples match */
      if(pChannelVoltage->DmaBlockCount == pChannelCurrent->DmaBlockCount)
      {
        /* Raw True Energy */
        if(pMutualVIData->AccPowerVal < 0)
        {
          pMutualVIData->AccPowerValIn1Sec = 0;
        }
        else
        {
          pMutualVIData->AccPowerValIn1Sec = (unsigned long)(pMutualVIData->AccPowerVal/pMutualVIData->TotalPowerSamples);//scaled by 20 bits
        }

        pMutualVIData->AccPowerVal = 0;
        pMutualVIData->TotalPowerSamples = 0;

        pChannelVoltage->DcChannelOffset = pChannelVoltage->DcChannelOffsetLast1Sec;
        pChannelCurrent->DcChannelOffset = pChannelCurrent->DcChannelOffsetLast1Sec;

        pChannelCurrent->DmaBlockCount = 0;
        pChannelVoltage->DmaBlockCount = 0;

        /* trigger sft interrupt Indicating that Metrology data is valid */
        SoftwareIntTrigger(METROLOGY_SFT_INT_NUM);
      }
    }
    //
    // Set up the next transfer for the "A" buffer, using the primary
    // control structure.  When the ongoing receive into the "B" buffer is
    // done, the uDMA controller will switch back to this one.
    //
    ulChannelStructIndex &= 0x3f;
     //
    // Get the base address of the control table.
    //
    pControlTable = (tDMAControlTable *)HWREG(UDMA_BASE + UDMA_O_CTLBASE);
    //
    // Get the current control word value and mask off the mode and size
    // fields.
    //
    ulControl = (pControlTable[ulChannelStructIndex].ulControl &
                 ~(UDMA_CHCTL_XFERSIZE_M | UDMA_CHCTL_XFERMODE_M));

    //
    // Set the transfer size and mode in the control word (but don't write the
    // control word yet as it could kick off a transfer).
    //
    ulControl |= UDMA_MODE_PINGPONG | ((M_TOTAL_DMA_SAMPLES - 1) << 4);

    //
    // Write the new control word value.
    //
    pControlTable[ulChannelStructIndex].ulControl = ulControl;

    #if 0
    ArrayIndexV = 0;
    printf("\n\n\n\r");
    while(ArrayIndexV < M_TOTAL_DMA_SAMPLES)
    {
      printf("%d\n",(pDataDumpBuff[ArrayIndexV++]>> 2) & 0xFFF);
    }
    #endif
  }

#ifdef METROLOGY_DEBUG
#if 0
  printf("\n\n\n\r");

  ArrayIndexV = 0;
  while(ArrayIndexV < DecimatedTotalSamplesV)
  {
    printf("%d\n",DecimatedSamplesV[ArrayIndexV++] );
  }

  printf("\n\n\n\r");

  ArrayIndexV = 0;
  while(ArrayIndexV < DecimatedTotalSamplesV)
  {
    printf("%d\n",FilteredSamplesV[ArrayIndexV++]);
  }

  printf("\n\n\n\r");

#if 0
  ArrayIndexV = 0;
  while(ArrayIndexV < 5)
  {
    printf("%d\n",FirZSamplesV[ArrayIndexV++]);
  }
#endif
  printf("\n\n\n\r");

  ArrayIndexV = 0;
  while(ArrayIndexV < DecimatedTotalSamplesV)
  {
    printf("%d\n",FilteredValuesV[ArrayIndexV++]);
  }
#endif

#if 0
  printf("\n\n\n\r");

  ArrayIndexV = 0;
  while(ArrayIndexV < DecimatedTotalSamplesV)
  {
    printf("%d\n",FilteredValuesV[ArrayIndexV++]);
  }

  printf("\n\n\r %d\n", ChannelOffsetV);
#endif

#if 0
  if(2 == bDmaDoneV)
  {
    unsigned int DestIndex = 0;

    ADCDMADisable(ADC_BASE, pChannelVoltage->AdcChannelNum);
    ADCIntDisable(ADC_BASE, pChannelVoltage->AdcChannelNum,ADC_DMA_DONE);

    for(ArrayIndexV = 0; ArrayIndexV < 640; ArrayIndexV++)
    {
      BufferV[DestIndex]=      (DataDumpPingChV[ArrayIndexV] >> 2) & 0xFFF;
      BufferV[DestIndex+640]=  (DataDumpPongChV[ArrayIndexV] >> 2) & 0xFFF;
      BufferI[DestIndex]=      (DataDumpPingChI[ArrayIndexV] >> 2) & 0xFFF;
      BufferI[DestIndex+640]=  (DataDumpPongChI[ArrayIndexV] >> 2) & 0xFFF;
      DestIndex++;
    }

    ArrayIndexV = 0;//bp
  }
#endif
#endif

  if((TimerMsCount - TimerMsCountInit) > MipsVCh)
    MipsVCh = TimerMsCount - TimerMsCountInit;
}

void ADCIntHandlerChI(void)
{
  unsigned long ulChannelStructIndex, ulMode, ulControl;
  tDMAControlTable *pControlTable;
  unsigned long *pDataDumpBuff = NULL;
  short unsigned int ChannelOffset;
  UINT16 Status;
  UINT32 TimerMsCountInit;

  TimerMsCountInit = TimerMsCount;

  #ifdef METROLOGY_DEBUG
  bDmaDoneI++;
  #endif

  Status = ADCIntStatus(ADC_BASE, pChannelVoltage->AdcChannelNum);
  ADCIntClear(ADC_BASE, pChannelCurrent->AdcChannelNum,Status|ADC_DMA_DONE);

  //
  // Check the DMA control table to see if the ping-pong "A" transfer is
  // complete.  The "A" transfer uses receive buffer "A", and the primary
  // control structure.
  //
  ulMode = MAP_uDMAChannelModeGet(pChannelCurrent->DmaChannelNum | UDMA_PRI_SELECT);
  //
  // If the primary control structure indicates stop, that means the "A"
  // receive buffer is done.  The uDMA controller should still be receiving
  // data into the "B" buffer.
  //
  if(ulMode == UDMA_MODE_STOP)
  {
    ulChannelStructIndex = pChannelCurrent->DmaChannelNum | UDMA_PRI_SELECT;
    #ifdef METROLOGY_DEBUG
    pingI++;
    #endif

    pDataDumpBuff = &pChannelCurrent->DmaDataDumpPing[0];
    //pDataDumpBuff = (unsigned long*) 54545;

  }
  else
  {
    //
    // Check the DMA control table to see if the ping-pong "A" transfer is
    // complete.  The "A" transfer uses receive buffer "A", and the primary
    // control structure.
    //
    ulMode = MAP_uDMAChannelModeGet(pChannelCurrent->DmaChannelNum | UDMA_ALT_SELECT);
    //
    // If the primary control structure indicates stop, that means the "A"
    // receive buffer is done.  The uDMA controller should still be receiving
    // data into the "B" buffer.
    //
    if(ulMode == UDMA_MODE_STOP)
    {
      ulChannelStructIndex = pChannelCurrent->DmaChannelNum | UDMA_ALT_SELECT;
      #ifdef METROLOGY_DEBUG
      pongI++;
      #endif

      pDataDumpBuff = &pChannelCurrent->DmaDataDumpPong[0];
      //pDataDumpBuff = (unsigned long*)1233;

    }
  }

  if(pDataDumpBuff != NULL)
  {
    pChannelCurrent->DmaBlockCount++;

    /* Decimate Input samples */
    DecimateAdcSamples(pDataDumpBuff, &pChannelCurrent->ResidualSumInDmaBlock, M_TOTAL_DMA_SAMPLES, M_DECIMATION_RATIO, \
                       &pChannelCurrent->ResidualSamplesLength, &pChannelCurrent->DecimatedSamples[0], &pChannelCurrent->DecimatedTotalSamples);

    /* first time populate some value in past input */
    if(0 == pChannelCurrent->FirFilterStartInit)
    {
      pChannelCurrent->FirFilterStartInit = 1;
      pChannelCurrent->FirZSamples[0] = pChannelCurrent->DecimatedSamples[0];
      pChannelCurrent->FirZSamples[1] = pChannelCurrent->DecimatedSamples[0];
      pChannelCurrent->FirZSamples[2] = pChannelCurrent->DecimatedSamples[0];
      pChannelCurrent->FirZSamples[3] = pChannelCurrent->DecimatedSamples[0];
      pChannelCurrent->FirZSamples[4] = pChannelCurrent->DecimatedSamples[0];
    }

    /* Filter input sample with cut off frquency 300Hz & 5th order FIR filter */
    FirFilter(&pChannelCurrent->DecimatedSamples[0], pChannelCurrent->DecimatedTotalSamples, &pChannelCurrent->FilteredSamples[0], &pChannelCurrent->FirZSamples[0]);

    /* Convert ADC filtered samples into absolute analog value */
    ConvertSampleCodeWordsToValues(&pChannelCurrent->FilteredSamples[0], pChannelCurrent->DecimatedTotalSamples, &pChannelCurrent->FilteredValues[0]);//scaled by 12 bits

    /* Compute offset in ADC channel */
    ChannelOffset = ComputeChannelOffset(&pChannelCurrent->FilteredValues[0], pChannelCurrent);//scaled by 12 bits

    /* First 1sec use instantaneous offset value and after that use previous sec offset value */
    if(0 == pChannelCurrent->DcOffset1SecCalInit)
    {
      pChannelCurrent->DcChannelOffset = ChannelOffset;
    }

    /* Compute sum square */
    ComputeSumSquare(&pChannelCurrent->FilteredValues[0], pChannelCurrent->DecimatedTotalSamples, \
            &pChannelCurrent->AccSumSquareVal, &pChannelCurrent->TotalSumSquareSamples, pChannelCurrent->DcChannelOffset);

    /* Compute True Energy if both V & I samples match */
    if(pChannelVoltage->DmaBlockCount == pChannelCurrent->DmaBlockCount)
    {
      ComputeTrueEnergy(&pChannelVoltage->FilteredValues[0], &pChannelCurrent->FilteredValues[0], pChannelVoltage->DecimatedTotalSamples,\
          &pMutualVIData->AccPowerVal, &pMutualVIData->TotalPowerSamples, pChannelVoltage->DcChannelOffset, pChannelCurrent->DcChannelOffset);
    }

    /* Compute Raw metrology params every sec */
    if(M_TOTAL_DMA_BLOCKS_PER_SEC == pChannelCurrent->DmaBlockCount)
    {
      /* Compute avg channel offset */
      if(1 == pChannelCurrent->DcOffset1SecMoAvAccThrshReached)
      {
        pChannelCurrent->DcOffsetMoAvAccVal -= pChannelCurrent->DcOffset1SecMoAvArray[pChannelCurrent->DcOffset1SecMoAvCount];
        pChannelCurrent->DcOffsetMoAvAccVal += ChannelOffset;
        pChannelCurrent->DcOffset1SecMoAvArray[pChannelCurrent->DcOffset1SecMoAvCount++] = ChannelOffset;

        ChannelOffset = pChannelCurrent->DcOffsetMoAvAccVal/M_TOTAL_SECS_PER_OFFSET_CAL;

        if(pChannelCurrent->DcOffset1SecMoAvCount >= M_TOTAL_SECS_PER_OFFSET_CAL)
        {
          pChannelCurrent->DcOffset1SecMoAvCount = 0;
        }
      }
      else
      {
        pChannelCurrent->DcOffset1SecMoAvArray[pChannelCurrent->DcOffset1SecMoAvCount++] = ChannelOffset;
        pChannelCurrent->DcOffsetMoAvAccVal += ChannelOffset;
        ChannelOffset = pChannelCurrent->DcOffsetMoAvAccVal/pChannelCurrent->DcOffset1SecMoAvCount;

        if(pChannelCurrent->DcOffset1SecMoAvCount >= M_TOTAL_SECS_PER_OFFSET_CAL)
        {
          pChannelCurrent->DcOffset1SecMoAvCount = 0;
          pChannelCurrent->DcOffset1SecMoAvAccThrshReached = 1;
        }
      }

      pChannelCurrent->DcOffsetAccThrshReached = 0;
      pChannelCurrent->DcOffsetCount = 0;
      pChannelCurrent->DcOffsetAccVal = 0;
      pChannelCurrent->DcChannelOffsetLast1Sec = ChannelOffset;
      pChannelCurrent->DcOffset1SecCalInit = 1;

      /* Raw Rms current */
      pChannelCurrent->AccSumSquare1SecVal = pChannelCurrent->AccSumSquareVal/pChannelCurrent->TotalSumSquareSamples;//scaled by 20 bits

      pChannelCurrent->AccSumSquareVal = 0;
      pChannelCurrent->TotalSumSquareSamples = 0;

      /* Compute raw True Energy per sec if both V & I samples match */
      if(pChannelVoltage->DmaBlockCount == pChannelCurrent->DmaBlockCount)
      {
        /* raw true energy */
        if(pMutualVIData->AccPowerVal < 0)
        {
          pMutualVIData->AccPowerValIn1Sec = 0;
        }
        else
        {
          pMutualVIData->AccPowerValIn1Sec = (unsigned long)(pMutualVIData->AccPowerVal/pMutualVIData->TotalPowerSamples);//scaled by 20 bits
        }

        pMutualVIData->AccPowerVal = 0;
        pMutualVIData->TotalPowerSamples = 0;

        pChannelVoltage->DcChannelOffset = pChannelVoltage->DcChannelOffsetLast1Sec;
        pChannelCurrent->DcChannelOffset = pChannelCurrent->DcChannelOffsetLast1Sec;

        pChannelCurrent->DmaBlockCount = 0;
        pChannelVoltage->DmaBlockCount = 0;

        /* trigger sft interrupt Indicating that Metrology data is valid */
        SoftwareIntTrigger(METROLOGY_SFT_INT_NUM);
      }
    }
    //
    // Set up the next transfer for the "A" buffer, using the primary
    // control structure.  When the ongoing receive into the "B" buffer is
    // done, the uDMA controller will switch back to this one.
    //
    ulChannelStructIndex &= 0x3f;
     //
    // Get the base address of the control table.
    //
    pControlTable = (tDMAControlTable *)HWREG(UDMA_BASE + UDMA_O_CTLBASE);
    //
    // Get the current control word value and mask off the mode and size
    // fields.
    //
    ulControl = (pControlTable[ulChannelStructIndex].ulControl &
                 ~(UDMA_CHCTL_XFERSIZE_M | UDMA_CHCTL_XFERMODE_M));

    //
    // Set the transfer size and mode in the control word (but don't write the
    // control word yet as it could kick off a transfer).
    //
    ulControl |= UDMA_MODE_PINGPONG | ((M_TOTAL_DMA_SAMPLES - 1) << 4);

    //
    // Write the new control word value.
    //
    pControlTable[ulChannelStructIndex].ulControl = ulControl;

    #if 0
    printf("\n\n\n\r");
    ArrayIndexV = 0;
    while(ArrayIndexV < M_TOTAL_DMA_SAMPLES)
    {
      printf("%d\n",(pDataDumpBuff[ArrayIndexV++]>> 2) & 0xFFF);
    }
    #endif
  }

#ifdef METROLOGY_DEBUG
#if 0
  printf("\n\n\n\r");

  ArrayIndexI = 0;
  while(ArrayIndexI < DecimatedTotalSamplesI)
  {
    printf("%d\n",DecimatedSamplesI[ArrayIndexI++] );
  }

  printf("\n\n\n\r");

  ArrayIndexI = 0;
  while(ArrayIndexI < DecimatedTotalSamplesI)
  {
    printf("%d\n",FilteredSamplesI[ArrayIndexI++]);
  }

#if 0
  printf("\n\n\n\r");

  ArrayIndexI = 0;
  while(ArrayIndexI < 5)
  {
    printf("%d\n",FirZSamplesI[ArrayIndexI++]);
  }
#endif

#endif

#if 0
  printf("\n\n\n\r");

  ArrayIndexI = 0;
  while(ArrayIndexI < DecimatedTotalSamplesI)
  {
    printf("%d\n",FilteredValuesI[ArrayIndexI++]);
  }

  printf("\n\n\r %d\n", ChannelOffsetI);
#endif

#if 0
  if(2 == bDmaDoneI)
  {
    ADCDMADisable(ADC_BASE, pChannelCurrent->AdcChannelNum);
    ADCIntDisable(ADC_BASE, pChannelCurrent->AdcChannelNum,ADC_DMA_DONE);
  }
#endif
#endif

  if((TimerMsCount - TimerMsCountInit) > MipsICh)
    MipsICh = TimerMsCount - TimerMsCountInit;
}

void DecimateAdcSamples(unsigned long *pDataDumpBuf, unsigned long *pResidualVal, \
                        short unsigned int TotalInputSamples, short unsigned int DecimationRatio, \
                        short unsigned int *pResidualLength, short unsigned int *pDecimatedSamples,\
                        short unsigned int *pTotalDecimatedSamples)
{
  short unsigned int DecimatedSamples = 0;
  unsigned long OutputVal = 0;
  short unsigned int InputValCount = 0, index = 0;

  /* Value of residual length can not be more than 63 */
  if(*pResidualLength != 0)
  {
    OutputVal = *pResidualVal;
    InputValCount = *pResidualLength;
    *pResidualLength = 0;
  }

  while(index < TotalInputSamples)
  {
    OutputVal += (pDataDumpBuf[index] >> 2) & 0xFFF;
    InputValCount++;
    index++;

    if(InputValCount == M_DECIMATION_RATIO)
    {
      InputValCount = 0;

      OutputVal = (OutputVal + (1 << 2)) >> 3; //decimate by 3 bits

      pDecimatedSamples[DecimatedSamples] = OutputVal;

      OutputVal = 0;
      DecimatedSamples++;
    }
  }

  if(InputValCount != 0)
  {
    *pResidualLength = InputValCount;
    *pResidualVal = OutputVal;
  }

  *pTotalDecimatedSamples = DecimatedSamples;
}

void FirFilter(short unsigned int *pDecimatedSamples, short unsigned int DecimatedTotalSamples,\
               short unsigned int *pFilteredSamples, short unsigned int *pFirZSamples)
{
  short unsigned int index;
  int filterdSample;

  for(index = 0; index < DecimatedTotalSamples; index++)
  {
    filterdSample = (int)pDecimatedSamples[index]*(int)FirCoeff[0] + (int)pFirZSamples[0]*(int)FirCoeff[1] +\
          (int)pFirZSamples[1]*(int)FirCoeff[2] + (int)pFirZSamples[2]*(int)FirCoeff[3] + (int)pFirZSamples[3]*(int)FirCoeff[4] +\
          (int)pFirZSamples[4]*(int)FirCoeff[5];
    pFilteredSamples[index] = (short unsigned int)((filterdSample + (1 << 11)) >> 12);//down scale by 12 bits

    pFirZSamples[4] = pFirZSamples[3];
    pFirZSamples[3] = pFirZSamples[2];
    pFirZSamples[2] = pFirZSamples[1];
    pFirZSamples[1] = pFirZSamples[0];
    pFirZSamples[0] = pDecimatedSamples[index];
  }
}

void ConvertSampleCodeWordsToValues(short unsigned int *pFilteredSamples, short unsigned int DecimatedTotalSamples,\
                                    short unsigned int *pFilteredValues)
{
  short unsigned int index;

  for(index = 0; index < DecimatedTotalSamples; index++)
  {
    pFilteredValues[index] = (short unsigned int)(((unsigned long)pFilteredSamples[index] *\
                    (unsigned long)M_CONVERT_CODEWORD_TO_VALUES)/(unsigned long)((1 << M_SAMPLE_CODEWORD_DECEMA_BITS) - 1));
  }
}

short unsigned int ComputeChannelOffset(short unsigned int *pFilteredValues, t_AdcDmaChParams *pChannelParams)
{
  short unsigned int index, ChOffsetVal = 0;
  unsigned long AccValue = 0;

  if(1 == pChannelParams->DcOffsetAccThrshReached)
  {
    ChOffsetVal = pChannelParams->DcOffsetAccVal/pChannelParams->DcOffsetCount;
    return ChOffsetVal;
  }

  for(index = 0; index < M_MIN_DECEM_SAMPLES_PER_DMA_BLOCK; index++)
  {
    AccValue += pFilteredValues[index];
    pChannelParams->DcOffsetCount++;

    if(pChannelParams->DcOffsetCount >= pMutualVIData->TotalDecSamplesUsedForDcOffsetCal)
    {
      pChannelParams->DcOffsetAccThrshReached = 1;
      break;
    }
  }

  pChannelParams->DcOffsetAccVal += AccValue;

  ChOffsetVal = pChannelParams->DcOffsetAccVal/pChannelParams->DcOffsetCount;

  return ChOffsetVal;//scaled by 12 bits
}

void ComputeSumSquare(short unsigned int *pFilteredValues, short unsigned int DecimatedTotalSamples,\
                      unsigned long *pAccSumSquareVal, short unsigned int *pTotalSumSquareSamples,\
                      short unsigned int ChannelOffset)
{
  short unsigned int index;
  int InputVal;

  for(index = 0; index < DecimatedTotalSamples; index++)
  {
    InputVal = pFilteredValues[index] - ChannelOffset;//scaled by 12 bits

    *pAccSumSquareVal += ((unsigned long)(InputVal*InputVal) >> 4);//scaled by 20 bits
  }

  *pTotalSumSquareSamples += DecimatedTotalSamples;
}

void ComputeTrueEnergy(short unsigned int *pFilteredValuesV, short unsigned int *pFilteredValuesI,\
                  short unsigned int DecimatedTotalSamples, signed long *pAccPowerVal,\
                  short unsigned int *pTotalPowerSamples, short unsigned int ChannelOffsetV, \
                  short unsigned int ChannelOffsetI)
{
  short unsigned int index;
  int InputValV, InputValI;

  for(index = 0; index < DecimatedTotalSamples; index++)
  {
    InputValV = pFilteredValuesV[index] - ChannelOffsetV;//scaled by 12 bits
    InputValI = pFilteredValuesI[index] - ChannelOffsetI;//scaled by 12 bits

#ifdef SMARTPLUG_HW
    *pAccPowerVal += ((signed long)((-InputValV)*(InputValI)) >> 4);//scaled by 20 bits //DKS current is 180 degree off in New Hw
#else
    *pAccPowerVal += ((signed long)(InputValV*InputValI) >> 4);//scaled by 20 bits
#endif
  }

  *pTotalPowerSamples += DecimatedTotalSamples;

}

void ComputeZeroCrossing(short unsigned int *pFilteredValues, short unsigned int DecimatedTotalSamples)
{
  static short unsigned int HighCycle = 0, LowCycle = 0, NumOfSamples = 0;
  short unsigned int index;
  int ValInput;

  for(index = 0; index < DecimatedTotalSamples; index++)
  {
    ValInput = pFilteredValues[index] - pChannelVoltage->DcChannelOffset;//scaled by 12 bits

    /* Discard samples till first zero cross detected */
    if(1 == pMutualVIData->FirstZeroCrossDetected)
    {
      /* Detect zero crossing and count number of samples */
      if(ValInput >= 0)
      {
        if(1 == LowCycle)
        {
          pMutualVIData->ZeroCrossingCount++;
          pMutualVIData->NumOfSamplesInTotalFullCycles += NumOfSamples;
          NumOfSamples = 0;
          LowCycle = 0;
        }
        HighCycle = 1;
      }
      else
      {
        if(1 == HighCycle)
        {
          pMutualVIData->ZeroCrossingCount++;
          pMutualVIData->NumOfSamplesInTotalFullCycles += NumOfSamples;
          NumOfSamples = 0;
          HighCycle = 0;
        }
        LowCycle = 1;
      }

      NumOfSamples++;

    }
    else
    {
      if(ValInput >= 0)
      {
        if(1 == LowCycle)
        {
          pMutualVIData->FirstZeroCrossDetected = 1;
          LowCycle = 0;
        }
        HighCycle = 1;
      }
      else
      {
        if(1 == HighCycle)
        {
          pMutualVIData->FirstZeroCrossDetected = 1;
          HighCycle = 0;
        }
        LowCycle = 1;
      }

      NumOfSamples = 1;//during the first zero cross detection first 1 sample belongs to other side of cycle
    }
  }
}

int UpdateStatusOfMetrologyModule(void)
{
  UINT32 InterruptMask;
#ifdef SMARTPLUG_HW
  UINT8 DataValid = 0;
#endif
  static UINT32 PrevMetroTime;
  static UINT32 PrevWhPulses;
  UINT32 CurrentMetroTime, CurrentWhPulses;
  INT32 DiffMetroTime, DiffWhPulses;

  /* Mask software interrupt which computes Metrology data before updating */
  InterruptMask = osi_EnterCritical();
#ifdef SMARTPLUG_HW
  DataValid        = pSmartPlugMetrologyData->DataValid;
#endif
  CurrentMetroTime = pSmartPlugMetrologyData->DataValidCountInSecs;
  CurrentWhPulses  = pSmartPlugMetrologyData->NumOfWhPulses;
  /* UnMask interrupt */
  osi_ExitCritical(InterruptMask);

  DiffWhPulses = CurrentWhPulses - PrevWhPulses;
  PrevWhPulses = CurrentWhPulses;

  DiffMetroTime = CurrentMetroTime - PrevMetroTime;
  PrevMetroTime = CurrentMetroTime;

  if(DiffWhPulses > 0)//rollover check not there
  {
#ifdef SMARTPLUG_HW
    //toggle Wh Pulse GPIO
    GPIO_IF_Set(SmartPlugGpioConfig.WhPulsePort, SmartPlugGpioConfig.WhPulsePad, 1);//High pulse
    DELAY(10);//10ms delay
    GPIO_IF_Set(SmartPlugGpioConfig.WhPulsePort, SmartPlugGpioConfig.WhPulsePad, 0);
#endif
  }

  if(DiffMetroTime <= 0)//rollover check not there
  {
    return -1;
  }
  else
  {
#ifdef SMARTPLUG_HW
    /* Check for validity flag */
    if(0 == DataValid)
    {
      return -2;
      //return 1;//DKS test remove
    }
    else
#endif
    {
      return 1;
    }
  }
}

int UpdateMetrologyDataForComm( void )
{
  UINT32 CommValue, InterruptMask;

#ifdef METROLOGY_DEBUG
  float ChannelOffset1SecVf, ChannelOffset1SecIf;
#endif

  /* Convert smartplug metrology data into communication format */

  /* Mask software interrupt which computes Metrology data before updating */
  InterruptMask = osi_EnterCritical();

  /* Update count in 1.024 secs interval */
  memcpy((void *)&pMetrologyDataComm->MetrologyCount[0],(const void *)&pSmartPlugMetrologyData->DataValidCountInSecs,4);

  /* Rms voltage in scle factor of 1000 */
  CommValue = (UINT32)(pSmartPlugMetrologyData->RmsVoltage * M_SCALE_FACTOR_3_DIGIT);
  memcpy((void *)&pMetrologyDataComm->RmsVoltage[0],(const void *)&CommValue,4);

  /* Rms current in scle factor of 1000 */
  CommValue = (UINT32)(pSmartPlugMetrologyData->RmsCurrent * M_SCALE_FACTOR_3_DIGIT);
  memcpy((void *)&pMetrologyDataComm->RmsCurrent[0],(const void *)&CommValue,4);

  /* True power in scle factor of 1000 */
  CommValue = (UINT32)(pSmartPlugMetrologyData->TrueEnergy * M_SCALE_FACTOR_3_DIGIT);
  memcpy((void *)&pMetrologyDataComm->TruePower[0],(const void *)&CommValue,4);

  /* Apparent power in scle factor of 1000 */
  CommValue = (UINT32)(pSmartPlugMetrologyData->ApparentEnergy * M_SCALE_FACTOR_3_DIGIT);
  memcpy((void *)&pMetrologyDataComm->ApparentPower[0],(const void *)&CommValue,4);

  /* Reactive power in scle factor of 1000 */
  CommValue = (UINT32)(pSmartPlugMetrologyData->ReactiveEnergy * M_SCALE_FACTOR_3_DIGIT);
  memcpy((void *)&pMetrologyDataComm->ReactivePower[0],(const void *)&CommValue,4);

  /* Power Factor in scle factor of 10000 */
  CommValue = (UINT32)(pSmartPlugMetrologyData->PowerFactor * M_SCALE_FACTOR_4_DIGIT);
  memcpy((void *)&pMetrologyDataComm->PowerFactor[0],(const void *)&CommValue,4);

  /* AC Frequency in scle factor of 1000 */
  CommValue = (UINT32)(pSmartPlugMetrologyData->InputACFrequency * M_SCALE_FACTOR_3_DIGIT);
  memcpy((void *)&pMetrologyDataComm->ACFrequency[0],(const void *)&CommValue,4);

  /* Consumed TrueEnergy in scle factor of 10000 */
  CommValue = (UINT32)(pSmartPlugMetrologyData->GrandTotalTrueEnergyKWh * M_SCALE_FACTOR_4_DIGIT);
  memcpy((void *)&pMetrologyDataComm->TrueEnergyConsumed[0],(const void *)&CommValue,4);

  /* Consumed ApparentEnergy in scle factor of 10000 */
  CommValue = (UINT32)(pSmartPlugMetrologyData->GrandTotalApparentEnergyKVAh * M_SCALE_FACTOR_4_DIGIT);
  memcpy((void *)&pMetrologyDataComm->ApperentEnergyConsumed[0],(const void *)&CommValue,4);

  /* Avg True power in scle factor of 1000 */
  CommValue = (UINT32)(pSmartPlugMetrologyData->AvgTruePower * M_SCALE_FACTOR_3_DIGIT);
  memcpy((void *)&pMetrologyDataComm->AvgTruePower[0],(const void *)&CommValue,4);

  /* UnMask interrupt */
  osi_ExitCritical(InterruptMask);

#ifdef METROLOGY_DEBUG
//#if 1
  //ChannelOffset1SecVf = (float)pChannelVoltage->DcChannelOffsetLast1Sec/(float)4096.0;
  //ChannelOffset1SecIf = (float)pChannelCurrent->DcChannelOffsetLast1Sec/(float)4096.0;

  //Debug print
  //printf("\n\n\n/*****************************************************/ \n\r");
  //printf("Smart plug metrology count %d\n\n\r", pSmartPlugMetrologyData->DataValidCountInSecs);

  //printf("DC channel offset voltage %f [V]\n\r",ChannelOffset1SecVf);
  printf("\nRms Voltage %f [V]\n\r",pSmartPlugMetrologyData->RmsVoltage);
  //printf("DC channel offset current %f [A]\n\r",ChannelOffset1SecIf);
  printf("Rms Current %f [A]\n\r",pSmartPlugMetrologyData->RmsCurrent);

  //printf("Input AC Frequency %f [Hz]\n\r",pSmartPlugMetrologyData->InputACFrequency);
  printf("True Energy %f [W]\n\n\r",pSmartPlugMetrologyData->TrueEnergy);
  //printf("Apparent Energy %f [VA]\n\r",pSmartPlugMetrologyData->ApparentEnergy);
  //printf("Reactive Energy %f [VAR]\n\r",pSmartPlugMetrologyData->ReactiveEnergy);
  //printf("Power factor %f\n\r",pSmartPlugMetrologyData->PowerFactor);
  //printf("Grand True Energy %lf [W]\n\r",pSmartPlugMetrologyData->GrandTotalTrueEnergy);
  //printf("Grand Apparent Energy %lf [VA]\n\r",pSmartPlugMetrologyData->GrandTotalApparentEnergy);
  //printf("Grand True Energy %f [KWh]\n\r",pSmartPlugMetrologyData->GrandTotalTrueEnergyKWh);
  //printf("Grand Apparent Energy %f [KVAh]\n\r",pSmartPlugMetrologyData->GrandTotalApparentEnergyKVAh);

  //printf("/*****************************************************/ \n\r");
#else
  //printf("Smart plug metrology count %d\n\r", pSmartPlugMetrologyData->DataValidCountInSecs);
  //printf("Smart plug metrology pulse %d\n\r", pSmartPlugMetrologyData->NumOfWhPulses);
  //printf("Smart plug metrology avg power %f\n\r", pSmartPlugMetrologyData->AvgTruePower);
#endif

  return 1;
}
/* This interrupt is triggered evry 1.024secs */
void ComputeSmartPlugMetrology( void )
{
  short unsigned int ResidualSamples;
  float TotalCyclInOffCal, ResidualSamplesfl, TotalDecSamplesPerCycle;
  float RmsNoiseV, RmsNoiseI, RmsNoisePact, RmsNoisePapr;
  float ChannelGainV, ChannelGainI;
  double TimeInSecs;
  UINT32 TimerMsCountInit;
  UINT8 MetroDataValid;

  /* Clear flag when fresh data available*/
  pSmartPlugMetrologyData->DataValid = 0;
  /* By Default assume metro data valid */
  MetroDataValid = 1;

  TimerMsCountInit = TimerMsCount;

  //printf("\n Voltage DC offset %d \n\r",pChannelVoltage->DcChannelOffsetLast1Sec);
  //printf("\n Current DC offset %d \n\r",pChannelCurrent->DcChannelOffsetLast1Sec);
  MetrologyDebugParams.VChDCOffset = pChannelVoltage->DcChannelOffsetLast1Sec;
  MetrologyDebugParams.IChDCOffset = pChannelCurrent->DcChannelOffsetLast1Sec;

  /* If input offset voltage < 0.5V then update o/p with zero */
  if((pChannelVoltage->DcChannelOffsetLast1Sec < 2048) || (pChannelCurrent->DcChannelOffsetLast1Sec < 2048))
  {
    pSmartPlugMetrologyData->RmsVoltage = (float)0;
    pSmartPlugMetrologyData->RmsCurrent = (float)0;
    pSmartPlugMetrologyData->TrueEnergy = (float)0;
    pSmartPlugMetrologyData->InputACFrequency = (float)0;
    MetroDataValid = 0;//data not valid
  }
  else
  {
    /* Updated these params in critical region */
    memcpy((void *)&RmsNoiseV,(const void *)&SmartPlugNvmmFile.MetrologyConfigData.RmsNoiseVCh[0],4);
    memcpy((void *)&ChannelGainV,(const void *)&SmartPlugNvmmFile.MetrologyConfigData.VChScaleFactor[0],4);
    memcpy((void *)&RmsNoiseI,(const void *)&SmartPlugNvmmFile.MetrologyConfigData.RmsNoiseICh[0],4);
    memcpy((void *)&ChannelGainI,(const void *)&SmartPlugNvmmFile.MetrologyConfigData.IChScaleFactor[0],4);

    /* RMS Voltage */
    pSmartPlugMetrologyData->RmsVoltage = (float)(sqrt((pChannelVoltage->AccSumSquare1SecVal << 4)))/(float)4096;
   // pSmartPlugMetrologyData->RmsVoltage = (float)1.2

    MetrologyDebugParams.RawRmsVoltage = pSmartPlugMetrologyData->RmsVoltage;

    /* Remove ADC noise rms*/
    //pSmartPlugMetrologyData->RmsVoltage -= RmsNoiseV;//DKS
    if(pSmartPlugMetrologyData->RmsVoltage < (float)0)
    {
      pSmartPlugMetrologyData->RmsVoltage = (float)0;
    }

    /* Multiply channel gain */
    pSmartPlugMetrologyData->RmsVoltage *= ChannelGainV;

    /* RMS Current */
    pSmartPlugMetrologyData->RmsCurrent = (float)(sqrt((pChannelCurrent->AccSumSquare1SecVal << 4)))/(float)4096;

    MetrologyDebugParams.RawRmsCurrent = pSmartPlugMetrologyData->RmsCurrent;

    if(1 == g_MetrologyCalEn)
    {
      g_MetroCalCount++;
      ClearWDT();
      if(g_MetroCalCount >= 3)
      {
        AvgRmsNoise += pSmartPlugMetrologyData->RmsCurrent;
      }
      Report("Rms I noise %f \n\r", pSmartPlugMetrologyData->RmsCurrent);
    }

    /* Remove ADC noise rms*/
    pSmartPlugMetrologyData->RmsCurrent -= M_RMS_NOISE_MEAN;//DKS
    if(pSmartPlugMetrologyData->RmsCurrent < (float)0)
    {
      pSmartPlugMetrologyData->RmsCurrent = 0;
    }
    /* Multiply channel gain */
    pSmartPlugMetrologyData->RmsCurrent *= ChannelGainI;

    /* True Energy */
    pSmartPlugMetrologyData->TrueEnergy = (float)((pMutualVIData->AccPowerValIn1Sec + (1<<7)) >> 8)/(float)4096;//20 bit to 12 bit scaling

    MetrologyDebugParams.RawActivePower = pSmartPlugMetrologyData->TrueEnergy;

    pSmartPlugMetrologyData->TrueEnergy *= (ChannelGainV*ChannelGainI);

    /* Remove Rms Noise power - 4.9W (120V) or 9.5W (230V)*/
    RmsNoisePact = (pSmartPlugMetrologyData->RmsVoltage)*((RmsNoiseI*0.5)*ChannelGainI);
    RmsNoisePapr = (pSmartPlugMetrologyData->RmsVoltage)*((RmsNoiseI-M_RMS_NOISE_MEAN)*ChannelGainI);

    //pSmartPlugMetrologyData->TrueEnergy -= RmsNoiseP;//DKS

    //printf("\n True Power %f \n\r",pSmartPlugMetrologyData->TrueEnergy);

    /* Active power is coherent dot product so noise is half */
    if(pSmartPlugMetrologyData->TrueEnergy < ((float)RmsNoisePact))
    {
      pSmartPlugMetrologyData->TrueEnergy = 0;
    }

    /* Compute Frequency */
    ResidualSamples = M_TOTAL_SAMPLES_PER_SEC - pMutualVIData->NumOfSamplesIn1SecFullCycles;
    TotalDecSamplesPerCycle = (float)(pMutualVIData->NumOfSamplesIn1SecFullCycles << 1)/(float)pMutualVIData->NumOfZeroCrossingIn1Sec;//per half cycle so *2
    if(pMutualVIData->TotalDecSamplesPerCycle != (float)0.0)
    {
      pMutualVIData->TotalDecSamplesPerCycle = (pMutualVIData->TotalDecSamplesPerCycle + TotalDecSamplesPerCycle)/(float)2.0;//take avg
    }
    else
    {
      pMutualVIData->TotalDecSamplesPerCycle = TotalDecSamplesPerCycle;
    }
    pSmartPlugMetrologyData->InputACFrequency = (float)pMutualVIData->NumOfZeroCrossingIn1Sec/(float)2.0;
    pSmartPlugMetrologyData->InputACFrequency += (float)ResidualSamples/(float)pMutualVIData->TotalDecSamplesPerCycle;//1000 samples for 1.024sec
    pSmartPlugMetrologyData->InputACFrequency /= (float)M_TIME_IN_SEC_PER_METROLOGY_PROC;//per sec frequency
  }

  //printf("\n Voltage rms %f \n\r",pSmartPlugMetrologyData->RmsVoltage);
  //printf("\n Current rms %f \n\r",pSmartPlugMetrologyData->RmsCurrent);
  //printf("\n freq %f \n\r",pSmartPlugMetrologyData->InputACFrequency);
  MetrologyDebugParams.ACFrequency = pSmartPlugMetrologyData->InputACFrequency;

  if( (MetroDataValid) && ((pSmartPlugMetrologyData->InputACFrequency >= M_MIN_AC_FREQUENCY) &&\
        (pSmartPlugMetrologyData->InputACFrequency <= M_MAX_AC_FREQUENCY)))
  {
    if(g_MetrologyGainScalCalEn)
    {
      AvgRmsVoltage += pSmartPlugMetrologyData->RmsVoltage;
      AvgRmsCurrent += pSmartPlugMetrologyData->RmsCurrent;
      g_MetroCalCount++;
      ClearWDT();
    }
  }

  /* Bound check for voltage & frequency */
  if(((pSmartPlugMetrologyData->InputACFrequency < M_MIN_AC_FREQUENCY) || (pSmartPlugMetrologyData->InputACFrequency > M_MAX_AC_FREQUENCY))||\
      ((pSmartPlugMetrologyData->RmsVoltage < M_MIN_AC_VOLTAGE) || (pSmartPlugMetrologyData->RmsVoltage > M_MAX_AC_VOLTAGE)))
  {
    pSmartPlugMetrologyData->RmsVoltage = (float)0;
    pSmartPlugMetrologyData->RmsCurrent = (float)0;
    pSmartPlugMetrologyData->TrueEnergy = (float)0;
    pSmartPlugMetrologyData->InputACFrequency = (float)0;
    MetroDataValid = 0;//data not valid
  }

  /* Bound check for current (power) */
  if(pSmartPlugMetrologyData->TrueEnergy <= (float)0)
  {
    pSmartPlugMetrologyData->TrueEnergy = (float)0;
    pSmartPlugMetrologyData->ApparentEnergy = (float)0;
    pSmartPlugMetrologyData->RmsCurrent = (float)0;
  }
  else
  {
    /* Apparent Energy */
    pSmartPlugMetrologyData->ApparentEnergy = (pSmartPlugMetrologyData->RmsVoltage)*pSmartPlugMetrologyData->RmsCurrent;
    /* apparent power noise check */
    if(pSmartPlugMetrologyData->ApparentEnergy < RmsNoisePapr)
    {
      pSmartPlugMetrologyData->ApparentEnergy = (float)0.0;
    }
  }

  /* Apparent Energy can not be less than True energy - eliminate noise effect */
  if(pSmartPlugMetrologyData->ApparentEnergy <= pSmartPlugMetrologyData->TrueEnergy)
  {
    pSmartPlugMetrologyData->ApparentEnergy = pSmartPlugMetrologyData->TrueEnergy;
    /* Reactive Energy is zero in this case */
    pSmartPlugMetrologyData->ReactiveEnergy = (float)0.0;
    /* Power factor is 1 */
    pSmartPlugMetrologyData->PowerFactor = (float)1.0;
    /* Compute current based on power */
    if(pSmartPlugMetrologyData->ApparentEnergy > (float)0.0)
    {
      pSmartPlugMetrologyData->RmsCurrent = pSmartPlugMetrologyData->ApparentEnergy/pSmartPlugMetrologyData->RmsVoltage;
    }
  }
  else
  {
    /* Reactive Energy */
    pSmartPlugMetrologyData->ReactiveEnergy = sqrtf((pSmartPlugMetrologyData->ApparentEnergy*pSmartPlugMetrologyData->ApparentEnergy) - \
                                                      (pSmartPlugMetrologyData->TrueEnergy*pSmartPlugMetrologyData->TrueEnergy));
    /* Power factor */
    pSmartPlugMetrologyData->PowerFactor = pSmartPlugMetrologyData->TrueEnergy/pSmartPlugMetrologyData->ApparentEnergy;
  }

  /* Compute Grand total power */
  pSmartPlugMetrologyData->GrandTotalTrueEnergy += (double)(pSmartPlugMetrologyData->TrueEnergy*(float)M_TIME_IN_SEC_PER_METROLOGY_PROC);
  pSmartPlugMetrologyData->GrandTotalTrueEnergyKWh = (float)(pSmartPlugMetrologyData->GrandTotalTrueEnergy*M_CONVERT_WATT_TO_KWH);// 1/(3600*1000) KwH
  pSmartPlugMetrologyData->GrandTotalTrueEnergyWh = (pSmartPlugMetrologyData->GrandTotalTrueEnergy*M_CONVERT_WATT_TO_WH);// 1/3600 wH

  pSmartPlugMetrologyData->GrandTotalApparentEnergy += (double)(pSmartPlugMetrologyData->ApparentEnergy*(float)M_TIME_IN_SEC_PER_METROLOGY_PROC);
  pSmartPlugMetrologyData->GrandTotalApparentEnergyKVAh = (float)(pSmartPlugMetrologyData->GrandTotalApparentEnergy*M_CONVERT_WATT_TO_KWH);//3600*1000 KvA

  /* Compute total number of samples required for full cycle offset compuation*/
  if(pSmartPlugMetrologyData->InputACFrequency != (float)0.0)
  {
    /* Compute total cycles in offset cal window */
    TotalCyclInOffCal = (float)(M_TOTAL_DMA_BLOCKS_PER_OFFSET_CAL*M_MIN_DECEM_SAMPLES_PER_DMA_BLOCK)/pMutualVIData->TotalDecSamplesPerCycle;
    ResidualSamplesfl = TotalCyclInOffCal - (float)((short unsigned int)TotalCyclInOffCal);
    ResidualSamplesfl *= pMutualVIData->TotalDecSamplesPerCycle;
    ResidualSamplesfl += (float)0.5;//rounding
    /* Update total samples corresponds to full cycels in window */
    pMutualVIData->TotalDecSamplesUsedForDcOffsetCal = (M_TOTAL_DMA_BLOCKS_PER_OFFSET_CAL*M_MIN_DECEM_SAMPLES_PER_DMA_BLOCK) -\
                               (short unsigned int)ResidualSamplesfl;
  }

  pSmartPlugMetrologyData->DataValidCountInSecs++;

  /* Generate a pulse for every 1 Wh energy spent */
  if((pSmartPlugMetrologyData->GrandTotalTrueEnergyWh - (double)pSmartPlugMetrologyData->NumOfWhPulses) >= (double)1.0)
  {
    pSmartPlugMetrologyData->NumOfWhPulses++;
  }

  TimeInSecs = ((double)pSmartPlugMetrologyData->DataValidCountInSecs*(double)M_TIME_IN_SEC_PER_METROLOGY_PROC) + (float)0.5;

  pSmartPlugMetrologyData->AvgTruePower = pSmartPlugMetrologyData->GrandTotalTrueEnergy/(double)TimeInSecs;

  MetrologyDebugParams.RmsVoltage = pSmartPlugMetrologyData->RmsVoltage;
  MetrologyDebugParams.RmsCurrent = pSmartPlugMetrologyData->RmsCurrent;
  MetrologyDebugParams.TrueEnergy = pSmartPlugMetrologyData->TrueEnergy;
  MetrologyDebugParams.ApparentEnergy = pSmartPlugMetrologyData->ApparentEnergy;
  MetrologyDebugParams.ReactiveEnergy = pSmartPlugMetrologyData->ReactiveEnergy;
  MetrologyDebugParams.PowerFactor = pSmartPlugMetrologyData->PowerFactor;

#if 0 //DKS test
  /* Rms voltage in scle factor of 1000 */
  pSmartPlugMetrologyData->RmsVoltage  = 240.123;

  /* Rms current in scle factor of 1000 */
  pSmartPlugMetrologyData->RmsCurrent = 1.025123;

  /* True power in scle factor of 1000 */
  pSmartPlugMetrologyData->TrueEnergy = 250.12345;

  /* Apparent power in scle factor of 1000 */
  pSmartPlugMetrologyData->ApparentEnergy = 280.12345;

  /* Reactive power in scle factor of 1000 */
  pSmartPlugMetrologyData->ReactiveEnergy = 5.4567;

  /* Power Factor in scle factor of 10000 */
  pSmartPlugMetrologyData->PowerFactor = 0.945678;

  /* AC Frequency in scle factor of 1000 */
  pSmartPlugMetrologyData->InputACFrequency = 50.0268;

  /* Consumed TrueEnergy in scle factor of 10000 */
  pSmartPlugMetrologyData->GrandTotalTrueEnergyKWh = (float)pSmartPlugMetrologyData->DataValidCountInSecs*0.5;

  /* Consumed ApparentEnergy in scle factor of 10000 */
  pSmartPlugMetrologyData->GrandTotalApparentEnergyKVAh = (float)pSmartPlugMetrologyData->DataValidCountInSecs*0.6;

  /* Avg True power in scle factor of 1000 */
  pSmartPlugMetrologyData->AvgTruePower = 230.4567;

  MetroDataValid = 1;
#endif

  /* Update flag indicating valid data available for smartplug */
  if(1 == MetroDataValid)
  {
    pSmartPlugMetrologyData->DataValid = 1;
  }

  if((TimerMsCount - TimerMsCountInit) > MipsComm)
    MipsComm = TimerMsCount - TimerMsCountInit;
}



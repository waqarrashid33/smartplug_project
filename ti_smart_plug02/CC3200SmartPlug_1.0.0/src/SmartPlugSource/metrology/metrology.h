//*****************************************************************************
// File: metrology.h
//
// Description: metrology module header file of Smartplug gen-1 application
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

#ifndef __METROLOGY_H__
#define __METROLOGY_H__

/* Macros  */
/* Address of ADC channel 0 FIFO data base */
#define ADC_BASE_FIFO_DATA                    0x4402E874

#define M_TOTAL_DMA_SAMPLES                   640
#define M_DECIMATION_RATIO                    64
#define M_TOTAL_DECIMATED_SAMPLES             (M_TOTAL_DMA_SAMPLES/M_DECIMATION_RATIO)
#define M_TOTAL_DMA_BLOCKS_PER_SEC            100

#define M_CONVERT_CODEWORD_TO_VALUES          6007 //each samples 15 bits codeword converted into 1.1*4/3 * 2^12 (scaling factor)
#define M_SAMPLE_CODEWORD_DECEMA_BITS         15
#define M_ORDER_OF_FIR_FILTER                 5

#define M_MIN_DECEM_SAMPLES_PER_DMA_BLOCK     M_TOTAL_DECIMATED_SAMPLES
#define M_TOTAL_DMA_BLOCKS_PER_OFFSET_CAL     M_TOTAL_DMA_BLOCKS_PER_SEC
#define M_TOTAL_SAMPLES_PER_SEC               (M_TOTAL_DMA_BLOCKS_PER_SEC*M_MIN_DECEM_SAMPLES_PER_DMA_BLOCK)
#define M_TOTAL_SECS_PER_OFFSET_CAL           3

#define M_V_CH_SCALE_FATOR_MIN                (float)(50.0)
#define M_I_CH_SCALE_FATOR_MIN                (float)(5.0)

#ifdef SMARTPLUG_HW
#define M_V_CH_SCALE_FATOR                    (float)(580.43273) /* Smartplug V scaling factor */
#define M_I_CH_SCALE_FATOR                    (float)(23.37815)  /* Smartplug I scaling factor */
#else
#define M_V_CH_SCALE_FATOR                    (float)(540.0)
#define M_I_CH_SCALE_FATOR                    (float)(1.0)
#endif

#define M_RMS_NOISE_IN_I_CH                   (float)(0.0018)   /* 2 sigma (90%) noise threshold in current channel */
#define M_RMS_NOISE_IN_V_CH                   (float)(0.0001)   /* Not used */
#define M_RMS_NOISE_MEAN                      (float)(0.0012)   /* Mean rms noise */
#define M_MAX_CAL_COUNT                       10
#define M_MAX_GAIN_CAL_COUNT                  30

#define M_MIN_OFFSET_VOLTAGE_OF_CH            (float)(0.5)

#define M_MIN_AC_FREQUENCY                    (float)(40.0)
#define M_MAX_AC_FREQUENCY                    (float)(70.0)

#define M_MIN_AC_VOLTAGE                      (float)(20.0)
#define M_MAX_AC_VOLTAGE                      (float)(300.0)

#define M_CONVERT_WATT_TO_KWH                 (double)(0.0000002777777778)

#define M_CONVERT_WATT_TO_WH                  (double)(0.0002777777778)

/* the data process happens in every 1.024 sec  */
#define M_TIME_IN_SEC_PER_METROLOGY_PROC      (1.024)

/* Scale factor for metrology data communication */
#define M_SCALE_FACTOR_3_DIGIT                (1000)
#define M_SCALE_FACTOR_4_DIGIT                (10000)
#define M_SCALE_FACTOR_5_DIGIT                (100000)

#define M_ONE_HOUR_IN_SEC                     3600

/* Enum and Structures */

typedef struct t_MetrologyConfig
{
  UINT8   RmsNoiseVCh[4];
  UINT8   RmsNoiseICh[4];
  UINT8   VChScaleFactor[4];
  UINT8   IChScaleFactor[4];

  char    mark[8]; //string holds mark "metrlgy"

}t_MetrologyConfig;

typedef struct t_AdcDmaChParams
{
  /* ADC & DMA ch and data buffer selection params */
  unsigned long      AdcChannelNum;
  unsigned long      AdcIntNum;
  unsigned long      DmaChannelNum;
  unsigned long      DmaDataDumpPing[M_TOTAL_DMA_SAMPLES+5];
  unsigned long      DmaDataDumpPong[M_TOTAL_DMA_SAMPLES+5];
  short unsigned int DmaBlockCount;

  /* Data decimation params */
  short unsigned int DecimatedSamples[M_TOTAL_DECIMATED_SAMPLES];
  unsigned long      ResidualSumInDmaBlock;
  short unsigned int ResidualSamplesLength;
  short unsigned int DecimatedTotalSamples;

  /* Data Filter params */
  short unsigned int FirZSamples[M_ORDER_OF_FIR_FILTER];
  short unsigned int FilteredSamples[M_TOTAL_DECIMATED_SAMPLES];
  short unsigned int FilteredValues[M_TOTAL_DECIMATED_SAMPLES];
  short unsigned int FirFilterStartInit;

  /* Channel DC offset calculation params */
  short unsigned int DcOffsetAccThrshReached;
  short unsigned int DcOffsetCount;
  unsigned long      DcOffsetAccVal;
  short unsigned int DcOffset1SecMoAvArray[M_TOTAL_SECS_PER_OFFSET_CAL];
  short unsigned int DcOffset1SecMoAvAccThrshReached;
  short unsigned int DcOffset1SecMoAvCount;
  short unsigned int DcChannelOffsetLast1Sec;
  unsigned long      DcOffsetMoAvAccVal;
  short unsigned int DcChannelOffset;
  short unsigned int DcOffset1SecCalInit;

  /* Channel data sum square calculation params */
  short unsigned int TotalSumSquareSamples;
  unsigned long      AccSumSquareVal;
  unsigned long      AccSumSquare1SecVal;

} t_AdcDmaChParams;

typedef struct t_MutualVIChParams
{
  /* Frequency detection params */
  short unsigned int FirstZeroCrossDetected;
  short unsigned int ZeroCrossingCount;
  short unsigned int NumOfSamplesInTotalFullCycles;
  short unsigned int NumOfZeroCrossingIn1Sec;
  short unsigned int NumOfSamplesIn1SecFullCycles;
  short unsigned int TotalDecSamplesUsedForDcOffsetCal;
  float              TotalDecSamplesPerCycle;

  /* True Power calculation params */
  short unsigned int TotalPowerSamples;
  signed long        AccPowerVal;
  unsigned long      AccPowerValIn1Sec;

}t_MutualVIChParams;

typedef volatile struct t_SmartPlugMetrologyParams
{
  /* SmartPlug Metrology Raw data Params*/
  short unsigned int DataValid;

  /* SmartPlug Metrology Global Params*/
  unsigned long      DataValidCountInSecs;
  unsigned long      NumOfWhPulses;
  float              RmsVoltage;
  float              RmsCurrent;
  float              TrueEnergy;
  float              ApparentEnergy;
  float              ReactiveEnergy;
  float              PowerFactor;
  float              InputACFrequency;
  double             GrandTotalTrueEnergy;
  double             GrandTotalApparentEnergy;
  float              GrandTotalTrueEnergyKWh;
  float              GrandTotalApparentEnergyKVAh;
  double             GrandTotalTrueEnergyWh;
  float              AvgTruePower;

}t_SmartPlugMetrologyParams;

typedef volatile struct t_MetrologyDebugParams
{
  float              RawRmsVoltage;
  float              RawRmsCurrent;
  float              ACFrequency;
  float              VChDCOffset;
  float              IChDCOffset;
  float              RawActivePower;
  float              RmsVoltage;
  float              RmsCurrent;
  float              TrueEnergy;
  float              ApparentEnergy;
  float              ReactiveEnergy;
  float              PowerFactor;
}t_MetrologyDebugParams;

typedef volatile struct
{
  /* Metrology data */
  unsigned char MetrologyCount[4];
  unsigned char RmsVoltage[4];
  unsigned char RmsCurrent[4];
  unsigned char TruePower[4];
  unsigned char ApparentPower[4];
  unsigned char ReactivePower[4];
  unsigned char PowerFactor[4];
  unsigned char ACFrequency[4];
  unsigned char TrueEnergyConsumed[4];
  unsigned char ApperentEnergyConsumed[4];
  unsigned char AvgTruePower[4];

  /* Average Energy */
  unsigned char EnergyUptoLastHour[4];
  unsigned char AveragePower[4];
  unsigned char APUpdateCount[4];

  /* Average Energy 24 Hours */
  unsigned char AveragePower24Hr[24][4];
  unsigned char AP24HrCount;
  unsigned char AP24HrRollover;

  /* Data sender indication */
  unsigned char UpdateTime[4];
  unsigned char UpdateTimeServer[4];

}t_SmartPlugMetrologyComm;

/* variables  */

extern t_AdcDmaChParams ChannelVoltage, ChannelCurrent;
extern t_AdcDmaChParams *pChannelVoltage, *pChannelCurrent;//pointer to data structure

extern t_MutualVIChParams MutualVIData;
extern t_MutualVIChParams *pMutualVIData;//pointer to data structure

extern t_SmartPlugMetrologyParams SmartPlugMetrologyData;
extern t_SmartPlugMetrologyParams *pSmartPlugMetrologyData;//pointer to data structure

extern t_SmartPlugMetrologyComm MetrologyDataComm;
extern t_SmartPlugMetrologyComm *pMetrologyDataComm;

extern const short signed int FirCoeff[M_ORDER_OF_FIR_FILTER+1]
#ifdef extern
= {-43, 110, 1981, 1981, 110, -43 }
#endif
;//5th order FIR coefficients scaled by 4096

/* Function proto types */
extern void InitMetrologyModule( void );

extern void ADCIntHandlerChV(void);

extern void ADCIntHandlerChI(void);

extern void DecimateAdcSamples(unsigned long *pDataDumpBuf, unsigned long *pResidualVal, \
                        short unsigned int TotalInputSamples, short unsigned int DecimationRatio, \
                        short unsigned int *pResidualLength, short unsigned int *pDecimatedSamples,\
                        short unsigned int *pTotalDecimatedSamples);

extern void FirFilter(short unsigned int *pDecimatedSamples, short unsigned int DecimatedTotalSamples,\
               short unsigned int *pFilteredSamples, short unsigned int *pFirZSamples);

extern void ConvertSampleCodeWordsToValues(short unsigned int *pFilteredSamples, short unsigned int DecimatedTotalSamples,\
                                    short unsigned int *pFilteredValues);

extern short unsigned int ComputeChannelOffset(short unsigned int *pFilteredValues, t_AdcDmaChParams *pChannelParams);

extern void ComputeSumSquare(short unsigned int *pFilteredValues, short unsigned int DecimatedTotalSamples,\
                      unsigned long *pAccSumSquareVal, short unsigned int *pTotalSumSquareSamples,\
                      short unsigned int ChannelOffset);

extern void ComputeTrueEnergy(short unsigned int *pFilteredValuesV, short unsigned int *pFilteredValuesI, short unsigned int DecimatedTotalSamples,\
                  signed long *pAccPowerVal, short unsigned int *pTotalPowerSamples, short unsigned int ChannelOffsetV, short unsigned int ChannelOffsetI);


extern void ComputeSmartPlugMetrology( void );


extern void ComputeZeroCrossing(short unsigned int *pFilteredValues, short unsigned int DecimatedTotalSamples);

extern void InitMetrologyAdcDma( void );

extern int UpdateMetrologyDataForComm( void );

extern int UpdateStatusOfMetrologyModule(void);

#endif


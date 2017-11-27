/******************************************************************************
*
*   Copyright (C) 2014 Texas Instruments Incorporated
*
*   All rights reserved. Property of Texas Instruments Incorporated.
*   Restricted rights to use, duplicate or disclose this code are
*   granted through contract.
*
*   The program may not be used without the written permission of
*   Texas Instruments Incorporated or against the terms and conditions
*   stipulated in the agreement under which this program has been supplied,
*   and under no circumstances can it be used with non-TI connectivity device.
*
******************************************************************************/

package com.ti.cc3200smartplug.data;

/**
 * Packet 3.1.8 - Metrology calibration specific requirements
 */
public class DToA_MetrologyCalibration extends DToA_Packet{
	
	public float voltageChannelNoise;
	public float currentChannelNoise;
	public float voltageChannelScaleFactor;
	public float currentChannelScaleFactor;
	
	public int timestamp;
	
	/**
	 * Default constructor that initializes all values.
	 */
	public DToA_MetrologyCalibration(long dataReceivedTime){
		this.dataReceivedTime = dataReceivedTime;
		
		voltageChannelNoise = 0;
		currentChannelNoise = 0;
		voltageChannelScaleFactor = 0;
		currentChannelScaleFactor = 0;
	}
	
	public DToA_MetrologyCalibration(
			long	dataReceivedTime,
			float voltageChannelNoise,
			float currentChannelNoise,
			float voltageChannelScaleFactor,
			float currentChannelScaleFactor){
		
		this.dataReceivedTime = dataReceivedTime;
		
		this.voltageChannelNoise = voltageChannelNoise;
		this.currentChannelNoise = currentChannelNoise;
		this.voltageChannelScaleFactor = voltageChannelScaleFactor;
		this.currentChannelScaleFactor = currentChannelScaleFactor;
	}
	
	@Override
	public String toString(){
		return "Voltage channel noise:" + voltageChannelNoise
				+ ", Current channel noise:" + currentChannelNoise
				+ ", Voltage channel scale factor:" + voltageChannelScaleFactor
				+ ", Current channel scale factor:" + currentChannelScaleFactor
				+ ", timestamp:" + timestamp;
	}
}

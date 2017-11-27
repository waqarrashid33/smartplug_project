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
 * Packet 3.1.2 - Packet to update average energy at hourly frequency
 */
public class DToA_AvgEnergyHourly extends DToA_Packet{
		
	public float 	avgEnergy;
	public int		timestamp;
	
	/**
	 * Default constructor that initializes all values.
	 */
	public DToA_AvgEnergyHourly(long dataReceivedTime){
		this.dataReceivedTime = dataReceivedTime;
		avgEnergy = 0;
		timestamp = 0;
	}
	
	public DToA_AvgEnergyHourly(
			long	dataReceivedTime,
			float 	avgEnergy,
			int		timestamp){
		
		this.dataReceivedTime = dataReceivedTime;
		
		this.avgEnergy = avgEnergy;
		this.timestamp = timestamp;
	}
}

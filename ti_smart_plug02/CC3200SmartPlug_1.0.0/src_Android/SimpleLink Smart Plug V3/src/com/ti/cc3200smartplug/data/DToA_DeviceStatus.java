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
 * Packet 3.1.3 - Packet to update device status
 */
public class DToA_DeviceStatus extends DToA_Packet{
	
	public boolean	deviceOn;
	public boolean	powerSavingEnable;
	public byte		updateInterval;
	public float	energyThreshold;
	public float	powerThreshold;
	public int		timestamp;
	
	/**
	 * Default constructor that initializes all values.
	 */
	public DToA_DeviceStatus(long dataReceivedTime){
		this.dataReceivedTime = dataReceivedTime;
		
		deviceOn = false;
		powerSavingEnable = false;
		updateInterval = 1;
		energyThreshold = 0;
		powerThreshold = 0;
		timestamp = 0;
	}
	
	public DToA_DeviceStatus(
			long	dataReceivedTime,
			boolean	deviceOn,
			boolean	powerSavingEnable,
			byte	updateInterval,
			float	energyThreshold,
			float	powerThreshold,
			int		timestamp){
		
		this.dataReceivedTime = dataReceivedTime;
		
		this.deviceOn = deviceOn;
		this.powerSavingEnable = powerSavingEnable;
		this.updateInterval = updateInterval;
		this.energyThreshold = energyThreshold;
		this.powerThreshold = powerThreshold;
		this.timestamp = timestamp;
	}
}

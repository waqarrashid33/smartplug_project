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
 * Packet 3.1.4 - Warning message for over energy and power consumption threshold
 */
public class DToA_Warning extends DToA_Packet{
	
	public boolean	hasNewValue;
	public int		timestamp;
	
	/**
	 * Default constructor that initializes all values.
	 */
	public DToA_Warning(long dataReceivedTime){
		this.dataReceivedTime = dataReceivedTime;
		hasNewValue = false;
		timestamp = 0;
	}
	
	public DToA_Warning(
			long	dataReceivedTime,
			boolean	hasNewValue,
			int		timestamp){
		
		this.dataReceivedTime = dataReceivedTime;
		this.hasNewValue = hasNewValue;
		this.timestamp = timestamp;
	}
}

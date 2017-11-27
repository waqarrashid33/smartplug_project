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
 * Packet 3.1.6 - Device's default switch table
 */
public class DToA_SwitchTable extends DToA_Packet{
	
	//Each of the following is taking 1 byte.
	//WakeUp Hour;
	//WakeUp Minute;
	//Leave  Hour;
	//Leave  Minute;
	//Return Hour;
	//Return Minute;
	//Sleep  Hour;
	//Sleep  Minute;
	public byte[] weekdaySchedule = new byte[8];
	public byte[] saturdaySchedule = new byte[8];
	public byte[] sundaySchedule = new byte[8];
	
	public byte[] smartPlugTime = new byte[3];
	
	public boolean active;
	
	/**
	 * Default constructor that initializes all values.
	 */
	public DToA_SwitchTable(long dataReceivedTime){
		this.dataReceivedTime = dataReceivedTime;
		
		weekdaySchedule = new byte[8];
		saturdaySchedule = new byte[8];
		sundaySchedule = new byte[8];
		smartPlugTime = new byte[3];
		active = false;
	}
	
	public DToA_SwitchTable(
			long	dataReceivedTime,
			byte[] weekdaySchedule,
			byte[] saturdaySchedule,
			byte[] sundaySchedule,
			byte[] smartPlugTime,
			boolean valid){
		
		this.dataReceivedTime = dataReceivedTime;
		
		this.weekdaySchedule = weekdaySchedule;
		this.saturdaySchedule = saturdaySchedule;
		this.sundaySchedule = sundaySchedule;
		this.smartPlugTime = smartPlugTime;
		this.active = valid;
	}
}

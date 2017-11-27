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
 * Packet 3.1.1 - Packet to Update Metrology Data
 */
public class DToA_MetrologyData extends DToA_Packet{
	
	public float	activePower;
	public float	voltage;
	public float	current;
	public float	frequency;
	public float	reactivePower;
	public float	cos;
	public float	apparentPower;
	public float	KWh;
	public float	avgPower;
	public int		timestamp;
	
	/**
	 * Default constructor that initializes all values.
	 */
	public DToA_MetrologyData(long dataReceivedTime){
		this.dataReceivedTime = dataReceivedTime;
		activePower = 0;
		voltage = 0;
		current = 0;
		frequency = 0;
		reactivePower = 0;
		cos = 0;
		apparentPower = 0;
		KWh = 0;
		avgPower = 0;
		timestamp = 0;
	}
	
	public DToA_MetrologyData(
			long	dataReceivedTime,
			float	activePower,
			float	voltage,
			float	current,
			float	frequency,
			float	reactivePower,
			float	cos,
			float	apparentPower,
			float	KWh,
			float	avgPower,
			int		timestamp){
		
		this.dataReceivedTime = dataReceivedTime;
		
		this.activePower= activePower;
		this.voltage	= voltage;
		this.current 	= current;
		this.frequency	= frequency;
		this.reactivePower	= reactivePower;
		this.cos		= cos;
		this.apparentPower	= apparentPower;
		this.KWh		= KWh;
		this.avgPower	= avgPower;
		this.timestamp	= timestamp;
	}
}

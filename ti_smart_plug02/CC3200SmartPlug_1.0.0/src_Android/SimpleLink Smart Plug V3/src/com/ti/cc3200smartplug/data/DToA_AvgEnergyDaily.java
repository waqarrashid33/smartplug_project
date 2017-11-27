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

import java.util.ArrayList;

/**
 * Packet 3.1.5 - Average energy (KWh) of last 24 hours
 */
public class DToA_AvgEnergyDaily extends DToA_Packet{
	
	public ArrayList<Float>	avgEnergyIn24Hour;
	public int				timestamp;
	
	/**
	 * Default constructor that initializes all values.
	 */
	public DToA_AvgEnergyDaily(long dataReceivedTime){
		this.dataReceivedTime = dataReceivedTime;
		
		avgEnergyIn24Hour = new ArrayList<Float>(24);
		timestamp = 0;
	}
	
	public DToA_AvgEnergyDaily(
			final long	dataReceivedTime,
			final float	avgEnergy[],
			final int	timestamp){
		
		this.dataReceivedTime	= dataReceivedTime;
		
		avgEnergyIn24Hour = new ArrayList<Float>(24);
		
		for(int i=0; i<24; i++){
			avgEnergyIn24Hour.add(avgEnergy[i]);
		}
		
		this.timestamp	= timestamp;
	}
	
	/**
     * Retrieves an Average Energy data with an given hour in the last 24 hour
     * @param index hour to retrieve. 0 for last hour, 1 for past 1-2 hour. So on and so forth
     * @return average energy value of that hour
     */
	public float getHourData(int index){
    	return avgEnergyIn24Hour.get(index);
    }
	
	/**
     * Retrieves all Average Energy data in the last 24 hours
     * @return an ArrayList of average energy value of past 24 hours
     */
	public ArrayList<Float> get24HourData(){
    	return avgEnergyIn24Hour;
    }
	
	public String[] get24HourDataStringArray(){
		String[] data = new String[24];
		
		for(int i=0; i<24; i++){
			data[i] = Float.toString(avgEnergyIn24Hour.get(i));
		}
		
		return data;
	}
}

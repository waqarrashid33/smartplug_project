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

import com.ti.cc3200smartplug.Common;

/**
 * Packet 3.1.7 - Cloud specific requirements
 */
public class DToA_CloudInfo extends DToA_Packet{
	
	public String	vendor;
	public String	model;
	public byte[]	MAC = new byte[6];
	public byte		cloudStatus;
	public int		timestamp;
	
	/**
	 * Default constructor that initializes all values.
	 */
	public DToA_CloudInfo(long dataReceivedTime){
		this.dataReceivedTime = dataReceivedTime;
		
		vendor = null;
		model = null;
		MAC = new byte[8];
		cloudStatus = 0x00;
		timestamp = 0;
	}
	
	public DToA_CloudInfo(
			long	dataReceivedTime,
			String	vendor,
			String	model,
			byte[]	MAC,
			byte	cloudStatus,
			int		timestamp){
		
		this.dataReceivedTime = dataReceivedTime;
		
		this.vendor = vendor;
		this.model = model;
		this.cloudStatus = cloudStatus;
		this.MAC = MAC;
		this.timestamp = timestamp;
	}
	
	@Override
	public String toString(){
		StringBuilder sb = new StringBuilder(100);
		sb.append("Vendor:");
		sb.append(vendor);
		sb.append(", Model:");
		sb.append(model);
		sb.append(", MAC:");
		sb.append(Common.ByteArrayToString(MAC, 0, 6, true));
		sb.append(", Status:");
		sb.append(String.format("%02X", cloudStatus));
		sb.append(", timestamp:");
		sb.append(timestamp);
		return sb.toString();
	}
}

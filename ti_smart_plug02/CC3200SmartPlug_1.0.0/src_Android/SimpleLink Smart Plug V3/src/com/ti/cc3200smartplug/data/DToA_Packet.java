package com.ti.cc3200smartplug.data;

public class DToA_Packet {
	public long	dataReceivedTime;
	
	public final long getLastDataTime(){
		return dataReceivedTime;
	}
	
	public final int getLastDataTimeInSecond(){
		return (int) (dataReceivedTime/1000);
	}
}

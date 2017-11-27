package com.ti.cc3200smartplug.mdns;

import javax.jmdns.ServiceInfo;

/**
 * For onProgressUpdate() in MDNSThread to pass on messages
 * @author Victor Lin
 *
 */
public class MDNSThreadUpdateMessage{
	private int OP_CODE;
	private ServiceInfo service;
	
	public MDNSThreadUpdateMessage(int OP_CODE, ServiceInfo s){
		this.OP_CODE = OP_CODE;
		this.service = s;
	}
	
	public int getOpCode(){
		return OP_CODE;
	}
	
	public ServiceInfo getServiceInfo(){
		return service;
	}
}

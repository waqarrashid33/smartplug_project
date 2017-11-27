package com.ti.cc3200smartplug;

import com.ti.cc3200smartplug.service.SmartPlugDevice;

/**
 * Callback flow:
 * <br/>
 * <br/>LocalConnectionThread:		onProgressUpdate()				onProgressUpdate()
 * <br/>									|								|
 * <br/>									V								V
 * <br/>SmartPlugService:			LCTProgressUpdate()				CCTProgressUpdate()
 * <br/>					(if no activity is present, these will not pass on the infromation.)
 * <br/> 									|								|
 * <br/>									V								V
 * <br/>SmartPlugBaseActivity:		LocalConnectionUpdate()			CloudConnectionUpdate()
 * <br/>									|								|
 * <br/>	DeviceListActivity:				V								V
 * <br/>OR	DeviceDetailActivity:	LocalConnectionUpdate()			CloudConnectionUpdate()
 * <br/>									|								|
 * <br/>									V								V
 * <br/>DeviceDetailFragment:		LocalConnectionUpdate()			CloudConnectionUpdate()
 * <br/>
 * tcpThreadCode is one of the update integers found in "LocalConnectionThread"
 */
public interface GuiUpdateCallback {
	public int LocalConnectionUpdate(final short tcpThreadCode, SmartPlugDevice device);
	public int CloudConnectionUpdate(final short cloudThreadCode, SmartPlugDevice device, String msg);
}

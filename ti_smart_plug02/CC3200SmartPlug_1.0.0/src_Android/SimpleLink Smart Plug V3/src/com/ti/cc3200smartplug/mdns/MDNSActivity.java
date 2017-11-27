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

package com.ti.cc3200smartplug.mdns;

import android.app.Fragment;
import android.os.Bundle;

import com.ti.cc3200smartplug.DeviceDetailFragment;
import com.ti.cc3200smartplug.R;
import com.ti.cc3200smartplug.SmartPlugBaseActivity;
import com.ti.cc3200smartplug.service.SmartPlugDevice;

public class MDNSActivity extends SmartPlugBaseActivity{

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_mdns);
	}
	
	@Override
	public void onBackPressed() {
	    finish();
	    overridePendingTransition(android.R.anim.slide_in_left, android.R.anim.slide_out_right); 
	}
	
	@Override
	public int LocalConnectionUpdate(short tcpThreadCode, SmartPlugDevice device) {
		// Nothing
		return 0;
	}

	@Override
	public int CloudConnectionUpdate(short cloudThreadCode, SmartPlugDevice device, String msg) {
		// Nothing
		return 0;
	}
	
	@Override
	protected void serviceConnectedCallBack() {
		// Nothing
	}

	
	@Override
	protected void statusUpdateWiFi(boolean wifiOn, boolean cellularOn) {
		// No implementation as of right now, but might be useful in the future.
	}
}
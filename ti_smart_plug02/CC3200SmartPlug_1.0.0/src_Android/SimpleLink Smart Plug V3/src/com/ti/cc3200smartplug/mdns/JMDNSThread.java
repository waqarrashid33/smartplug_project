/******************************************************************************
*
*   Copyright (C) 2015 Texas Instruments Incorporated
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

import java.io.IOException;
import java.net.InetAddress;
import java.util.Locale;

import javax.jmdns.JmDNS;
import javax.jmdns.ServiceEvent;
import javax.jmdns.ServiceInfo;
import javax.jmdns.ServiceListener;

import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.MulticastLock;
import android.os.AsyncTask;
import android.util.Log;
import android.widget.ProgressBar;
import android.widget.TextView;

public class JMDNSThread extends AsyncTask<Void, MDNSThreadUpdateMessage, Void> {

	public interface ThreadUpdate {
		public void resolvedService(ServiceInfo info);
	}

	private static final String DEBUG_CLASS = "JMDNSThread";

	private static final int PROGRESS_UPDATE_SERVICE_RESOLVED = 1;

	private ThreadUpdate tf;
	private WifiManager wifiManager;

	private volatile boolean RUNNING;

	private JmDNS jmdns;
	private ServiceListener mdnsServiceListener;
	private static final int MDNS_SEARCH_TIMEOUT = 5000;
	private static final int SERVICE_RESOLVE_TIMEOUT = 5000;

	public JMDNSThread(ThreadUpdate t, WifiManager w) {
		RUNNING = true;
		tf = t;
		wifiManager = w;
	}

	// Custom defined cancel flag
	public void cancel() {
		RUNNING = false;
	}

	@Override
	protected Void doInBackground(Void... params) {
		MulticastLock mcast_lock = wifiManager.createMulticastLock("MDNSLock");
		mcast_lock.setReferenceCounted(true);
		mcast_lock.acquire();

		while (RUNNING) {
			try {
				int ip = wifiManager.getConnectionInfo().getIpAddress();
				String ipString = String.format(Locale.ENGLISH, "%d.%d.%d.%d", (ip & 0xff), (ip >> 8 & 0xff), (ip >> 16 & 0xff), (ip >> 24 & 0xff));
				Log.d("ip", ipString);
				jmdns = JmDNS.create(InetAddress.getByName(ipString));
				jmdns.addServiceListener(MDNSFragment.SERVICE_TYPE, mdnsServiceListener = new ServiceListener() {

					@Override
					public void serviceAdded(ServiceEvent e) {
						jmdns.getServiceInfo(e.getType(), e.getName(), SERVICE_RESOLVE_TIMEOUT);
					}

					@Override
					public void serviceRemoved(ServiceEvent e) {
					}

					@Override
					public void serviceResolved(ServiceEvent e) {
						Log.d(DEBUG_CLASS, "Service resolved: " + e.getName() + " of type " + e.getType());
						publishProgress(new MDNSThreadUpdateMessage(PROGRESS_UPDATE_SERVICE_RESOLVED, e.getInfo()));
					}
				});

				/* Just list, but don't do anything else */
				jmdns.list(MDNSFragment.SERVICE_TYPE, MDNS_SEARCH_TIMEOUT);

			} catch (IOException e) {
				e.printStackTrace();
			}

			if (jmdns != null)
				jmdns.removeServiceListener(MDNSFragment.SERVICE_TYPE, mdnsServiceListener);

		}

		if (jmdns != null)
			jmdns.removeServiceListener(MDNSFragment.SERVICE_TYPE, mdnsServiceListener);

		try {
			mcast_lock.release();
		} catch (Exception e) {
		}

		return null;
	}

	protected void onProgressUpdate(MDNSThreadUpdateMessage... progress) {
		switch (progress[0].getOpCode()) {
		case PROGRESS_UPDATE_SERVICE_RESOLVED:
			Log.d(DEBUG_CLASS, "Service info: " + progress[0].getServiceInfo());
			ServiceInfo s = progress[0].getServiceInfo();
			Log.d(DEBUG_CLASS, "NAME  : " + s.getName());
			Log.d(DEBUG_CLASS, "SERVER: " + s.getServer());
			tf.resolvedService(progress[0].getServiceInfo());
			break;

		default:
			break;
		}
	}

	protected void onPostExecute(Void result) {
		Log.d(DEBUG_CLASS, "DONE");
	}
}

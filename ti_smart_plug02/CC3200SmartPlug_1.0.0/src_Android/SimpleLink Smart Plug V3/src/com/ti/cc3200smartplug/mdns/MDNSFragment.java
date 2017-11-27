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

import java.util.ArrayList;

import javax.jmdns.ServiceInfo;

import android.app.Activity;
import android.app.Fragment;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.AsyncTask.Status;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;

import com.ti.cc3200smartplug.R;
import com.ti.cc3200smartplug.SmartPlugBaseActivity;

public class MDNSFragment extends Fragment implements JMDNSThread.ThreadUpdate{
		
	private static final String DEBUG_CLASS = "MDNSFragment";
	
	/* The array storing all found devices */
	private ListView mDNSListView;
	private MDNSArrayAdapter mDNSListAdapter;
	
	private View spinningThingy;
	private View textWiFiOff;
	
    private volatile ArrayList<ServiceInfo> serviceFound;
	
	private JMDNSThread thread;
     
    public static final String SERVICE_TYPE = "_device-info._tcp.local.";
//	public static final String SERVICE_TYPE = "_udisks-ssh._tcp.local.";
    
    private BroadcastReceiver mWiFiStateReceiver;
	
	@Override
	public void onAttach(Activity activity) {
		super.onAttach(activity);
		
		serviceFound = new ArrayList<ServiceInfo>();
		mDNSListAdapter	= new MDNSArrayAdapter((SmartPlugBaseActivity) activity, this, serviceFound);		
	}
	
	@Override
	public void onCreate(Bundle savedInstanceState){
		super.onCreate(savedInstanceState);
		
		mWiFiStateReceiver = new BroadcastReceiver(){
			@Override
			public void onReceive(Context context, Intent intent) {
				Log.w(DEBUG_CLASS, "WiFi Status changed");
				
				ConnectivityManager connManager = (ConnectivityManager) getActivity().getSystemService(android.content.Context.CONNECTIVITY_SERVICE);
				
				boolean wifiOn = connManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI).isConnected();
				
				if(wifiOn){
					onWiFiOn();
				}else{
					onWiFiOff();
				}
			}
		};
	}
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.fragment_mdns_search, container, false);
		
		mDNSListView	= (ListView) rootView.findViewById(R.id.mdnsList);
		mDNSListView.setAdapter(mDNSListAdapter);
		
		spinningThingy	= rootView.findViewById(R.id.progressBar_mdns_search);
		textWiFiOff		= rootView.findViewById(R.id.mdns_textview_wifiOff);
		textWiFiOff.setVisibility(View.INVISIBLE);
		
		Log.d(DEBUG_CLASS, "[onCreateView] Done");
		return rootView;
	}
	
	@Override
	public void onResume(){
		super.onPause();
		
		ConnectivityManager manager = (ConnectivityManager) getActivity().getSystemService(android.content.Context.CONNECTIVITY_SERVICE);
		
		if(manager.getNetworkInfo(ConnectivityManager.TYPE_WIFI).isConnected()){
			thread = new JMDNSThread(MDNSFragment.this, (WifiManager) getActivity().getSystemService(android.content.Context.WIFI_SERVICE));
			thread.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
		
		} else {
			spinningThingy.setVisibility(View.INVISIBLE);
			textWiFiOff.setVisibility(View.VISIBLE);
		}
		
		getActivity().registerReceiver(mWiFiStateReceiver, new IntentFilter("android.net.conn.CONNECTIVITY_CHANGE"));
		
		Log.d(DEBUG_CLASS, "[onResume] Done");
	}
	
	@Override
	public void onPause(){
		super.onPause();
		
		try{
			thread.cancel();
		} catch(Exception e) {
			//Any exception, don't care.
		}
		
		getActivity().unregisterReceiver(mWiFiStateReceiver);
		
		Log.d(DEBUG_CLASS, "[onPause] Done");
	}
	
	@Override
	public void onStop(){
		super.onStop();
		Log.d(DEBUG_CLASS, "[onStop] Done");
	}
	
	@Override
	public void onDestroyView(){
		super.onDestroyView();
		Log.d(DEBUG_CLASS, "[onDestroyView] Done");
	}
	
	@Override
	public void onDestroy(){
		Log.d(DEBUG_CLASS, "[onDestroy] Done");
		super.onDestroy();
	}
	
	@Override
	public void onDetach(){
		Log.d(DEBUG_CLASS, "[onDetach] Done");
		super.onDetach();
	}
	
	@Override
	public void resolvedService(ServiceInfo info) {
		
		//If the device already in the mDNS list, do not add.
		for(ServiceInfo a: serviceFound){
			if(a.getServer().equals(info.getServer())){
				return;
			}
		}
		
		serviceFound.add(info);
		mDNSListAdapter.notifyDataSetChanged();
	}
	
	public void onWiFiOn(){
		spinningThingy.setVisibility(View.VISIBLE);
		textWiFiOff.setVisibility(View.INVISIBLE);
		
		if(thread.getStatus() == Status.RUNNING)
			return;
		
		//Start a nre thread if no MDNS thread is running
		thread = new JMDNSThread(MDNSFragment.this, (WifiManager) getActivity().getSystemService(android.content.Context.WIFI_SERVICE));
		thread.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
	}
	
	public void onWiFiOff(){
		spinningThingy.setVisibility(View.INVISIBLE);
		textWiFiOff.setVisibility(View.VISIBLE);
		
		thread.cancel();
	}
}
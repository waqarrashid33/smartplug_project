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

import java.net.InetAddress;
import java.util.ArrayList;
import java.util.Locale;

import javax.jmdns.ServiceInfo;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.ti.cc3200smartplug.Common;
import com.ti.cc3200smartplug.R;
import com.ti.cc3200smartplug.SmartPlugBaseActivity;
import com.ti.cc3200smartplug.SmartPlugBaseActivity.DeviceOperationCompleteCallback;
import com.ti.cc3200smartplug.service.SmartPlugDevice;

public class MDNSArrayAdapter extends ArrayAdapter<ServiceInfo>{
	
	public interface DeviceFinder{
		/**
		 * Checks if device is in the main list.
		 * @return true if device is in the main list
		 */
		public SmartPlugDevice deviceExistInList(String mac_address);
		
		/**
		 * Add to main list, as a local device
		 * @param item
		 * @param macAddress
		 * @return
		 */
		public boolean addToDeviceList(ServiceInfo item, String macAddress, String rid, String cik);
		
		public boolean updateDeviceLocalAddress(InetAddress address, String macAddress);
	}
	
	private SmartPlugBaseActivity mContext;
	private LayoutInflater mInflater;
	
	private static int list_view_resource = R.layout.listview_item_mdns;

	public MDNSArrayAdapter(SmartPlugBaseActivity context, MDNSFragment fragment, ArrayList<ServiceInfo> serviceFound) {
		super(context, list_view_resource, serviceFound);

		mContext = context;
		mInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
	}
	
	@Override
	public View getView(int position, View convertView, ViewGroup parent){
		
		View view;

        if (convertView == null) {
            view = mInflater.inflate(list_view_resource, parent, false);
        } else {
            view = convertView;
        }

        // Set item
        final ServiceInfo item = getItem(position);
        
        //Get Name
        final String name = item.getName();
        
        //Get MAC address
        final String macAddress = item.getServer().toString().substring(0, 12);
        
        //Get IP addresses
        final InetAddress[] addresses = item.getInetAddresses();
        StringBuilder ipBuffer = new StringBuilder();
        if (addresses.length > 0) {
            for (InetAddress address : addresses) {
            	ipBuffer.append(address);
            	ipBuffer.append(':');
            	ipBuffer.append(item.getPort());
            	ipBuffer.append(' ');
            }
        } else {
        	ipBuffer.append("(null):");
        	ipBuffer.append(item.getPort());
        }
        
        ((TextView) view.findViewById(R.id.lvi_name))			.setText(name);
        ((TextView) view.findViewById(R.id.lvi_service_type))	.setText(item.getType());
        ((TextView) view.findViewById(R.id.lvi_mac_address))	.setText(macAddress);
        ((TextView) view.findViewById(R.id.lvi_ip_address))		.setText(ipBuffer.toString());
        
        // Add button
        final ImageView button_add = ((ImageView) view.findViewById(R.id.lvi_image_added));
        
        /* Check activation status and change icon accordingly */
        SmartPlugDevice d = mContext.deviceExistInList(macAddress);
        if(d != null){
        	
        	InetAddress existingLocalAddress = d.getLocalAddress();
        	
        	final StringBuilder sb = new StringBuilder();
        	
        	if(existingLocalAddress == null){
        		sb.append(String.format(Locale.ENGLISH, "The device %s currently doesn't have a local address."
						+ "Would you like to add the local address %s to this device?", macAddress, addresses[0].getHostAddress()));
        		
        	} else if(!existingLocalAddress.getHostAddress().equals(addresses[0].getHostAddress())) {
        		//Local address mismatch, ask to confirm update
        		sb.append(String.format(Locale.ENGLISH, "The device %s currently has a known local address %s."
						+ "Would you like to add the local address %s to this device?", macAddress, existingLocalAddress.getHostAddress(), addresses[0].getHostAddress()));
        	}
        	
        	if(sb.length() != 0){
        		// Device is already in the list, but doesn't have a local address (or with a different address). Prompt the user and confirm updating address information.
        		button_add.setImageResource(R.drawable.icon_update);
        		button_add.setOnClickListener(new View.OnClickListener(){
        			@Override
        			public void onClick(View v) {
        				AlertDialog updateConfirm = new AlertDialog.Builder(mContext).create();
        				updateConfirm.setTitle(mContext.getString(R.string.dialog_title_warning));
        				updateConfirm.setMessage(sb.toString());
        				updateConfirm.setButton(DialogInterface.BUTTON_POSITIVE, "Yes", new DialogInterface.OnClickListener(){
							@Override
							public void onClick(DialogInterface dialog, int which) {
								boolean success = mContext.updateDeviceLocalAddress(addresses[0], macAddress);
		        				if(success){
		        					button_add.setImageResource(R.drawable.icon_added);
		        					button_add.setClickable(false);
		        				}else{
		        					// Nothing
		        				}
							}
        				});
        				updateConfirm.setButton(DialogInterface.BUTTON_NEGATIVE, "No", (DialogInterface.OnClickListener) null);
        				updateConfirm.show();
        			}
                });
        	} else {
        		button_add.setImageResource(R.drawable.icon_added);
            	button_add.setClickable(false);
        	}
        } else {
        	button_add.setImageResource(R.drawable.icon_add);
        	button_add.setOnClickListener(new View.OnClickListener(){
    			@Override
    			public void onClick(View v) {
    				
    				if(Common.isLoggedIn(mContext)){
    					mContext.addToCloud(name, macAddress, new DeviceOperationCompleteCallback(){

    						@Override
    						public void finalAction(String... args) {
    							// args[0] = rid
    							// args[1] = cik
    							
    							mContext.addToDeviceList(item, macAddress, args[0], args[1]);
    							button_add.setImageResource(R.drawable.icon_added);
		    					button_add.setClickable(false);
    						}
        				});
    					
    				}else{
    					mContext.addToDeviceList(item, macAddress, null, null);
    					button_add.setImageResource(R.drawable.icon_added);
    					button_add.setClickable(false);
    				}
    			}
            });
        }

        return view;
	}
}

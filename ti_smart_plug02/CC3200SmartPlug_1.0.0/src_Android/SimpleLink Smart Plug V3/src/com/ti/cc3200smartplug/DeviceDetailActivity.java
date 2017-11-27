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

package com.ti.cc3200smartplug;

import java.util.ArrayList;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import com.ti.cc3200smartplug.service.SmartPlugDevice;

/**
 * An activity representing a single Device detail screen. This activity is only
 * used on handset devices. On tablet-size devices, item details are presented
 * side-by-side with a list of items in a {@link DeviceListActivity}.
 * <p>
 * This activity is mostly just a 'shell' activity containing nothing more than
 * a {@link DeviceDetailFragment}.
 */
public class DeviceDetailActivity extends SmartPlugBaseActivity {
	
	public static final String KEY_NAME = "KEY_NAME";
	private DeviceDetailFragment fragment;
		
	private String mMac;
	
	private ArrayList<DrawerMenuItem> menuList;
	private DrawerMenuItem mMenuAddToCloud;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_device_detail);
		
		this.setTitle(getIntent().getExtras().getString(KEY_NAME, getString(R.string.default_title_device_detail)));
		mMac = getIntent().getExtras().getString(DeviceDetailFragment.KEY_MAC, "");
		
		// Pass information to fragment
		Bundle b = new Bundle();
		b.putString(DeviceDetailFragment.KEY_MAC, mMac);
		fragment = new DeviceDetailFragment();
		fragment.setArguments(b);
		
		// Drawer
		initDrawerList();
	}
	
	private void initDrawerList(){
		menuList = new ArrayList<DrawerMenuItem>();
		menuList.add(new DrawerMenuItem(R.string.change_device_details, R.drawable.ic_menu_edit, new DrawerMenuItem.Action() {
			@Override
			public void execute() {
				editDeviceDetails(fragment.getCurrentDevice(), new DeviceOperationCompleteCallback(){
					@Override
					public void finalAction(String... args) {
						fragment.device_name.setText(fragment.getCurrentDevice().getName());
					}
				});
			}
		}));
		menuList.add(new DrawerMenuItem(R.string.delete_device, R.drawable.ic_menu_delete, new DrawerMenuItem.Action() {
			@Override
			public void execute() {
				deleteDevice(fragment.getCurrentDevice(), new DeviceOperationCompleteCallback(){
					@Override
					public void finalAction(String... args) {
						finish();
						overridePendingTransition(android.R.anim.slide_in_left, android.R.anim.slide_out_right); 
					}
				});
			}
		}));
		mMenuAddToCloud = new DrawerMenuItem(R.string.add_to_cloud, R.drawable.ic_menu_add, new DrawerMenuItem.Action() {
			@Override
			public void execute() {
				try{
					final SmartPlugDevice d = fragment.getCurrentDevice();
					addToCloud(d.getName(), d.getMAC(), new DeviceOperationCompleteCallback(){
						@Override
						public void finalAction(String... args) {
							// args[0] = rid
							// args[1] = cik
							
							String rid = args[0];
							String cik = args[1];
							
							if(rid == null || cik == null)
								return;
							
							getService().updateLocalRID(d, rid);
							getService().updateLocalCIK(d, cik);
							
							// remove the mMenuAddToCloud item
							menuList.remove(mMenuAddToCloud);
							refreshDrawerMenu(menuList);
						}
					});
				}catch(Exception e){
					Toast.makeText(getApplicationContext(), "Error: fragment not present", Toast.LENGTH_SHORT).show();
				}
			}
		});
		
		menuList.add(mMenuAddToCloud);
	}
		
	@Override
	public void onBackPressed() {
	    super.onBackPressed();
	    overridePendingTransition(android.R.anim.slide_in_left, android.R.anim.slide_out_right); 
	}
		
	protected void deviceDeletionGUIChangeCallback() {
		finish();
	}

	@Override
	public int LocalConnectionUpdate(final short tcpThreadCode, SmartPlugDevice device) {
		if(fragment==null)
			return 0;
		
		//If data update for the device doesn't match current fragment, do nothing
		if(fragment.getDevice() != device)
			return 0;
		
		fragment.LocalConnectionUpdate(tcpThreadCode, device);
		return 0;
	}

	@Override
	public int CloudConnectionUpdate(short cloudThreadCode, SmartPlugDevice device, String msg) {
		if(fragment==null)
			return 0;
		
		//If data update for the device doesn't match current fragment, do nothing
		if(fragment.getDevice() != device)
			return 0;
		
		fragment.CloudConnectionUpdate(cloudThreadCode, device, msg);
		return 0;
	}

	@Override
	protected void serviceConnectedCallBack() {
		Log.d("TTT", "Check10");
		findViewById(R.id.status).setVisibility(View.GONE);
		getFragmentManager().beginTransaction().replace(R.id.device_detail_container, fragment).commit();
		
		// Drawer
		menuList.remove(mMenuAddToCloud);
		if(Common.isLoggedIn(getApplicationContext()) && (this.getService().findDeviceByMac(mMac).getRID() == null))
			menuList.add(mMenuAddToCloud);
		refreshDrawerMenu(menuList);
		
		Log.d("TTT", "Check11");
	}

	@Override
	protected void statusUpdateWiFi(boolean wifiOn, boolean cellularOn) {
		// No implementation as of right now, but might be useful in the future.
	}
}

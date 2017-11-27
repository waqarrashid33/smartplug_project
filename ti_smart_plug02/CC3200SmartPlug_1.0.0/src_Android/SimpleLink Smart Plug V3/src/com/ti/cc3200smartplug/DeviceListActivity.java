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
import java.util.HashMap;
import java.util.List;
import java.util.Locale;

import android.app.AlertDialog;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.FrameLayout;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Toast;

import com.ti.cc3200smartplug.mdns.MDNSActivity;
import com.ti.cc3200smartplug.mdns.MDNSFragment;
import com.ti.cc3200smartplug.service.SmartPlugDevice;
import com.ti.cc3200smartplug.service.SmartPlugService;
import com.ti.cc3200smartplug.service.SmartPlugService.CloudSyncCallback;

/**
 * An activity representing a list of Devices. This activity has different
 * presentations for handset and tablet-size devices. On handsets, the activity
 * presents a list of items, which when touched, lead to a
 * {@link DeviceDetailActivity} representing item details. On tablets, the
 * activity presents the list of items and item details side-by-side using two
 * vertical panes.
 * <p>
 * The activity makes heavy use of fragments. The list of items is a
 * {@link DeviceListFragment} and the item details (if present) is a
 * {@link DeviceDetailFragment}.
 */
public class DeviceListActivity extends SmartPlugBaseActivity implements DeviceListFragment.DeviceListCallbacks, GuiUpdateCallback{
	
	private static final String DEBUG_CLASS = "DeviceListActivity";
	
	/**
	 * Whether or not the activity is in two-pane mode, i.e. running on a tablet
	 * device.
	 */
	public static boolean mTwoPane;
	
	/* The frame layout for device detail content */
	public FrameLayout device_detail_container;
	
	/* Dummy content for user */
	private View home_layout;
	
	private DeviceListFragment mDLFragment;
	
	/* Get default system text size */
	public static float defaultTextSize;
	public static int defaultTextColor;
	
	private volatile boolean syncing;
	
	private ArrayList<DrawerMenuItem> menuList;
	private DrawerMenuItem mMenuLogout;
	private DrawerMenuItem mMenuLogin;
	private DrawerMenuItem mMenuAddToCloud;
    
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		// In two-pane mode, list items should be given the 'activated' state when touched.
		setContentView(R.layout.activity_device_list);
		
		if (findViewById(R.id.device_detail_container) != null) {
			// The detail container view will be present only in the large-screen layouts (res/values-large and res/values-sw600dp).
			// If this view is present, then the activity should be in two-pane mode.
			mTwoPane = true;

			// In two-pane mode, list items should be given the 'activated' state when touched.
			((DeviceListFragment) getFragmentManager().findFragmentById(R.id.device_list)).setActivateOnItemClick(true);
			device_detail_container = (FrameLayout) findViewById(R.id.device_detail_container);
			home_layout = findViewById(R.id.text_default_view);
		}
				
		//get_DLF().setActivateOnItemClick(true);

		// Set default text size
		defaultTextSize = 18; //new TextView(this).getTextSize();
		defaultTextColor = new TextView(this).getTextColors().getDefaultColor();
		
		// Drawer
		initDrawerList();
	}
	
	private void initDrawerList(){
		menuList = new ArrayList<DrawerMenuItem>();
		menuList.add(new DrawerMenuItem(R.string.add_device, R.drawable.ic_menu_add, new DrawerMenuItem.Action() {
			@Override
			public void execute() {
				startActivityForResult(new Intent(DeviceListActivity.this, AddDeviceActivity.class), REQUEST_CODE_NONE);
			}
		}));
		menuList.add(new DrawerMenuItem(R.string.mdns_search, R.drawable.ic_menu_search, new DrawerMenuItem.Action() {
			@Override
			public void execute() {
				if (DeviceListActivity.mTwoPane) {
					// In two-pane mode, show the detail view in this activity by
					// adding or replacing the detail fragment using a fragment transaction.
					MDNSFragment mdns_fragment = new MDNSFragment();
					
					//Important: This is where the fragment first get created/replaced
					(DeviceListActivity.this).device_detail_container.removeAllViews();
					
					FragmentTransaction trans = getFragmentManager().beginTransaction();
					trans.replace(R.id.device_detail_container, mdns_fragment);
					trans.addToBackStack(null);
					trans.commit();

				} else {
					// In single-pane mode, simply start the detail activity
					// for the selected item ID.
					startActivityForResult(new Intent(DeviceListActivity.this, MDNSActivity.class), REQUEST_CODE_NONE);
					overridePendingTransition(R.anim.slide_in_right, R.anim.slide_out_left); 
				}
			}
		}));
		menuList.add(new DrawerMenuItem(R.string.synchronize_with_cloud, R.drawable.ic_menu_refresh, new DrawerMenuItem.Action() {
			@Override
			public void execute() {
				if(!Common.isLoggedIn(getApplicationContext())){
					AlertDialog dialog = new AlertDialog.Builder(DeviceListActivity.this).create();
					dialog.setTitle("Not Logged In");
					dialog.setButton(DialogInterface.BUTTON_POSITIVE, "Enter Credentials", new DialogInterface.OnClickListener(){
						@Override
						public void onClick(DialogInterface dialog, int which) {
							Intent intent = new Intent(DeviceListActivity.this, LoginActivity.class);
							intent.putExtra(LoginActivity.BUNDLE_HIDE_SKIP, true);
							intent.putExtra(LoginActivity.BUNDLE_START_NEW_LIST_ACTIVITY, false);
							startActivityForResult(intent, REQUEST_CODE_NONE);
						}
					});
					dialog.setButton(DialogInterface.BUTTON_NEUTRAL, "Cancel", (DialogInterface.OnClickListener) null);
					dialog.show();
					
				}else{
					syncList();
				}
			}
		}));
				
		if(mTwoPane){
			menuList.add(new DrawerMenuItem(R.string.change_device_details, R.drawable.ic_menu_edit, new DrawerMenuItem.Action() {
				@Override
				public void execute() {
					final DeviceDetailFragment fragment = get_current_DDF();
					if(fragment == null){
						Toast.makeText(getApplicationContext(), "Please select a device from the list", Toast.LENGTH_SHORT).show();
						return;
					}
					
					editDeviceDetails(fragment.getCurrentDevice(), new DeviceOperationCompleteCallback(){
						@Override
						public void finalAction(String... args) {
							((ArrayAdapter<?>) mDLFragment.getListView().getAdapter()).notifyDataSetChanged();
							fragment.device_name.setText(fragment.getCurrentDevice().getName());
						}
					});
				}
			}));
			menuList.add(new DrawerMenuItem(R.string.delete_device, R.drawable.ic_menu_delete, new DrawerMenuItem.Action() {
				@Override
				public void execute() {
					final DeviceDetailFragment fragment = get_current_DDF();
					if(fragment == null){
						Toast.makeText(getApplicationContext(), "Please select a device from the list", Toast.LENGTH_SHORT).show();
						return;
					}
					
					deleteDevice(fragment.getCurrentDevice(), new DeviceOperationCompleteCallback(){
						@Override
						public void finalAction(String... args) {
							device_detail_container.removeAllViews();
							device_detail_container.addView(home_layout);
							LocalBroadcastManager.getInstance(DeviceListActivity.this).sendBroadcast(new Intent(DeviceListFragment.BROADCAST_DEVICE_LIST_REFRESH));
							System.gc();
						}
					});
				}
			}));
		}
		
		mMenuLogout = new DrawerMenuItem(R.string.logout, R.drawable.ic_menu_login, new DrawerMenuItem.Action() {
			@Override
			public void execute() {
				AlertDialog signOutDialog = new AlertDialog.Builder(DeviceListActivity.this).create();
				signOutDialog.setTitle(getString(R.string.dialog_title_warning));
				signOutDialog.setMessage(getString(R.string.message_confirm_sign_out));
				signOutDialog.setButton(DialogInterface.BUTTON_NEGATIVE,  "Cancel", (DialogInterface.OnClickListener) null);
				signOutDialog.setButton(DialogInterface.BUTTON_POSITIVE,  "OK", new DialogInterface.OnClickListener(){
					@Override
					public void onClick(DialogInterface dialog, int which) {
						ProgressDialog signingOutProgress = new ProgressDialog(DeviceListActivity.this);
						signingOutProgress.setTitle(getString(R.string.dialog_title_disconnecting));
						signingOutProgress.setMessage(getString(R.string.message_please_wait));
						signingOutProgress.setCancelable(false);
						signingOutProgress.setCanceledOnTouchOutside(false);
						signingOutProgress.show();
						
						int poolSize = getService().poolGetCount();
						for(int i=0; i<poolSize; i++){
							SmartPlugDevice d = getService().poolGetDevice(i);
							if(d.isCloudConnectionLive()){
								d.stopCloudThread();
							}
						}
						
						Common.setLoggedIn(DeviceListActivity.this, getApplicationContext(), false);
						signingOutProgress.dismiss();
						
						// Remove/add the following items
						menuList.add(mMenuLogin);
						menuList.remove(mMenuLogout);
						menuList.remove(mMenuAddToCloud);
						refreshDrawerMenu(menuList);
					}
				});
				signOutDialog.show();
			}
		});
		
		mMenuLogin = new DrawerMenuItem(R.string.login, R.drawable.ic_menu_login, new DrawerMenuItem.Action() {
			@Override
			public void execute() {
				Intent intent = new Intent(DeviceListActivity.this, LoginActivity.class);
				intent.putExtra(LoginActivity.BUNDLE_HIDE_SKIP, true);
				intent.putExtra(LoginActivity.BUNDLE_START_NEW_LIST_ACTIVITY, false);
				startActivityForResult(intent, REQUEST_CODE_NONE);
			}
		});
		
		mMenuAddToCloud = new DrawerMenuItem(R.string.add_to_cloud, R.drawable.ic_menu_add, new DrawerMenuItem.Action() {
			@Override
			public void execute() {
				try{
					final SmartPlugDevice d = get_current_DDF().getCurrentDevice();
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
							LocalBroadcastManager.getInstance(getApplicationContext()).sendBroadcast(new Intent(DeviceListFragment.BROADCAST_DEVICE_LIST_REFRESH));
							
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
		
		if(Common.isLoggedIn(getApplicationContext())){
			menuList.add(mMenuLogout);
			
			DeviceDetailFragment fragment = get_current_DDF();
			if((fragment != null) && (fragment.getDevice().getRID() == null))
				menuList.add(mMenuAddToCloud);
			
		}else{
			menuList.add(mMenuLogin);
		}
	}
	
	@Override
	public void onResume(){
		super.onResume();
		
		mDLFragment = ((DeviceListFragment) getFragmentManager().findFragmentById(R.id.device_list));
		
		// Set appropriate drawer views based on signed in status
		menuList.remove(mMenuLogin);
		menuList.remove(mMenuLogout);
		menuList.remove(mMenuAddToCloud);
		
		if(Common.isLoggedIn(getApplicationContext())){
			menuList.add(mMenuLogout);
			
			DeviceDetailFragment fragment = get_current_DDF();
			if((fragment != null) && (fragment.getDevice().getRID() == null))
				menuList.add(mMenuAddToCloud);
			
		}else{
			menuList.add(mMenuLogin);
		}
		
		// Refresh drawer
		refreshDrawerMenu(menuList);
		
		/* Cloud device loading */
		syncing = false;
		//cloudDevicesSync();
	}
	
	@Override
	public void onItemSelected(int position) {
		
		Log.d("TTT", "Check-4");
		
		SmartPlugDevice device = getService().poolGetDevice(position);
		
		if (mTwoPane) {
			//If clicking on the currently displayed item, do nothing.
			if(get_current_DDF() != null){
				if(get_current_DDF().getDevice() == device){
					return;
				}
			}
			
			// In two-pane mode, show the detail view in this activity by
			// adding or replacing the detail fragment using a fragment transaction.
			Bundle b = new Bundle();
			b.putString(DeviceDetailFragment.KEY_MAC, device.getMAC());
			DeviceDetailFragment detail_fragment = new DeviceDetailFragment();
			detail_fragment.setArguments(b);
			
			//Important: This is where the fragment first get created/replaced
			device_detail_container.removeAllViews();
			FragmentTransaction trans = getFragmentManager().beginTransaction();
			trans.replace(R.id.device_detail_container, detail_fragment);
			trans.addToBackStack(null);
			trans.commit();

		} else {
			Log.d("TTT", "Check-3");
			// In single-pane mode, simply start the detail activity for the selected item ID.
			Intent intent = new Intent(getApplicationContext(), DeviceDetailActivity.class);
			intent.putExtra(DeviceDetailActivity.KEY_NAME, device.getMAC());
			intent.putExtra(DeviceDetailFragment.KEY_MAC, device.getMAC());
			intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
			startActivityForResult(intent, REQUEST_CODE_NONE);
			
			Log.d("TTT", "Check-2");
			
			// Set transition animation
			overridePendingTransition(R.anim.slide_in_right, R.anim.slide_out_left);
			Log.d("TTT", "Check-1");
		}
	}
		
	private void syncList() {
		if(!syncing){
			syncing = true;
			
			final ProgressDialog pd = new ProgressDialog(this);
			pd.setTitle("Synchronizing...");
			pd.setMessage("Please wait.");
			pd.setCancelable(false);
			pd.setIndeterminate(true);
			pd.show();
			
			getService().syncCloudDevices(new CloudSyncCallback(){
				@Override
				public void onSyncComplete(
						boolean success,
						String message,
						final ArrayList<SmartPlugDevice> existLocal_noAssoUser,
						final ArrayList<SmartPlugDevice> existLocal_isNotCurAssoUser) {
					
					syncing = false;
					pd.dismiss();
					
					if(success){
						Toast.makeText(DeviceListActivity.this, "Cloud synchronization complete", Toast.LENGTH_SHORT).show();
						
						// Ask user for confirming adding to devices without an associated user.
						if(!existLocal_noAssoUser.isEmpty()){
							String[] items = new String[existLocal_noAssoUser.size()];
							
							for(int i=0; i<existLocal_noAssoUser.size(); i++){
								SmartPlugDevice d = existLocal_noAssoUser.get(i);
								items[i] = String.format(Locale.ENGLISH,  "%s (%s)", d.getName(), d.getMAC());
							}
							
							final ArrayList<Boolean> selectedItem = new ArrayList<Boolean>();
							for(int i=0; i<existLocal_noAssoUser.size(); i++){
								selectedItem.add(false);
							}
							
							AlertDialog.Builder d1 = new AlertDialog.Builder(DeviceListActivity.this);
							d1.setTitle(getString(R.string.dialog_title_confirm) + ": update these devices");
							d1.setMultiChoiceItems(items, null, new DialogInterface.OnMultiChoiceClickListener(){
								@Override
								public void onClick(DialogInterface dialog, int which, boolean isChecked) {
									selectedItem.set(which, isChecked);
								}
							});
							d1.setPositiveButton("OK", new DialogInterface.OnClickListener() {
								@Override
								public void onClick(DialogInterface dialog, int which) {
									// Update checked devices
									for(int i=0; i<existLocal_noAssoUser.size(); i++){
										if(selectedItem.get(i)){
											SmartPlugDevice targetDevice = getService().findDeviceByMac(existLocal_noAssoUser.get(i).getMAC());
											getService().updateLocalName(targetDevice, existLocal_noAssoUser.get(i).getName());
											getService().updateLocalRID(targetDevice, existLocal_noAssoUser.get(i).getRID());
											getService().updateLocalCIK(targetDevice, existLocal_noAssoUser.get(i).getCIK());
											
											LocalBroadcastManager.getInstance(DeviceListActivity.this).sendBroadcast(new Intent(DeviceListFragment.BROADCAST_DEVICE_LIST_REFRESH));
										}
									}
								}
							});
							d1.setNegativeButton("Cancel", (DialogInterface.OnClickListener) null);
							d1.create().show();
						}
						
						// Warn user for the local device being associated with another user already.
						if(!existLocal_isNotCurAssoUser.isEmpty()){
							
							ListView lv = new ListView(DeviceListActivity.this);
							
							String[] from = new String[] {"col_1", "col_2"};
							List<HashMap<String, String>> list = new ArrayList<HashMap<String, String>>();
							
							for(SmartPlugDevice d : existLocal_isNotCurAssoUser){
								HashMap<String, String> item = new HashMap<String, String>();
								item.put(from[0], String.format(Locale.ENGLISH,  "%s (%s)", d.getName(), d.getMAC()));
								list.add(item);
							}
							
							SimpleAdapter a = new SimpleAdapter(
									DeviceListActivity.this,
									list,
									android.R.layout.simple_list_item_1,
									from,
									new int[] {android.R.id.text1});
							lv.setAdapter(a);
							
							AlertDialog d2 = new AlertDialog.Builder(DeviceListActivity.this).create();
							d2.setTitle(getString(R.string.dialog_title_warning));
							d2.setView(lv);
							d2.setButton(DialogInterface.BUTTON_POSITIVE, "OK", (DialogInterface.OnClickListener) null);
							d2.show();
						}
						
					}else{
						Toast.makeText(getApplicationContext(), "Cloud synchronization error: " + message, Toast.LENGTH_SHORT).show();
						AlertDialog dialog = new AlertDialog.Builder(DeviceListActivity.this).create();
						dialog.setTitle("Log in failed");
						dialog.setMessage("Cloud synchronization error: "  + message
								+ "\nThis mobile device may not be connected to the internet or your credentials has been changed."
								+ "\nPlease try again later if you see this message frequently");
						dialog.setButton(DialogInterface.BUTTON_POSITIVE, "Retry", new DialogInterface.OnClickListener(){
							@Override
							public void onClick(DialogInterface dialog, int which) {
								syncList(); //Calls itself again
							}
						});
						dialog.setButton(DialogInterface.BUTTON_NEUTRAL, "Enter Credentials", new DialogInterface.OnClickListener(){
							@Override
							public void onClick(DialogInterface dialog, int which) {
								Intent intent = new Intent(DeviceListActivity.this, LoginActivity.class);
								intent.putExtra(LoginActivity.BUNDLE_HIDE_SKIP, true);
								intent.putExtra(LoginActivity.BUNDLE_START_NEW_LIST_ACTIVITY, false);
								startActivityForResult(intent, REQUEST_CODE_NONE);
							}
						});
						dialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel", (DialogInterface.OnClickListener) null);
						dialog.show();
					}
				}
			});
		}else{
			Toast.makeText(getApplicationContext(), "Cloud synchronization already in progress", Toast.LENGTH_SHORT).show();
		}
	}
	
	@Override
	public int LocalConnectionUpdate(final short tcpThreadCode, SmartPlugDevice device) {
		if(!mTwoPane)
			return 0;
		
		DeviceDetailFragment f = get_current_DDF();
		
		// No fragment brought to front -> do not update fragment
		if(f==null)
			return 0;
		
		// Current fragment isn't a DeviceDetailFragment -> do not update fragment
		if(!(f instanceof DeviceDetailFragment))
			return 0; 
		
		//If data update for the device doesn't match current fragment, do nothing
		if(f.getDevice() != device)
			return 0;
		
		f.LocalConnectionUpdate(tcpThreadCode, device);
		return 0;
	}
	
	@Override
	public int CloudConnectionUpdate(short cloudThreadCode, SmartPlugDevice device, String msg) {
		if(!mTwoPane)
			return 0;
		
		DeviceDetailFragment f = get_current_DDF();
		
		// No fragment brought to front -> do not update fragment
		if(f==null)
			return 0;
		
		// Current fragment isn't a DeviceDetailFragment -> do not update fragment
		if(!(f instanceof DeviceDetailFragment))
			return 0; 
		
		//If data update for the device doesn't match current fragment, do nothing
		if(f.getDevice() != device)
			return 0;
		
		f.CloudConnectionUpdate(cloudThreadCode, device, msg);
		return 0;
	}
	
	/**
	 * Get the current Device Detail Fragment
	 * @return fragment
	 */
	public DeviceDetailFragment get_current_DDF(){
		if(!mTwoPane)
			return null;
			
		Fragment fragment = getFragmentManager().findFragmentById(R.id.device_detail_container);
		if(fragment instanceof DeviceDetailFragment){
			return (DeviceDetailFragment) fragment;
		}
		
		return null;
	}

	@Override
	protected void serviceConnectedCallBack() {
		SmartPlugService s = getService();
		mDLFragment.setCustomAdapter(new DeviceListArrayAdapter(s, mDLFragment, s.getPoolForListAdapter()));
		mDLFragment.setListShown(true);
		
		Intent intent = getIntent();
		boolean autoRefresh = intent.getBooleanExtra(LoginActivity.BUNDLE_LIST_FRAGMENT_REFRESH_DEVICES, false);
		if(autoRefresh){
			syncList();
		}
		intent.removeExtra(LoginActivity.BUNDLE_LIST_FRAGMENT_REFRESH_DEVICES);
	}

	@Override
	protected void statusUpdateWiFi(boolean wifiOn, boolean cellularOn) {
		// No implementation as of right now, but might be useful in the future.
	}
}

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

package com.ti.cc3200smartplug;

import java.net.HttpURLConnection;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;
import java.util.Locale;

import javax.jmdns.ServiceInfo;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBarDrawerToggle;
import android.text.TextUtils;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.exosite.api.ExoCallback;
import com.exosite.api.ExoException;
import com.exosite.api.portals.HTTPResult;
import com.exosite.api.portals.Portals;
import com.ti.cc3200smartplug.mdns.MDNSArrayAdapter;
import com.ti.cc3200smartplug.service.SmartPlugDevice;
import com.ti.cc3200smartplug.service.SmartPlugService;

/**
 * Base Activity of Smart Plug.
 *
 */
public abstract class SmartPlugBaseActivity extends Activity implements GuiUpdateCallback, MDNSArrayAdapter.DeviceFinder {
	
	private String DEBUG_CLASS = "SmartPlugBaseActivity";
	
	protected static final int REQUEST_CODE_NONE	= 0;
	protected static final int RESULT_CODE_NORMAL	= 0;
	protected static final int RESULT_CODE_FINISH	= 1;
	
	/**
	 * The interface that contains a callback function when the device property change operation completes.<br/>
	 * This is used mostly by renaming and deleting for GUI handling.
	 */
	public interface DeviceOperationCompleteCallback{
		public void finalAction(String... args);
	}
	
	/**
	 * The Reference to the SmartPlugService
	 */
	private SmartPlugService SPS;
	
	private boolean SPSIsBound;
	
	/* Setup WiFi & Power related instances */
	protected WifiManager wifiManager;
	protected ConnectivityManager connManager;
	
	// The receiver for WiFi status
	private BroadcastReceiver mWiFiStateReceiver;
	
	protected ServiceConnection mConnection;
	
//	public static final String BROADCAST_SIGNED_IN_STATUS_CHANGE = "signed_in_status_change";
	private View warningMessage;
	
	protected OnClickListener warningViewClick = new OnClickListener(){
		@Override
		public void onClick(View v) {
			AlertDialog warningDialog = new AlertDialog.Builder(SmartPlugBaseActivity.this).create();
			warningDialog.setTitle(getString(R.string.dialog_title_warning));
			warningDialog.setMessage(getString(R.string.message_not_signed_in));
			warningDialog.setButton(DialogInterface.BUTTON_NEUTRAL, "OK", (DialogInterface.OnClickListener) null);
			warningDialog.show();
		}
	};
	
	// Custom menu items IDs
	
	// Drawer
	private ArrayList<DrawerMenuItem>	essentialList;
	private ArrayList<DrawerMenuItem>	menuList;
	private DrawerMenuAdapter			mDrawerAdapter;
    private ActionBarDrawerToggle		mDrawerToggle;
	
	//================================= Activity Lifecycle =================================
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		Log.d("TTT", "Check1");
		
		//Fix orientation. Prevent ANY orientation from happening.
		//10" tablet - Landscape
		//7"  tablet - Landscape
		//<7" phones - Portrait
		Common.fixOrientation(this);
		
		connManager = (ConnectivityManager) getSystemService(CONNECTIVITY_SERVICE);
		wifiManager = (WifiManager) getSystemService(WIFI_SERVICE);
		
		SPSIsBound = false;
		
		// Register the receiver for monitoring connectivity
		mWiFiStateReceiver = new BroadcastReceiver(){
			@Override
			public void onReceive(Context context, Intent intent) {
				Log.w(DEBUG_CLASS, "WiFi Status changed");
				
				ConnectivityManager connManager = (ConnectivityManager) getSystemService(android.content.Context.CONNECTIVITY_SERVICE);
				
				boolean WiFi = connManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI).isConnected();
				boolean Cellular = false;
				NetworkInfo infoCellular = connManager.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
				if(infoCellular != null){
					Cellular = infoCellular.isConnected();
				}
				
				statusUpdateWiFi(WiFi, Cellular);
			}
		};
		
		/* Check our service.
		 * If running, retrieve and link all data to the service
		 * If not running, start a new service */
		if(!SmartPlugService.isMyServiceRunning(this)){
			Intent i = new Intent(this, SmartPlugService.class);
			startService(i);
		}
		
		signedInStatusChanged();
		
		mConnection = new ServiceConnection() {
		    public void onServiceConnected(ComponentName className, IBinder service) {
				// This is called when the connection with the service has been
				// established, giving us the service object we can use to
				// interact with the service.  Because we have bound to a explicit
				// service that we know is running in our own process, we can
				// cast its IBinder to a concrete class and directly access it.
		    	
		    	Log.d(DEBUG_CLASS, "mConnection: onServiceConnected");
		    	
				SPS = ((SmartPlugService.LocalBinder)service).getService(SmartPlugBaseActivity.this);
				Common.setService(SPS);
			        
				//Populate the list when connected
				Log.w("List Update", "Send broadcast message");
				LocalBroadcastManager.getInstance(SmartPlugBaseActivity.this).sendBroadcast(new Intent(DeviceListFragment.BROADCAST_DEVICE_LIST_REFRESH));
				
				SPSIsBound = true;
				serviceConnectedCallBack();
		    }

		    public void onServiceDisconnected(ComponentName className) {
		        // This is called when the connection with the service has been
		        // unexpectedly disconnected -- that is, its process crashed.
		        // Because it is running in our same process, we should never
		        // see this happen.
		    	SPS = null;
		    	SPSIsBound = false;
		    }
		};
		
		Log.d("TTT", "Check2");
	}
	
	@Override
	protected void onPostCreate(Bundle savedInstanceState) {
		super.onPostCreate(savedInstanceState);
		initDrawer();
		mDrawerToggle.syncState();
	}
	
	@Override
	protected void onStart(){
		super.onStart();
		
		Log.d("TTT", "Check3");
		
		// Bind service
		boolean success = getApplicationContext().bindService(new Intent(this, SmartPlugService.class), mConnection, 0);
		
		while(!success){
			Log.e(DEBUG_CLASS, "Service bound " + Boolean.toString(success));
			success = getApplicationContext().bindService(new Intent(this, SmartPlugService.class), mConnection, 0);
			try {
				Thread.sleep(250);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
		
		Log.d("TTT", "Check4");
		
		warningMessage = findViewById(R.id.warning_message_not_signed_in);
		warningMessage.setOnClickListener(new OnClickListener(){
			@Override
			public void onClick(View v) {
				AlertDialog warningDialog = new AlertDialog.Builder(SmartPlugBaseActivity.this).create();
				warningDialog.setTitle(getString(R.string.dialog_title_warning));
				warningDialog.setMessage(getString(R.string.message_deletion_warning));
				warningDialog.setButton(DialogInterface.BUTTON_NEUTRAL, "OK", (DialogInterface.OnClickListener)null);
				warningDialog.show();
			}
		});
	}
	
	@Override
	protected void onResume(){
		super.onResume();
		
		Log.d("TTT", "Check5");
		
		signedInStatusChanged();
//		registerReceiver((SignedInStateChangeReceiver), new IntentFilter(BROADCAST_SIGNED_IN_STATUS_CHANGE));
		registerReceiver(mWiFiStateReceiver, new IntentFilter("android.net.conn.CONNECTIVITY_CHANGE"));
		
		Log.d("TTT", "Check6");
	}
	
	@Override
	protected void onPause(){
		try{unregisterReceiver(mWiFiStateReceiver);}
		catch (IllegalArgumentException e){}
		
		super.onPause();
	}
	
	@Override
	protected void onStop(){
		super.onStop();
		
		if(SPSIsBound){
			getApplicationContext().unbindService(mConnection);
			SPSIsBound = false;
		}
	}
	
	@Override
	protected void onDestroy(){
		super.onDestroy();
	}
	
	@Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {		
	    switch (resultCode) {
		    case RESULT_CODE_FINISH:
		        this.finish();
		        break;
	    }
	    super.onActivityResult(requestCode, resultCode, data);
	}
	
	// ================================= Drawer Creation =================================
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle the "Hamburger" icon
		if (mDrawerToggle.onOptionsItemSelected(item))
	          return true;
	    
		return super.onOptionsItemSelected(item);
	}
	
	protected void initDrawer(){
		
		essentialList = new ArrayList<DrawerMenuItem>();
		menuList = new ArrayList<DrawerMenuItem>();
		
		essentialList.add(new DrawerMenuItem(R.string.about, R.drawable.ic_menu_info_details, new DrawerMenuItem.Action() {
			@Override
			public void execute() {
				AlertDialog aboutDialog = new AlertDialog.Builder(SmartPlugBaseActivity.this).create();
				aboutDialog.setTitle("About Smart Plug");
				
				String verNumber = "";
				try {
					verNumber = getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
				} catch (NameNotFoundException e) {	}
				
				View v = getLayoutInflater().inflate(R.layout.dialog_about, null, false);
				((TextView)(v.findViewById(R.id.txt_dialog_version))).setText("Version: " + verNumber);
				((Button)(v.findViewById(R.id.about_btn_privacy_policy))).setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("http://www.ti.com/corp/docs/legal/privacy.shtml"));
						startActivity(browserIntent);
					}
				});
				((Button)(v.findViewById(R.id.about_btn_terms_of_use))).setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("http://www.ti.com/corp/docs/legal/termsofuse.shtml"));
						startActivity(browserIntent);
					}
				});
				aboutDialog.setView(v);
				aboutDialog.setButton(AlertDialog.BUTTON_POSITIVE, "OK", (DialogInterface.OnClickListener) null);
				aboutDialog.show();
			}
		}));
		essentialList.add(new DrawerMenuItem(R.string.exit, R.drawable.ic_menu_close_clear_cancel, new DrawerMenuItem.Action() {
			@Override
			public void execute() {
				AlertDialog exitDialog = new AlertDialog.Builder(SmartPlugBaseActivity.this).create();
				exitDialog.setTitle("Exit Application");
				exitDialog.setMessage("This will terminate all its background service and exit the application. Are you sure?");	
				exitDialog.setButton(AlertDialog.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						if(isServiceBound()){
							getApplicationContext().unbindService(mConnection);
							setServiceBound(false);
						}
						stopService(new Intent(SmartPlugBaseActivity.this, SmartPlugService.class));
						
						//android.os.Process.killProcess(android.os.Process.myPid());
						setResult(RESULT_CODE_FINISH);
						finish();
					}
				});
				exitDialog.setButton(AlertDialog.BUTTON_NEGATIVE, "Cancel", (DialogInterface.OnClickListener) null);
				exitDialog.show();
			}
		}));
		
		final DrawerLayout mDrawerLayout = (DrawerLayout) findViewById(R.id.drawer_layout);
        final ListView mDrawerList = (ListView) findViewById(R.id.left_drawer);
        //mDrawerList.setBackgroundColor(getResources().getColor(R.color.holo_light_background_color));

        // Set the adapter for the list view
        menuList.addAll(essentialList);
        mDrawerAdapter = new DrawerMenuAdapter(this, menuList);
        mDrawerList.setAdapter(mDrawerAdapter);
        // Set the list's click listener
        mDrawerList.setOnItemClickListener(new ListView.OnItemClickListener(){
			@Override
			public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
				DrawerMenuItem.Action action = menuList.get(position).action;
				if(action != null){
					action.execute();
				}
				mDrawerLayout.closeDrawer(mDrawerList);
			}
        });
        
        mDrawerToggle = new ActionBarDrawerToggle(this, mDrawerLayout, R.string.drawer_open, R.string.drawer_close) {
        	@Override
            public void onDrawerOpened(View drawerView) {
                super.onDrawerOpened(drawerView);
                invalidateOptionsMenu(); // creates call to onPrepareOptionsMenu()
            }
        	
        	@Override
            public void onDrawerClosed(View view) {
                super.onDrawerClosed(view);
                invalidateOptionsMenu(); // creates call to onPrepareOptionsMenu()
            }
        };
        
        mDrawerLayout.setDrawerListener(mDrawerToggle);
        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setHomeButtonEnabled(true);
	}
	
	protected void refreshDrawerMenu(ArrayList<DrawerMenuItem> toAdd){
		
		menuList.retainAll(essentialList);
		
		// Display Email Account
		String email = "Not Singed In";
		if(Common.isLoggedIn(getApplicationContext())){
			email = Common.getEmail(getApplicationContext());
		}
		
		menuList.add(0, new DrawerMenuItem(email, R.drawable.ic_menu_star, null));
		
		// Add Activity-specific list
		if(toAdd != null){
			menuList.addAll(1, toAdd);
		}
		
		mDrawerAdapter.notifyDataSetChanged();
	}
	
	//================================= Service Connection =================================
	
	/**
	 * A callback function once service is connected.
	 */
	protected abstract void serviceConnectedCallBack();
	
	/**
	 * Check whether service is bound or not.
	 * @return
	 */
	protected boolean isServiceBound(){
		return SPSIsBound;
	}
	
	/**
	 * Set service bound status
	 * @param b
	 */
	protected void setServiceBound(boolean b){
		SPSIsBound = b;
	}
	
	/**
	 * Get the service. Should always check if service is running before acquiring the service
	 * @return The SmartPlugService. If no such service, null is returned.
	 */
	public SmartPlugService getService(){
		return SPS;
	}
	
	//====================== Common methods for all children classes ======================
	
	protected abstract void statusUpdateWiFi(boolean wifiOn, boolean cellularOn);
	
	protected void signedInStatusChanged(){
		View warningMessage = findViewById(R.id.warning_message_not_signed_in);
		
		if(warningMessage == null)
			return;
    	
    	// User is not signed in, show a warning sign.
    	if(Common.isLoggedIn(getApplicationContext())){
    		warningMessage.setVisibility(View.GONE);
    	}else{
    		warningMessage.setVisibility(View.VISIBLE);
    	}
	}
	
	public void addToCloud(final String name, final String mac, final DeviceOperationCompleteCallback callback){
		AlertDialog d = new AlertDialog.Builder(this).create();
		d.setTitle(getString(R.string.dialog_title_confirm));
		
		View v = getLayoutInflater().inflate(R.layout.dialog_add_to_cloud, null, false);
		((TextView)(v.findViewById(R.id.textview_msg_dialog_addtocloud))).setText(getString(R.string.message_confirm_add_to_cloud));
		final SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        try {
        	// Portal Spinner
        	final JSONArray mPortalList = new JSONArray(sharedPreferences.getString("portal_list", "[]"));
			final Spinner mPortalSpinner = (Spinner) v.findViewById(R.id.spinner_portal_dialog_addtocloud);
			List<String> SpinnerArray = new ArrayList<String>();
			    try {
			        for (int i = 0; i < mPortalList.length(); i++) {
			            JSONObject portal = mPortalList.getJSONObject(i);
			            SpinnerArray.add(String.format("%s (CIK: %s...)",
			            portal.getString("name"),
			            portal.getString("key").substring(0,8)));
			    }
			} catch (JSONException e) {
			    Log.e(DEBUG_CLASS, "Exception while populating spinner: " + e.toString());
			}
			
			ArrayAdapter<String> adapter = new ArrayAdapter<String>(
			        this,
			        android.R.layout.simple_spinner_item,
			        SpinnerArray);
			adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
			mPortalSpinner.setAdapter(adapter);
			
			// Dialog creation continues
			d.setView(v);
			d.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel", (DialogInterface.OnClickListener) null);
			d.setButton(DialogInterface.BUTTON_NEUTRAL, "NO",  new DialogInterface.OnClickListener(){
				@Override
				public void onClick(DialogInterface dialog, int which) {
					callback.finalAction(null, null);
				}
			});
			d.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener(){
				@Override
				public void onClick(DialogInterface dialog, int which) {
					final ProgressDialog pd = new ProgressDialog(SmartPlugBaseActivity.this);
					pd.setTitle("Synchronizing");
					pd.setMessage("Please Wait...");
					pd.setCancelable(false);
					pd.setCanceledOnTouchOutside(false);
					pd.show();
					
			        String email = sharedPreferences.getString("email", null);
		            String password = sharedPreferences.getString("password", null);
	
			        try {
			            String portalID = mPortalList.getJSONObject(mPortalSpinner.getSelectedItemPosition()).getString("rid");
			            Portals.addDeviceInBackground(portalID, SmartPlugDevice.DEFAULT_VENDOR, SmartPlugDevice.DEFAULT_MODEL, mac, name, email, password, new ExoCallback<JSONObject>(){
							@Override
							public void done(JSONObject newDevice, ExoException e) {
								pd.dismiss();
								if (newDevice != null) {
									// New device added
									String rid = null;
									String cik = null;
									try {
										rid = newDevice.getString("rid");
										cik = newDevice.getString("cik");
										Toast.makeText(getApplicationContext(), "Success", Toast.LENGTH_SHORT).show();
									} catch (JSONException e1) {
										Toast.makeText(getApplicationContext(),
												String.format(Locale.ENGLISH, "Error: %s\nDevice is added locally only.", e1.getMessage()),
												Toast.LENGTH_SHORT).show();
									}
									callback.finalAction(rid, cik);
									
								} else {
									// Error when adding device
									Toast.makeText(getApplicationContext(),
											String.format(Locale.ENGLISH, "Error: %s\nDevice is added locally only.", e.getMessage()),
											Toast.LENGTH_SHORT).show();
								}
							}
						});
			        } catch (JSONException e) {
			            Log.e(DEBUG_CLASS, "portal_list shared preference was not set.");
			        }
				}
			});
			d.show();
        } catch (JSONException e) {
            Log.e(DEBUG_CLASS, "portal_list shared preference was not set.");
        }
	}
		
	//====================== Device Operation ======================
	
	/**
	 * Protected method for editing device details.
	 * @param device - device to be edited
	 * @param callback - used for GUI update
	 */
	protected void editDeviceDetails(final SmartPlugDevice device, final DeviceOperationCompleteCallback callback){
		
		//new
		final AlertDialog editdialog = new AlertDialog.Builder(this).create();
		editdialog.setTitle(R.string.dialog_title_edit_details);
		View layout = getLayoutInflater().inflate(R.layout.dialog_edit_device, null, false);
		editdialog.setView(layout);
		editdialog.setButton(DialogInterface.BUTTON_POSITIVE, "Update", (DialogInterface.OnClickListener) null);
		editdialog.setButton(DialogInterface.BUTTON_NEUTRAL, "Re-Enable Device", (DialogInterface.OnClickListener) null);
		editdialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel", (DialogInterface.OnClickListener) null);
		editdialog.setOnShowListener(new DialogInterface.OnShowListener() {
			@Override
			public void onShow(DialogInterface dialog) {
				final EditText fieldName			= (EditText) editdialog.findViewById(R.id.edittext_device_name);
				EditText fieldMac				= (EditText) editdialog.findViewById(R.id.edittext_device_serial_number);
				fieldMac.setEnabled(false);	//Don't want user to change this
				final EditText fieldLocalAddr	= (EditText) editdialog.findViewById(R.id.edittext_manual_local_address);
				
				EditText fieldCik				= (EditText) editdialog.findViewById(R.id.editText_dialog_edit_cik);
				fieldCik.setEnabled(false);
				Spinner fieldConnPref			= (Spinner)	editdialog.findViewById(R.id.spinner_dialog_edit_connpref);
				fieldConnPref.setSelection(device.getConnectionPreference());
				
				// Set Texts
				fieldName.setText(device.getName());
				fieldMac.setText(device.getMAC());
				
				if(device.getLocalAddress() != null)
					fieldLocalAddr.setText(device.getLocalAddress().getHostAddress());
				
				fieldCik.setText(device.getCIK());
				
				// Button click actions
				Button update = editdialog.getButton(AlertDialog.BUTTON_POSITIVE);
				Button reenable = editdialog.getButton(AlertDialog.BUTTON_NEUTRAL);
				
				update.setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						boolean cancel = false;
						View focusView = null;
						
						// Check for a valid name
						final String newName = fieldName.getText().toString().trim();
						if(TextUtils.isEmpty(newName)){
							fieldName.setError(getString(R.string.error_field_required));
							cancel = true;
							focusView = fieldName;
						}
						
						// Check for a valid local address
						final String localAddressString = fieldLocalAddr.getText().toString().trim();
						if(TextUtils.isEmpty(localAddressString)){
							// Nothing. Thie field can be empty
						}else if(!Common.checkTextRegex(localAddressString, Common.REG_EXP_IP_ADDRESS)){
							fieldLocalAddr.setError(getString(R.string.error_field_invalid));
							cancel = true;
							focusView = fieldName;
						}
						
						if (cancel) {
							// There was an error; don't attempt login and focus the first form field with an error.
							focusView.requestFocus();
						} else if(device.getLocalConnectionStatus() == SmartPlugDevice.LOCAL_CONNECTION_LIVE){
							Toast.makeText(getApplicationContext(), getString(R.string.error_local_running), Toast.LENGTH_SHORT).show();
						} else {
							final ProgressDialog pd = new ProgressDialog(SmartPlugBaseActivity.this);
							pd.setTitle("Updating");
							pd.setMessage("Please Wait...");
							pd.setCancelable(false);
							pd.setCanceledOnTouchOutside(false);
							pd.show();
							
							editCloudInfo(device, newName, new EditCloudFinish(){
								@Override
								public void editLocal(boolean success, String errorMessage) {
									if(success){
										
										// Local Name Update
										getService().updateLocalName(device, newName);
										
										// Local Address Update
										if(localAddressString.equals("")){
											getService().updateLocalAddress(device, (String) null);
										}else{
											getService().updateLocalAddress(device, localAddressString);
										}
										
										// Connection Preference Update
										getService().updateConnectionPreference(
												device,
												(short)((Spinner) editdialog.findViewById(R.id.spinner_dialog_edit_connpref)).getSelectedItemPosition());
										
										callback.finalAction();
										
										pd.dismiss();
										editdialog.dismiss();
										
									} else {
										pd.dismiss();
										AlertDialog editFailDialog = new AlertDialog.Builder(SmartPlugBaseActivity.this).create();
										editFailDialog.setTitle(getString(R.string.dialog_title_error));
										editFailDialog.setMessage(errorMessage);
										editFailDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", (DialogInterface.OnClickListener) null);
										editFailDialog.show();
									}
								}
							});
						}
					}
				});
				reenable.setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						// TODO - [Exosite API Limitation]
						AlertDialog reEnableNotAvailableDialog = new AlertDialog.Builder(SmartPlugBaseActivity.this).create();
						reEnableNotAvailableDialog.setTitle(getString(R.string.dialog_title_warning));
						reEnableNotAvailableDialog.setMessage(getString(R.string.message_reenable_fail));
						reEnableNotAvailableDialog.setButton(DialogInterface.BUTTON_NEUTRAL, "OK", (DialogInterface.OnClickListener) null);
						reEnableNotAvailableDialog.show();
					}
				});
			}
		});
		editdialog.show();
		editdialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);
	}
	
	private void editCloudInfo(SmartPlugDevice device, String newName, final EditCloudFinish callback){
		if(device.getRID() == null){
			// No cloud record, only update locally
			callback.editLocal(true, null);
		}
		
		SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
		String email = sharedPreferences.getString(Common.PREFERENCE_EMAIL, "");
		String password = sharedPreferences.getString(Common.PREFERENCE_PASSWORD, "");
		Portals.callInBackground(
				Portals.HTTP_PUT,
				"devices/" + device.getRID(),
				String.format("{\"info\":{\"description\":{\"name\":\"%s\"}}}", newName),
				email,
				password,
				new ExoCallback<HTTPResult>(){

			@Override
			public void done(HTTPResult result, ExoException e) {
				if(result != null){
					if (result.responseCode == HttpURLConnection.HTTP_OK) {
						callback.editLocal(true, null);
					} else {
						callback.editLocal(false, "Cloud Connection Error: " + result.responseCode);
					}
				}else{
					callback.editLocal(false, "Cloud Connection Error: " + e.getMessage());
				}
			}
		});
	}
	
	private interface EditCloudFinish{
		public void editLocal(boolean success, String errorMessage);
	}
		
	/**
	 * Delete the device
	 * @param device - the SmartPlugDevice to be deleted
	 * @param callback - a callback once deletion completes. Mainly for GUI cleanup
	 */
	protected void deleteDevice(final SmartPlugDevice device, final DeviceOperationCompleteCallback callback){
		AlertDialog DeleteWarningDialog = new AlertDialog.Builder(this).create();
		DeleteWarningDialog.setTitle("Device Delete Confirmation");
		DeleteWarningDialog.setMessage("Are you sure you want to delete this device?");
		DeleteWarningDialog.setButton(AlertDialog.BUTTON_POSITIVE, "OK",  new DialogInterface.OnClickListener(){
			@Override
			public void onClick(DialogInterface dialog, int which) {
				
				final ProgressDialog deleteProgress = new ProgressDialog(SmartPlugBaseActivity.this);
				deleteProgress.setTitle("Deleting");
				deleteProgress.setMessage("Please Wait...");
				deleteProgress.setCancelable(false);
				deleteProgress.setCanceledOnTouchOutside(false);
				deleteProgress.show();
				
				device.stopLocalThread();
				device.stopCloudThread();
				
				// Device exists in cloud, perform cloud deletion
				if(device.getRID() != null){
					SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
					String email = sharedPreferences.getString(Common.PREFERENCE_EMAIL, "");
					String password = sharedPreferences.getString(Common.PREFERENCE_PASSWORD, "");
					Portals.callInBackground(Portals.HTTP_DELETE, "devices/" + device.getRID(), null, email, password, new ExoCallback<HTTPResult>(){
						@Override
						public void done(HTTPResult result, ExoException e) {
							if(result!=null){
								if (result.responseCode == HttpURLConnection.HTTP_NO_CONTENT) {
									callback.finalAction();
								}else{
									AlertDialog cloudDeleteDialog = new AlertDialog.Builder(SmartPlugBaseActivity.this).create();
									cloudDeleteDialog.setTitle(getString(R.string.dialog_title_warning));
									cloudDeleteDialog.setMessage(
											String.format(Locale.ENGLISH, "Error: %s\nFailed to delete device on the cloud. %s",
													result.responseCode,
													getString(R.string.message_deletion_warning))
									);								
									cloudDeleteDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener(){
										@Override
										public void onClick(DialogInterface dialog, int which) {
											// GUI handling
											callback.finalAction();
										}
									});
									cloudDeleteDialog.show();
								}
							}else{
								AlertDialog error = new AlertDialog.Builder(SmartPlugBaseActivity.this).create();
								error.setTitle(getString(R.string.dialog_title_error));
								error.setMessage(e.getMessage()
										+ "\nDeletion failed on server. Please delete the device from the server manually."
										+ "This device has been deleted locally.");
								error.setButton(DialogInterface.BUTTON_POSITIVE, "OK", (DialogInterface.OnClickListener) null);
								error.show();
							}
							
							getService().poolDelete(device);
							deleteProgress.dismiss();
						}
					});
				}else{
					getService().poolDelete(device);
					
					//No cloud information, dismiss the progress and take final actions
					deleteProgress.dismiss();
					
					// GUI handling
					callback.finalAction();
				}
			}
		});
		DeleteWarningDialog.setButton(AlertDialog.BUTTON_NEGATIVE, "Cancel", (DialogInterface.OnClickListener) null);
		DeleteWarningDialog.setCanceledOnTouchOutside(false);
		DeleteWarningDialog.setCancelable(true);
		DeleteWarningDialog.show();
	}
		
	//============================== MDNS Finder Operations ==============================
	
	@Override
	public SmartPlugDevice deviceExistInList(String mac_address) {
		return SPS.poolContains(mac_address);
	}
	
	@Override
	public boolean addToDeviceList(ServiceInfo item, String macAddress, String rid, String cik) {
		SmartPlugDevice newDev = new SmartPlugDevice(
				getApplicationContext(),
				rid,
				item.getName(),
				macAddress,
				cik,
				item.getInetAddresses()[0],
				SmartPlugDevice.CONNECTION_PREF_AUTO,
				null);
		SPS.poolAddDevice(newDev);
		LocalBroadcastManager.getInstance(this).sendBroadcast(new Intent(DeviceListFragment.BROADCAST_DEVICE_LIST_REFRESH));
		return true;
	}
	
	@Override
	public boolean updateDeviceLocalAddress(InetAddress address, String macAddress) {
		SPS.updateLocalAddress(SPS.findDeviceByMac(macAddress), address);
		return true;
	}
}

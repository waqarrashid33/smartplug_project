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

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.Toast;

import com.exosite.api.ExoCallback;
import com.exosite.api.ExoException;
import com.exosite.api.portals.Portals;
import com.exosite.api.portals.PortalsResponseException;
import com.ti.cc3200smartplug.service.SmartPlugDevice;

/**
 * The activity that adds device by user's request.
 * The layout used in this activity is <b>R.layout.activity_add_device</b>.
 */
public class AddDeviceActivity extends SmartPlugBaseActivity {
	
	private static final String TAG = "AddDeviceActivity";
	
	public static final String DEFAULT_VENDOR_MAC = "F4B85E";
	
    private static JSONArray mPortalList;
    
    private EditText mVendorEditText;
    private EditText mModelEditText;
    private Spinner mPortalSpinner;
    
    private String vendor;
    private String model;
    private String sn;
    private String name;
    private String localAddress;
    
    private ProgressDialog cloudAddingProgressDialog;
    
    private boolean mLocalAddOnly;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_add_device);
        
        mLocalAddOnly = false;

        SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());

        try {
            mPortalList = new JSONArray(sharedPreferences.getString("portal_list", "[]"));
        } catch (JSONException e) {
            Log.e(TAG, "portal_list shared preference was not set.");
        }

        this.setTitle("Add Device");

        final EditText mNameEditText = (EditText)findViewById(R.id.edittext_device_name);
        mNameEditText.setText("Device Name");
        final EditText mSerialNumberEditText = (EditText)findViewById(R.id.edittext_device_serial_number);
        mSerialNumberEditText.setText(String.format("%s%06X", DEFAULT_VENDOR_MAC, new Random().nextInt(0xffffff)));
        mVendorEditText = (EditText)findViewById(R.id.device_vendor);
        mVendorEditText.setText(SmartPlugDevice.DEFAULT_VENDOR);
        mModelEditText = (EditText)findViewById(R.id.device_model);
        mModelEditText.setText(SmartPlugDevice.DEFAULT_MODEL);
        final EditText mManualLocalAddress = (EditText) findViewById(R.id.edittext_manual_local_address);
        
        // Disable Fields
        mVendorEditText.setEnabled(false);
        mModelEditText.setEnabled(false);

        // populate spinner
        mPortalSpinner = (Spinner) findViewById(R.id.device_portal);
        List<String> SpinnerArray = new ArrayList<String>();
        try {
            for (int i = 0; i < mPortalList.length(); i++) {
                JSONObject portal = mPortalList.getJSONObject(i);
                SpinnerArray.add(String.format("%s (CIK: %s...)",
                        portal.getString("name"),
                        portal.getString("key").substring(0,8)));
            }
        } catch (JSONException e) {
            Log.e(TAG, "Exception while populating spinner: " + e.toString());
        }

        ArrayAdapter<String> adapter = new ArrayAdapter<String>(
                AddDeviceActivity.this,
                android.R.layout.simple_spinner_item,
                SpinnerArray);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mPortalSpinner.setAdapter(adapter);

        findViewById(R.id.ada_btn_add_device).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
            	boolean cancel = false;
            	
            	mLocalAddOnly = ((CheckBox) findViewById(R.id.ada_cbx_local_add_only)).isChecked();
            	
            	vendor	= mVendorEditText		.getText().toString().trim();
				model	= mModelEditText		.getText().toString().trim();
				sn		= mSerialNumberEditText	.getText().toString().trim();
				name	= mNameEditText			.getText().toString().trim();
				localAddress = mManualLocalAddress.getText().toString().trim();
				
				// Check Name
				if (TextUtils.isEmpty(name)) {
					mNameEditText.setError(getString(R.string.error_field_required));
					cancel = true;
				}
				
				// Check Vendor
				if (TextUtils.isEmpty(vendor)) {
					mVendorEditText.setError(getString(R.string.error_field_required));
					cancel = true;
				}
				
				// Check Model
				if (TextUtils.isEmpty(model)) {
					mModelEditText.setError(getString(R.string.error_field_required));
					cancel = true;
				}
				
				// Check MAC address
				if (TextUtils.isEmpty(sn)) {
					mSerialNumberEditText.setError(getString(R.string.error_field_required));
					cancel = true;
				} else if (sn.length() != 12) {
					mSerialNumberEditText.setError(getString(R.string.error_invalid_password));
					cancel = true;
				} else if (getService().poolContains(sn) != null){
					Toast.makeText(getApplicationContext(), "Device exists", Toast.LENGTH_SHORT).show();
					cancel = true;
				} else {
				}
				
				// final action
				if(!cancel){
					if(localAddress.isEmpty()){
						AlertDialog emptyLocalDialog = new AlertDialog.Builder(AddDeviceActivity.this).create();
						emptyLocalDialog.setTitle(getString(R.string.dialog_title_warning));
						emptyLocalDialog.setMessage(getString(R.string.message_no_local_address));
						emptyLocalDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel", (DialogInterface.OnClickListener) null);
						emptyLocalDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener(){
							@Override
							public void onClick(DialogInterface dialog, int which) {
								add();
							}
						});
						emptyLocalDialog.show();
					} else {
						add();
					}
				}
            }
        });
    }
    
    private void add(){
    	if (mLocalAddOnly) {
			localAddandFinish(null, null);
		} else {
			// Cloud Add
        	if(Common.isLoggedIn(getApplicationContext())){
        		cloudAdd();
        		cloudAddingProgressDialog = new ProgressDialog(AddDeviceActivity.this);
        		cloudAddingProgressDialog.setCancelable(false);
        		cloudAddingProgressDialog.setCanceledOnTouchOutside(false);
        		cloudAddingProgressDialog.setTitle("Syncing with Cloud");
        		cloudAddingProgressDialog.setMessage("Please Wait...");
        		cloudAddingProgressDialog.show();
        	}
		}
    }
    
    @Override
    protected void onResume(){
    	super.onResume();
    	
    	if(!Common.isLoggedIn(getApplicationContext())){
    		mVendorEditText.setEnabled(false);
    		mModelEditText.setEnabled(false);
    		mPortalSpinner.setEnabled(false);
    	}
    }

    private void cloudAdd(){
    	try {
            String portalRID = mPortalList.getJSONObject(mPortalSpinner.getSelectedItemPosition()).getString("rid");
            
            SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
            String email = sharedPreferences.getString("email", null);
            String password = sharedPreferences.getString("password", null);
            Portals.addDeviceInBackground(portalRID, vendor, model, sn, name, email, password, new ExoCallback<JSONObject>() {
                @Override
                public void done(JSONObject newDevice, ExoException e) {
                	cloudAddingProgressDialog.dismiss();
                	
                    if (newDevice != null) {	// New device added
                    	String errorMessage = null;
                    	String rid = null;
                    	String cik = null;
                    	try{
                    		rid = newDevice.getString("rid");
                    		cik = newDevice.getString("cik");
                    	} catch(JSONException e2){
                    		errorMessage = e2.getMessage();
                    		rid = null;
                    		cik = null;
                    	}
                    	localAddandFinish(rid, cik);
                    	
                    	if(errorMessage==null){
                    		Toast.makeText(getApplicationContext(), String.format("Device created"), Toast.LENGTH_LONG).show();
                    		finish();
                    	}else{
                    		AlertDialog addErrorDialog = new AlertDialog.Builder(AddDeviceActivity.this).create();
                    		addErrorDialog.setTitle(getString(R.string.dialog_title_error));
                    		addErrorDialog.setMessage(errorMessage + "\nDevice will only only have local connection ability.");
                    		addErrorDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK", new DialogInterface.OnClickListener(){
								@Override
								public void onClick(DialogInterface dialog, int which) {
									Toast.makeText(getApplicationContext(), String.format("Device created Locally"), Toast.LENGTH_LONG).show();
		                    		finish();
								}
                    		});
                    		addErrorDialog.show();
                    	}
						
                    } else {	// Error when adding device
                    	AlertDialog dialogDevCreationError = new AlertDialog.Builder(getApplicationContext()).create();
                    	dialogDevCreationError.setTitle(getString(R.string.dialog_title_error));
                    	dialogDevCreationError.setButton(DialogInterface.BUTTON_NEUTRAL, "OK", (DialogInterface.OnClickListener) null);
                        if (e instanceof PortalsResponseException) {
                            PortalsResponseException pre = (PortalsResponseException)e;
                            JSONObject errObj;
                            try {
                                errObj = new JSONObject(pre.getResponseBody());
                                if (errObj != null) {
                                	dialogDevCreationError.setMessage(String.format("Device not created. Reason: %s", errObj.getJSONArray("errors").getString(0)));
                                }
                            } catch (JSONException je) {
                                int code = pre.getResponseCode();
                                dialogDevCreationError.setMessage(String.format("Error: %s (%d) caused JSONException %s",pre.getMessage(), code, je.getMessage()));
                            } finally {
                            	dialogDevCreationError.show();
                            }

                        } else {
                            Toast.makeText(getApplicationContext(), String.format("Unexpected error: %s", e.getMessage()), Toast.LENGTH_LONG).show();
                        }
                    }
                }
            });

        } catch (JSONException e) {
            Log.e(TAG, "Exception while handling add device click: " + e.toString());
        }
    }
    
    private void localAddandFinish(String rid, String cik){
    	try{
    		String associatedUser = null;
    		if(rid != null){
    			associatedUser = PreferenceManager.getDefaultSharedPreferences(getApplicationContext()).getString(Common.PREFERENCE_EMAIL, null);
    		}
    		getService().poolAddDevice(
    				new SmartPlugDevice(
    						getApplicationContext(),
    						rid,
    						name,
    						sn,
    						cik,
    						model,
    						vendor,
    						localAddress.isEmpty()? null:InetAddress.getByName(localAddress),
    						SmartPlugDevice.CONNECTION_PREF_AUTO,
    						associatedUser));
    	} catch(UnknownHostException e){
    		Toast.makeText(getApplicationContext(), "UnknownHostException", Toast.LENGTH_SHORT).show();
    	}
    	finish();
    }
    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        
        // Inflate the menu; this adds items to the action bar if it is present.
        //getMenuInflater().inflate(R.menu.add_device, menu);
        return true;
    }
    
	@Override
	public int LocalConnectionUpdate(short tcpThreadCode, SmartPlugDevice device) {
		//Nothing
		return 0;
	}
	
	@Override
	public int CloudConnectionUpdate(short cloudThreadCode, SmartPlugDevice device, String msg) {
		//Nothing
		return 0;
	}

	@Override
	protected void serviceConnectedCallBack() {
		//Nothing
	}

	
	@Override
	protected void statusUpdateWiFi(boolean wifiOn, boolean cellularOn) {
		// No implementation as of right now, but might be useful in the future.
	}}

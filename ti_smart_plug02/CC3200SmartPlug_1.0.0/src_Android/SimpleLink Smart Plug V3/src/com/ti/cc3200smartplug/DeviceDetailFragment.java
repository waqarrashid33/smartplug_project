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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;

import org.achartengine.ChartFactory;
import org.achartengine.GraphicalView;
import org.achartengine.model.TimeSeries;
import org.achartengine.model.XYMultipleSeriesDataset;
import org.achartengine.renderer.XYMultipleSeriesRenderer;
import org.achartengine.renderer.XYSeriesRenderer;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.Fragment;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Paint.Align;
import android.graphics.Typeface;
import android.net.ConnectivityManager;
import android.os.Bundle;
import android.os.Environment;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;
import android.view.WindowManager;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.LinearInterpolator;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TableLayout.LayoutParams;
import android.widget.TextView;
import android.widget.Toast;
import ar.com.daidalos.afiledialog.FileChooserDialog;

import com.ti.cc3200smartplug.data.AToDHandler;
import com.ti.cc3200smartplug.data.DToA_MetrologyData;
import com.ti.cc3200smartplug.service.CloudConnectionThread;
import com.ti.cc3200smartplug.service.LocalConnectionThread;
import com.ti.cc3200smartplug.service.SmartPlugDevice;

/**
 * A fragment representing a single Device detail screen. This fragment is
 * either contained in a {@link DeviceListActivity} in two-pane mode (on
 * tablets) or a {@link DeviceDetailActivity} on handsets.
 */
public class DeviceDetailFragment extends Fragment implements AToDHandler.AtoDTimeoutUpdate, SmartPlugDevice.ReconnectionTimerTickUpdate{
	
	public static final String KEY_MAC = "MAC_ADDRESS";
		
	private String					DEBUG_CLASS;
	private SmartPlugDevice			thisDevice;
	
	// MainActivity reference
	private SmartPlugBaseActivity	ACT;
	
	// Graphing instances
	private GraphicalView			mChart;
	private XYMultipleSeriesRenderer multiple_chartRenderer;
	
	private static final String mNameActivePower	= "Active Power";
	private static final String mNameReactivePower	= "Reactive Power";
	private static final String mNameApparentPower	= "Apparent Power";
	private static final String mNameVoltage		= "Voltage";
	private static final String mNameCurrent		= "Current";
	private static final String mNamePowerFactor	= "Power Factor";
	private static final String mNameFrequency		= "Frequency";
	private TimeSeries			mTSActivePower;
	private TimeSeries			mTSReactivePower;
	private TimeSeries			mTSApparentPower;
	private TimeSeries			mTSVoltage;
	private TimeSeries			mTSCurrent;
	private TimeSeries			mTSPowerFactor;
	private TimeSeries			mTSFrequency;
	private XYSeriesRenderer	mSRActivePower;
	private XYSeriesRenderer	mSRReactivePower;
	private XYSeriesRenderer	mSRApparentPower;
	private XYSeriesRenderer	mSRVoltage;
	private XYSeriesRenderer	mSRCurrent;
	private XYSeriesRenderer	mSRPowerFactor;
	private XYSeriesRenderer	mSRFrequency;
	
	private int[] colors;
	
	private static final int LINE_THICKNESS = 6;
	
	private boolean graphReady;
	
	// GUI components
	private	Switch		connection_switch;
	private CompoundButton.OnCheckedChangeListener connection_switch_listener;
	public	TextView	device_name;
	private	TextView	deviceStatus;
	private	TextView	power;
	private	Switch		device_on_off;
	private CompoundButton.OnCheckedChangeListener device_on_off_listener;
	private	TextView	averagePower;
	private	TextView	totalEnergy;
	private	TextView	voltage;
	private	TextView	current;
	private	TextView	frequency;
	private	TextView	cos;
	private	TextView	var;
	private	TextView	va;
	private TextView	hourlyAveragePower;
	private	View		graphingArea;
	private Button		button_detail;
	private Button		button_24hour_average;
	private Button		button_switch_table;
	private	Button		button_set_threshold;
	private	Button		button_power_saving;
	private Button		button_calibrate;
	private Button		button_enable_cloud_TLS;
	private Button		button_update_cert;
	private	ViewGroup	linearLayout;
	
	private boolean userTriggeredOn;
	private boolean userTriggeredOff;
	
//	private static final short WARNING_UPDATE_NONE = 0x00;
	private static final short WARNING_UPDATE_OVER_ENERGY = 0x01;
	private static final short WARNING_UPDATE_OVER_POWER = 0x02;
	private static final short WARNING_UPDATE_MOD_ERROR = 0x04;
	private static final short WARNING_UPDATE_ALL = 0x07;
	
	private	ProgressDialog updateCertDialog;
	private ProgressDialog calibrationInProgressDialog;
	
	/** Minimum values for calibration */
	public final static float CALIB_MIN_V = (float) 0.001;
	public final static float CALIB_MIN_C = (float) 0.001;
	
	public static final int CALIB_READ_TIMEOUT = 20000;
	
	// flags for displaying a specific dialog, after sending the "device status packet"
	private boolean switchTableRequested;
	private boolean	displayThresholdStatusDialog;
	private boolean	displayPowerSavingStatusDialog;
	private boolean displayDeviceDetailDialog;
	
	/* A bunch of CountDownTimers to timeuot the packet request */
	/* Index numbers corresponds to 3.2 packets */
	private ArrayList<AToDHandler> sendMessageHandlers;
	private static final int NUM_HANDLERS = 15;
	
	// Storing the fragment starting time for the graphing area
	private long chartStartingTime;
	
	/** 5 minutes interval of the graphing area */
	public static final int DATA_RANGE = 300000;
	
	/** File type filter */
	public static final String FILE_FILTER_CERTIFICATE = ".*der";
	
	// Custom Class Implementations for warning text
	private class WarningAlphaAnimation extends AlphaAnimation{

		private TextView mTextview;
		
		public WarningAlphaAnimation(TextView textview) {
			super(1, 0);
			mTextview = textview;
			
			setDuration(500); // duration - half a second
		    setInterpolator(new LinearInterpolator()); // do not alter animation rate
		    setRepeatCount(3); // Repeat animation for 3 seconds
		    setRepeatMode(Animation.REVERSE); // Reverse animation at the end so the button will fade back in
		    setAnimationListener(new Animation.AnimationListener() {
				@Override
				public void onAnimationStart(Animation animation) {
				}
				
				@Override
				public void onAnimationRepeat(Animation animation) {
				}
				
				@Override
				public void onAnimationEnd(Animation animation) {
					mTextview.setTextColor(Color.BLACK);
				}
			});
		}
	}
	
	//================================= Fragment Lifecycle =================================
	
	@Override
	public void onAttach(Activity activity) {
		super.onAttach(activity);
		ACT = (SmartPlugBaseActivity) getActivity();
		
		Log.d("TTT", "Check20");
	}
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		Log.d("TTT", "Check21");

		/* Find the current Device's object & set the ID */
		String mac = getArguments().getString(KEY_MAC);
		thisDevice = ACT.getService().poolGetDevice(mac);
		
		thisDevice.setTimerCallbackFragment(this);
		
		DEBUG_CLASS = thisDevice.getName() + " DeviceDetailFragment";
		
		/* Allow Options Menu */
		setHasOptionsMenu(true);
		
		/* Connection Switch Action */
		connection_switch_listener = new CompoundButton.OnCheckedChangeListener(){
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				
				buttonView.setEnabled(false);
				
				/* This method is also triggered if changed by the system.
				 * Therefore needs to check if it's changed by user. */
				Log.w("Connection Switch Triggered", Boolean.toString(isChecked));
				
			    if (isChecked) {
			    	userTriggeredOn = true;
			    	userTriggeredOff = false;
			    	String errorMessage = null;			    	
			    	
		    		/* Check WiFi connectivity */
					if(ACT.connManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI).isConnected()) {
						
						if(thisDevice.getConnectionPreference() == SmartPlugDevice.CONNECTION_PREF_CLOUD) {
							// CLOUD: use local only
							
							if(Common.isLoggedIn(ACT.getApplicationContext())){
								if(thisDevice.getCIK() != null){
									ACT.getService().executeCloudConnection(thisDevice);
								}else{
									silentlySetConnectionSwitChcheck(false, true);
									errorMessage = "Cannot create cloud connection. Please check your connection preference.";
								}
							}else{
								errorMessage = ACT.getString(R.string.message_no_cloud_connection);
							}
							
						} else if(thisDevice.getConnectionPreference() == SmartPlugDevice.CONNECTION_PREF_LOCAL) {
							// LOCAL: use local only
							if(thisDevice.getLocalAddress() != null){
								ACT.getService().executeLocalConnection(thisDevice);
								
							}else{ //No connection available, switch the connection switch back to off, and warn user
								silentlySetConnectionSwitChcheck(false, true);
								errorMessage = "Cannot create local connection. Please check your connection preference.";
							}
							
						} else {
							// AUTO: use cloud as priority. Switch to local if cloud fails.							
							if((thisDevice.getCIK() != null) && Common.isLoggedIn(ACT.getApplicationContext())){
								ACT.getService().executeCloudConnection(thisDevice);
								
							}else if(thisDevice.getLocalAddress() != null){
								ACT.getService().executeLocalConnection(thisDevice);
								
							}else{ //No connection available, switch the connection switch back to off, and warn user
								silentlySetConnectionSwitChcheck(false, true);
								errorMessage = ACT.getString(R.string.message_not_enough_info);
							}
						}
						
						if(errorMessage != null){
							silentlySetConnectionSwitChcheck(false, true);
							Toast.makeText(ACT, errorMessage, Toast.LENGTH_SHORT).show();
						}
						
					}else if(ACT.connManager.getNetworkInfo(ConnectivityManager.TYPE_MOBILE).isConnected()){					
						if(thisDevice.getCIK() != null){
							AlertDialog cellularWarning = new AlertDialog.Builder(ACT).create();
							cellularWarning.setTitle(getString(R.string.dialog_title_warning));
							cellularWarning.setMessage(getString(R.string.message_cellular_warning));
							cellularWarning.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel", new DialogInterface.OnClickListener(){
								@Override
								public void onClick(DialogInterface dialog, int which) {
									silentlySetConnectionSwitChcheck(false, true);
								}
							});
							cellularWarning.setButton(DialogInterface.BUTTON_POSITIVE, "Yes", new DialogInterface.OnClickListener(){
								@Override
								public void onClick(DialogInterface dialog, int which) {
									ACT.getService().executeCloudConnection(thisDevice);
								}
							});
							cellularWarning.show();
							
						}else{ //No connection available, switch the connection switch back to off, and warn user
							silentlySetConnectionSwitChcheck(false, true);
							Toast.makeText(ACT,
									"ERROR: Not enough information information to connect. Please add this device to cloud or enter an IP address",
									Toast.LENGTH_SHORT).show();
						}
						
					} else{
						AlertDialog alertDialog = new AlertDialog.Builder(ACT).create();
						alertDialog.setTitle("Please enable network connection");
						alertDialog.setButton(AlertDialog.BUTTON_POSITIVE, "OK", (DialogInterface.OnClickListener) null);
						alertDialog.show();
						silentlySetConnectionSwitChcheck(false, true);
					}
			    } else {
			    	userTriggeredOn = false;
			    	userTriggeredOff = true;
			    	resetToDefaultState();
			    }
			    
			    //Notify the list fragment to change icons
			    LocalBroadcastManager.getInstance(getActivity()).sendBroadcast(new Intent(DeviceListFragment.BROADCAST_DEVICE_LIST_REFRESH));
			}
		};
		
		device_on_off_listener = new CompoundButton.OnCheckedChangeListener(){
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				sendRelay(buttonView, isChecked);
			}
		};
	
		sendMessageHandlers = new ArrayList<AToDHandler>(NUM_HANDLERS);
		for(int i=0; i<NUM_HANDLERS; i++){
			sendMessageHandlers.add(new AToDHandler(this, i));
		}
		
		displayThresholdStatusDialog = false;
		displayPowerSavingStatusDialog = false;
		
		switchTableRequested = false;
		
		graphReady = false;
		
		displayDeviceDetailDialog = false;
		
		userTriggeredOn = false;
		userTriggeredOff = false;
		
		Log.d("TTT", "Check22");
	}
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		View rootView = inflater.inflate(R.layout.fragment_device_detail, container, false);
		
		Log.d("TTT", "Check23");
		
		/* Initialize all widgets */
		device_name		= (TextView)	rootView.findViewById(R.id.ddf_deviceName);
		device_name.setText(thisDevice.getName());
		
		/* Assigning components to variables */
		connection_switch = (Switch)	rootView.findViewById(R.id.device_switch);
		device_on_off	= (Switch)		rootView.findViewById(R.id.device_on_off);
		device_on_off.setEnabled(false);
		
		deviceStatus	= (TextView)	rootView.findViewById(R.id.ddf_deviceStatus);
//		timestamp		= (TextView)	rootView.findViewById(R.id.timestamp_value);
		power			= (TextView)	rootView.findViewById(R.id.power);
		averagePower	= (TextView)	rootView.findViewById(R.id.average_power);
		totalEnergy		= (TextView)	rootView.findViewById(R.id.totalEnergy);
		voltage			= (TextView)	rootView.findViewById(R.id.voltage);
		current			= (TextView)	rootView.findViewById(R.id.current);
		frequency		= (TextView)	rootView.findViewById(R.id.frequency);
		cos				= (TextView)	rootView.findViewById(R.id.COS);
		var				= (TextView)	rootView.findViewById(R.id.VAR);
		va				= (TextView)	rootView.findViewById(R.id.VA);
		hourlyAveragePower = (TextView) rootView.findViewById(R.id.hourlyAverage);
		
		graphingArea	= rootView.findViewById(R.id.graphing_area);
		
		button_24hour_average = (Button)	rootView.findViewById(R.id.ddf_button_24hour_average);
		button_24hour_average.setEnabled(false);
		button_24hour_average.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				sendRequest24HourAvg();	//Only local connection
			}
		});
		
		button_switch_table = (Button)	rootView.findViewById(R.id.ddf_button_switch_table);
		button_switch_table.setEnabled(false);
		button_switch_table.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if(thisDevice.isCloudConnectionLive()){
					// In cloud connection, simply show the dialog
					showSwitchTable();
				}else{
					// In local connection, send request
					sendRequestSwitchTable();
				}
			}
		});
		
		button_set_threshold = (Button)	rootView.findViewById(R.id.button_set_threshold);
		button_set_threshold.setEnabled(false);
		button_set_threshold.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				displayThresholdStatusDialog = true;
				if(thisDevice.isCloudConnectionLive()){
					// In cloud connection, simply show the dialog
					updateDeviceStatus(false);
				}else{
					// In local connection, send request
					sendRequestDeviceInfo();
				}
			}
		});
		
		button_power_saving = (Button)	rootView.findViewById(R.id.button_send_power_saving);
		button_power_saving.setEnabled(false);
		button_power_saving.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				displayPowerSavingStatusDialog = true;
				if(thisDevice.isCloudConnectionLive()){
					// In cloud connection, simply show the dialog
					updateDeviceStatus(false);
				}else{
					// In local connection, send request
					sendRequestDeviceInfo();
				}
			}
		});
		
		button_detail = (Button) rootView.findViewById(R.id.ddf_button_details);
		button_detail.setOnClickListener(new View.OnClickListener(){
			@Override
			public void onClick(View v) {
				if(thisDevice.getLocalConnectionStatus() == SmartPlugDevice.LOCAL_CONNECTION_LIVE){ //Socket not created, alert user and no action will be taken
					displayDeviceDetailDialog = true;
					sendRequestCloudInfo();
				} else {
					showDeviceDetailDialog(false);
					return;
				}
			}
		});
		
		button_calibrate = (Button) rootView.findViewById(R.id.ddf_button_calibrate);
		button_calibrate.setEnabled(false);
		button_calibrate.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				sendRequestCalibration();
			}
		});
		
		button_enable_cloud_TLS = (Button) rootView.findViewById(R.id.ddf_button_secure_cloud);
		button_enable_cloud_TLS.setEnabled(false);
		button_enable_cloud_TLS.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				sendRequestCloudInfo();
			}
		});
		
		button_update_cert = (Button) rootView.findViewById(R.id.ddf_button_update_cert);
		button_update_cert.setEnabled(false);
		button_update_cert.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				// Start File Chooser
				FileChooserDialog dialog = new FileChooserDialog(ACT);
				dialog.loadFolder(Environment.getExternalStorageDirectory() + "/Download/");
//				dialog.setFilter(FILE_FILTER_CERTIFICATE);
	    		dialog.setShowOnlySelectable(true);
			    dialog.addListener(new FileChooserDialog.OnFileSelectedListener() {
					
					@Override
					public void onFileSelected(Dialog source, File folder, String name) {
						//Nothing
					}
					
					@Override
					public void onFileSelected(final Dialog source, File file) {
						
						// Data to pass
						byte checksum = 0x00;
						final ArrayList<Byte> data = new ArrayList<Byte>((int) file.length() + 1);
						
						boolean readSuccess = false;
						
						InputStream input = null;
						try {
							input = new FileInputStream(file);
							int content = input.read();
							
							data.add((byte) content);
							checksum = (byte) content;
							
							while ((content = input.read()) != -1) {
								data.add((byte) content);
								checksum = (byte) (checksum ^ ((byte) content));
							}
							
							readSuccess = true;
							
						} catch (FileNotFoundException e) {
							Toast.makeText(ACT.getApplicationContext(), e.getMessage(), Toast.LENGTH_SHORT).show();
						} catch (IOException e2){
							Toast.makeText(ACT.getApplicationContext(), e2.getMessage(), Toast.LENGTH_SHORT).show();
						} finally {
							
							try {input.close();} catch (IOException e) {}
							if(!readSuccess)
								return;
						}
						
						data.add(checksum);
						
						String checksumString = String.format("%8s", Integer.toBinaryString(checksum)).replace(" ", "0");
						if(checksumString.length() > 8){
							checksumString = checksumString.substring(checksumString.length()-8);
						}
						
						// Display file detail to user
						StringBuilder msg = new StringBuilder();
						msg.append(getString(R.string.message_update_certificate));
						msg.append("\n\nFile Information:");
						msg.append("\n\nName: " + file.getName());
						msg.append("\nSize: " + file.length() + "bytes");
						msg.append("\nChecksum: " + checksumString);
						
						AlertDialog confirmation = new AlertDialog.Builder(ACT).create();
						confirmation.setTitle(getString(R.string.dialog_title_warning));
						confirmation.setMessage(msg.toString());
						confirmation.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel", (DialogInterface.OnClickListener) null);
						confirmation.setButton(DialogInterface.BUTTON_POSITIVE, "Update", new DialogInterface.OnClickListener(){
							@Override
							public void onClick(DialogInterface dialog, int which) {
								source.hide();
								
								// show progress. cannot be canceled.
								updateCertDialog = new ProgressDialog(ACT);
								updateCertDialog.setTitle("Updating");
								updateCertDialog.setMessage("Please Wait...");
								updateCertDialog.setCancelable(true);
								updateCertDialog.setCanceledOnTouchOutside(false);
								updateCertDialog.show();
								
								sendSetExositeCertificate(data);
							}
						});
						confirmation.show();
						
					}
				});
			    dialog.show();
				
//				sendSetExositeCertificate();
//				sendSetEnableCloudTLS(true);
				
				// check packet 3.1.7 and make sure bit 2 in byte 37 is good
			}
		});
		
		Spinner spinner_change_graph = (Spinner) rootView.findViewById(R.id.spinnerGraphOption);
		spinner_change_graph.setOnItemSelectedListener(new OnItemSelectedListener() {

			@Override
			public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
				thisDevice.CurrentGraphSelection = position;
				changeGraphFromSelection(position);	
				mChart.repaint();
			}

			@Override
			public void onNothingSelected(AdapterView<?> parent) {
				//Nothing
			}
		});
		
		linearLayout = (ViewGroup) rootView.findViewById(R.id.device_detail);
		
		/* Set Switch actions */
		connection_switch.setOnCheckedChangeListener(connection_switch_listener);
		device_on_off.setOnCheckedChangeListener(device_on_off_listener);
		
		((TextView) rootView.findViewById(R.id.ddf_text_mac)).setText(thisDevice.getMAC());
		Log.d("TTT", "Check24");
		return rootView;
	}
	
	private void showDeviceDetailDialog(boolean showCloudInfo) {
		ListView lv = new ListView(ACT);
		
		String[] from = new String[] {"col_1", "col_2"};
		List<HashMap<String, String>> fillMaps = thisDevice.getEverything(from[0], from[1], showCloudInfo);
		
		SimpleAdapter a = new SimpleAdapter(
				ACT,
				fillMaps,
				R.layout.listview_item_two_textview,
				from,
				new int[] {R.id.lv_text1, R.id.lv_text2});
		lv.setAdapter(a);
		
		AlertDialog dialog = new AlertDialog.Builder(ACT).create();
		dialog.setTitle(thisDevice.getMAC() + " Details");
		dialog.setView(lv);
		dialog.setButton(DialogInterface.BUTTON_NEUTRAL, "OK", (DialogInterface.OnClickListener)null);
		dialog.show();
	}
	
	@Override
	public void onSaveInstanceState(Bundle outState){
		super.onSaveInstanceState(outState);
		Log.d(DEBUG_CLASS, "I'm finally here! Not sure what to do...");
	}
	
	@Override
	public void onViewCreated(View view, Bundle savedInstanceState) {
		super.onViewCreated(view, savedInstanceState);

		Log.d("TTT", "Check25");
		
		/* Establish callback. Once graphing view is ready, we'll insert our graph */
		final ViewTreeObserver observer = graphingArea.getViewTreeObserver();
		observer.addOnGlobalLayoutListener(new OnGlobalLayoutListener() {
			@Override
			public void onGlobalLayout() {
				/* Keyboard appearance distorted view height. Therefore we wait until height is more than 50,
				 * which means keyboard is gone */
				observer.removeOnGlobalLayoutListener(this);
				
				/* Generate chart */
				generateChart();
				graphReady = true;
			}
		});
		
		//Check if socket connection is up. If it is, then set the appropriate texts.
		boolean cloudLive = thisDevice.isCloudConnectionLive();
		short	localStatus = thisDevice.getLocalConnectionStatus();
		if(cloudLive || (localStatus==SmartPlugDevice.LOCAL_CONNECTION_LIVE) || (localStatus==SmartPlugDevice.LOCAL_CONNECTION_TRYING)){
			if(cloudLive){
				deviceStatus.setText("Cloud Connection (" + thisDevice.getCIK().substring(0, 6) + "...)");
				button_calibrate.setEnabled(false);
				button_enable_cloud_TLS.setEnabled(false);
				button_update_cert.setEnabled(false);
				button_24hour_average.setEnabled(false);
			}else{
				if(localStatus==SmartPlugDevice.LOCAL_CONNECTION_LIVE)
					deviceStatus.setText("Local Connection (" + thisDevice.getLocalAddress() + ")");
				else
					deviceStatus.setText("Local Connection Retying (" + thisDevice.getLocalAddress().getHostAddress() + ")");
				button_calibrate.setEnabled(true);
				button_enable_cloud_TLS.setEnabled(true);
				button_update_cert.setEnabled(true);
				button_24hour_average.setEnabled(true);
			}
			if(localStatus==SmartPlugDevice.LOCAL_CONNECTION_LIVE)
				deviceStatus.setTextColor(getResources().getColor(R.color.TI_COLOR_PANTONE_3125));
			else
				deviceStatus.setTextColor(getResources().getColor(R.color.TI_COLOR_PANTONE_1807));
			
			silentlySetConnectionSwitChcheck(true, true);
			
			button_switch_table.setEnabled(true);
			button_set_threshold.setEnabled(true);
			button_power_saving.setEnabled(true);
			
			silentSetOnOffSwitChcheck(thisDevice.latestDeviceStatus.deviceOn, true);
			
			power		.setText(String.format("%,.3f", thisDevice.getLatestData().activePower));
			averagePower.setText(String.format("%,.3f", thisDevice.getLatestData().avgPower));
			totalEnergy	.setText(String.format("%,.4f",	thisDevice.getLatestData().KWh));
			voltage		.setText(String.format("%,.3f", thisDevice.getLatestData().voltage));
			current		.setText(String.format("%,.3f", thisDevice.getLatestData().current));
			frequency	.setText(String.format("%,.3f", thisDevice.getLatestData().frequency));
			cos			.setText(String.format("%,.4f", thisDevice.getLatestData().cos));
			var			.setText(String.format("%,.3f", thisDevice.getLatestData().reactivePower));
			va			.setText(String.format("%,.3f", thisDevice.getLatestData().activePower));
			
			if(thisDevice.latestWarningOverEnergy.hasNewValue){
				power.setTextColor(getResources().getColor(R.color.TI_COLOR_PANTONE_1807));
				totalEnergy.startAnimation(new WarningAlphaAnimation(totalEnergy));
			}
			
			if (thisDevice.latestWarningOverPower.hasNewValue){
		    	power.setTextColor(getResources().getColor(R.color.TI_COLOR_PANTONE_1807));
		    	power.startAnimation(new WarningAlphaAnimation(power));
		    }
			
		}else{
			//Socket is not live, only set the status text
			//Everything else remaining the same
			/* Set default offline string */
			deviceStatus.setText("Offline");
			deviceStatus.setTextColor(getResources().getColor(R.color.TI_COLOR_PANTONE_1807));
		}
		
		Log.d("TTT", "Check26");
	}
	
	//================================= Graphing Chart Setting =================================
	
	private void generateChart() {
		
		colors = new int[] {
				getResources().getColor(R.color.TI_COLOR_PANTONE_321),
				Color.GREEN,
				Color.YELLOW,
				Color.MAGENTA,
				Color.BLUE,
				Color.CYAN,
				Color.RED};
		
		mTSActivePower		= new TimeSeries(mNameActivePower);
		mTSReactivePower	= new TimeSeries(mNameReactivePower);
		mTSApparentPower	= new TimeSeries(mNameApparentPower);
		mTSVoltage			= new TimeSeries(mNameVoltage);
		mTSCurrent			= new TimeSeries(mNameCurrent);
		mTSPowerFactor		= new TimeSeries(mNamePowerFactor);
		mTSFrequency		= new TimeSeries(mNameFrequency);
		
		chartStartingTime = System.currentTimeMillis();
		
		//Add back all existing points
		ArrayList<DToA_MetrologyData> listData = thisDevice.getPowerHistory(true, DATA_RANGE);
		for(DToA_MetrologyData data : listData){
			mTSActivePower.add(data.getLastDataTime(), data.activePower);
			mTSReactivePower.add(data.getLastDataTime(), data.reactivePower);
			mTSApparentPower.add(data.getLastDataTime(), data.apparentPower);
			mTSVoltage.add(data.getLastDataTime(), data.voltage);
			mTSCurrent.add(data.getLastDataTime(), data.current);
			mTSPowerFactor.add(data.getLastDataTime(), data.cos);
			mTSFrequency.add(data.getLastDataTime(), data.frequency);
		}
		
		/* Add series into multi-series */
		XYMultipleSeriesDataset multiple_datapoints = new XYMultipleSeriesDataset();
		multiple_datapoints.addSeries(mTSActivePower);
		multiple_datapoints.addSeries(mTSReactivePower);
		multiple_datapoints.addSeries(mTSApparentPower);
		multiple_datapoints.addSeries(mTSVoltage);
		multiple_datapoints.addSeries(mTSCurrent);
		multiple_datapoints.addSeries(mTSPowerFactor);
		multiple_datapoints.addSeries(mTSFrequency);
		
		// Set all renderers
		mSRActivePower	= createRenderer(Color.TRANSPARENT);
//		mSRActivePower.setPointStyle(PointStyle.DIAMOND);
		mSRReactivePower = createRenderer(Color.TRANSPARENT);
		mSRApparentPower = createRenderer(Color.TRANSPARENT);
		mSRVoltage		= createRenderer(Color.TRANSPARENT);
		mSRCurrent		= createRenderer(Color.TRANSPARENT);
		mSRPowerFactor	= createRenderer(Color.TRANSPARENT);
		mSRFrequency	= createRenderer(Color.TRANSPARENT);
		
		/* Add renderer into multi-renderers */
		multiple_chartRenderer = new XYMultipleSeriesRenderer();
		multiple_chartRenderer.addSeriesRenderer(mSRActivePower);
		multiple_chartRenderer.addSeriesRenderer(mSRReactivePower);
		multiple_chartRenderer.addSeriesRenderer(mSRApparentPower);
		multiple_chartRenderer.addSeriesRenderer(mSRVoltage);
		multiple_chartRenderer.addSeriesRenderer(mSRCurrent);
		multiple_chartRenderer.addSeriesRenderer(mSRPowerFactor);
		multiple_chartRenderer.addSeriesRenderer(mSRFrequency);
		
		/* Default chart settings */
		/* Chart text settings */
		multiple_chartRenderer.setTextTypeface(Typeface.DEFAULT);
//		multiple_chartRenderer.setChartTitle("Power History");
		multiple_chartRenderer.setChartTitleTextSize(DeviceListActivity.defaultTextSize);
		multiple_chartRenderer.setXTitle("Time");
//		multiple_chartRenderer.setYTitle("Current Power (W)");
		multiple_chartRenderer.setYAxisAlign(Align.RIGHT, 0);
		multiple_chartRenderer.setAxisTitleTextSize(DeviceListActivity.defaultTextSize);
		
		multiple_chartRenderer.setGridColor(DeviceListActivity.defaultTextColor);
		multiple_chartRenderer.setLabelsColor(DeviceListActivity.defaultTextColor);
		multiple_chartRenderer.setMarginsColor(DeviceListActivity.defaultTextColor);
		multiple_chartRenderer.setXLabelsColor(DeviceListActivity.defaultTextColor);
		
		/* Chart graph area settings */
		multiple_chartRenderer.setXAxisMin(chartStartingTime - DATA_RANGE);
		multiple_chartRenderer.setXAxisMax(chartStartingTime);
		multiple_chartRenderer.setYAxisMin(0);
		changeGraphFromSelection(thisDevice.CurrentGraphSelection);
		multiple_chartRenderer.setYLabels(10);
		multiple_chartRenderer.setLabelsTextSize(DeviceListActivity.defaultTextSize);
		multiple_chartRenderer.setYLabelsAlign(Align.LEFT);
		multiple_chartRenderer.setShowLegend(false);
		multiple_chartRenderer.setAntialiasing(false);
		multiple_chartRenderer.setGridColor(Color.LTGRAY);
		
		/* Zoom and Pan settings */
		multiple_chartRenderer.setPanEnabled(false, false);
		multiple_chartRenderer.setZoomEnabled(false, false);
		
		/* Other settings */
		multiple_chartRenderer.setShowGrid(true);
		//multiple_chartRenderer.setXLabels(10);
		
		/* Transparent background */
		multiple_chartRenderer.setApplyBackgroundColor(true);
		multiple_chartRenderer.setMarginsColor(Color.argb(0x00, 0x01, 0x01, 0x01));
		multiple_chartRenderer.setBackgroundColor(Color.TRANSPARENT);
		
		/* Default chart settings FINISH */
		
		/* There's already data in the plot from previous session. */
//		if(activePowerTS.getItemCount()!=0){
//			long first_history_time = thisDevice.getPowerHistory().get(0).getLastDataTime();
//			
//			if(first_history_time > (fragmentStartingTime - GRAPH_INTERVAL)){
//				multiple_chartRenderer.setXAxisMin(first_history_time);
//				multiple_chartRenderer.setXAxisMax(first_history_time + GRAPH_INTERVAL);
//			} else {
//				multiple_chartRenderer.setXAxisMin(fragmentStartingTime - GRAPH_INTERVAL);
//				multiple_chartRenderer.setXAxisMax(fragmentStartingTime);
//			}
//			
//			adjustGraphingAreaYAxis();
//		}
		
		mChart = ChartFactory.getTimeChartView(ACT, multiple_datapoints, multiple_chartRenderer, null);
		LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, graphingArea.getHeight(), 0);
		lp.setMargins(Common.dp_to_pixel(getActivity(), 16),
				Common.dp_to_pixel(getActivity(), 8),
				Common.dp_to_pixel(getActivity(), 16),
				Common.dp_to_pixel(getActivity(), 16));
		mChart.setLayoutParams(lp);
		linearLayout.removeView(graphingArea);
		linearLayout.addView(mChart);
	}
	
	private XYSeriesRenderer createRenderer(int color){
		XYSeriesRenderer ret = new XYSeriesRenderer();
		changeRendererColor(ret, color);
		ret.setLineWidth(LINE_THICKNESS);
		return ret;
	}
	
	private TimeSeries getTimeSeriesFromSelection(int selection){
		switch(selection){
			case 0:
				return mTSActivePower;
			case 1:
				return mTSReactivePower;
			case 2:
				return mTSApparentPower;
			case 3:
				return mTSVoltage;
			case 4:
				return mTSCurrent;
			case 5:
				return mTSPowerFactor;
			case 6:
				return mTSFrequency;
		}
		
		return mTSActivePower;
	}
	
	private void changeGraphFromSelection(int position){
		changeRendererColor(mSRActivePower, Color.TRANSPARENT);
		changeRendererColor(mSRReactivePower, Color.TRANSPARENT);
		changeRendererColor(mSRApparentPower, Color.TRANSPARENT);
		changeRendererColor(mSRVoltage, Color.TRANSPARENT);
		changeRendererColor(mSRCurrent, Color.TRANSPARENT);
		changeRendererColor(mSRPowerFactor, Color.TRANSPARENT);
		changeRendererColor(mSRFrequency, Color.TRANSPARENT);
		
		switch(position){
			case 0:	//Active Power
				multiple_chartRenderer.setChartTitle(mNameActivePower + " History");
				multiple_chartRenderer.setYTitle("Power (W)");
				adjustGraphingAreaYAxis(mTSActivePower, -1, -1);
				changeRendererColor(mSRActivePower, colors[0]);
				break;
				
			case 1: //Reactive Power
				multiple_chartRenderer.setChartTitle(mNameReactivePower + " History");
				multiple_chartRenderer.setYTitle("Power (W)");
				adjustGraphingAreaYAxis(mTSReactivePower, -1, -1);
				changeRendererColor(mSRReactivePower, colors[1]);
				break;
				
			case 2: //Active Power
				multiple_chartRenderer.setChartTitle(mNameApparentPower + " History");
				adjustGraphingAreaYAxis(mTSApparentPower, -1, -1);
				multiple_chartRenderer.setYTitle("Power (W)");
				changeRendererColor(mSRApparentPower, colors[2]);
				break;
				
			case 3: //Voltage
				multiple_chartRenderer.setChartTitle(mNameVoltage + " History");
				multiple_chartRenderer.setYTitle("Voltage (V)");
				adjustGraphingAreaYAxis(mTSVoltage, -1, -1);
				changeRendererColor(mSRVoltage, colors[3]);
				break;
				
			case 4: //Current
				multiple_chartRenderer.setChartTitle(mNameCurrent + " History");
				multiple_chartRenderer.setYTitle("Current (A)");
				adjustGraphingAreaYAxis(mTSCurrent, -1, -1);
				changeRendererColor(mSRCurrent, colors[4]);
				break;
				
			case 5: //Power Factor
				multiple_chartRenderer.setChartTitle(mNamePowerFactor + " History");
				multiple_chartRenderer.setYTitle("Cos()");
				adjustGraphingAreaYAxis(mTSPowerFactor, 0, 1);
				changeRendererColor(mSRPowerFactor, colors[5]);
				break;
				
			case 6: //Frequency
				multiple_chartRenderer.setChartTitle(mNameFrequency + " History");
				multiple_chartRenderer.setYTitle("Frequency (Hz)");
				adjustGraphingAreaYAxis(mTSFrequency, -1, -1);
				changeRendererColor(mSRFrequency, colors[6]);
				break;
		}
	}
	
	private void changeRendererColor(XYSeriesRenderer renderer, int color){
		renderer.setColor(color);
		
//		FillOutsideLine fill;
//		if(color == Color.TRANSPARENT){
//			fill = new FillOutsideLine(FillOutsideLine.Type.BOUNDS_ALL);
//			fill.setColor(Color.TRANSPARENT);
//		}else{
//			fill = new FillOutsideLine(FillOutsideLine.Type.BOUNDS_ALL);
//			fill.setColor(Color.argb(0x0F, Color.red(color), Color.green(color), Color.blue(color)));
//		}
//		renderer.addFillOutsideLine(fill);
	}
		
	/** 
	 * Adjust chart Y axis for better plot fitting.
	 * @param series The series as the target for adjustment
	 * @param hardMinY - the absolute minimum Y value. Set it to a negative number to disable it.
	 * @param hardMaxY - the absolute maximum Y value. Set it to a negative number to disable it.
	 */
	private void adjustGraphingAreaYAxis(TimeSeries series, int hardMinY, int hardMaxY){
		
		double finalMin = hardMinY;
		double finalMax = hardMaxY;
		
		boolean hardMin = (hardMinY > 0);
		boolean hardMax = (hardMaxY > 0);
		
		if(!hardMin)
			finalMin = series.getMinY();
		
		if(!hardMax)
			finalMax = series.getMaxY();
		
		double difference = finalMax - finalMin;
		
		if (Math.abs(difference) < 0.0001) {	// max and min have (almost) the same value
			if(!hardMax)
				finalMax += 10;					// Spacing above the plotted line
			if(!hardMin){
				if (finalMin < 10)				// yMin should be 0 if lower than 10
					finalMin = 0;
				else
					finalMin -= 10;				// Spacing below the plotted line
			}
			
		} else {						// max and min have some difference
			if(!hardMax){
				finalMax += difference / 4.0;	// Spacing above the plotted line
//				if(finalMax < 5.0)
//					finalMax = 5.0;				// yMax should be at least 5.0
			}
			if(!hardMin){
				if (finalMin < 10)
					finalMin = 0;				// yMin should be 0 if lower than 10
				else
					finalMin -= difference / 4.0; // Spacing below the plotted line
			}
		}
		multiple_chartRenderer.setYAxisMax(finalMax);
		multiple_chartRenderer.setYAxisMin(finalMin);
		
		multiple_chartRenderer.setMargins(new int[] {(int) DeviceListActivity.defaultTextSize * 2, 0, (int) DeviceListActivity.defaultTextSize * 2, (int) DeviceListActivity.defaultTextSize * 4 });
	}
	
	//================================= Device Operations =================================
	
	/**
	 * Get the index of the device associated in this DDF from the devicePool. 
	 * @return The index
	 */
	public int getCurrentDeviceIndex(){
		return ACT.getService().poolIndexOf(thisDevice);
	}
	
	/**
	 * Get the device associated in this DDF from the devicePool. 
	 * @return
	 */
	public SmartPlugDevice getCurrentDevice(){
		return thisDevice;
	}
	
	/**
	 * Retrieve the device used in this fragment
	 * @return
	 */
	public SmartPlugDevice getDevice(){
		return thisDevice;
	}
	
	//================================= Update to Smart Plug =================================
	
	/**
	 * Packet 3.2.1/3.4.1 - Packet to turn off/on the connected device (Relay)
	 * @param buttonView The caller view
	 * @param isChecked check state
	 */
	private void sendRelay(CompoundButton buttonView, boolean isChecked){
	    if(thisDevice.getLocalConnectionStatus() == SmartPlugDevice.LOCAL_CONNECTION_LIVE){
	    	thisDevice.sendLocalData(Common.hexStringToByteArray("a701" + (isChecked? "01":"00")));
	    	
		} else if(thisDevice.isCloudConnectionLive()){
			thisDevice.sendCloudData(SmartPlugDevice.ALIAS_RELAY, (isChecked? "1":"0"));
			sendMessageHandlers.get(AToDHandler.PACKET_SET_THRESHOLD).startTimer(true);
			
		} else{
			// No active connection, alert user and no action will be taken
			Toast.makeText(ACT, getString(R.string.message_no_connection), Toast.LENGTH_SHORT).show();
			return;
		}
	}
	
	/**
	 * Packet 3.2.2/3.4.2 - Packet to set energy/power thresholds
	 */
	private void sendSetThreshold(){
		if(thisDevice.getLocalConnectionStatus() == SmartPlugDevice.LOCAL_CONNECTION_LIVE){ //Socket not created, alert user and no action will be taken
			//Proceed
		} else if(thisDevice.isCloudConnectionLive()){
			//Proceed
		} else{
			Toast.makeText(ACT, getString(R.string.message_no_connection), Toast.LENGTH_SHORT).show();
			return;
		}
		
		/* Show dialog for threshold input */
		final AlertDialog sendThresholdDialog = new AlertDialog.Builder(ACT).create();
		sendThresholdDialog.setTitle("Set Energy & Power Threshold");
		
		View v = ACT.getLayoutInflater().inflate(R.layout.dialog_threshold, null, false);
		
		final EditText inputEnergyThreshold = (EditText) v.findViewById(R.id.editText_energy_threshold);
		final EditText inputPowerThreshold = (EditText) v.findViewById(R.id.editText_power_threshold);
		
		//Create dialog
		sendThresholdDialog.setView(v);
		sendThresholdDialog.setButton(AlertDialog.BUTTON_POSITIVE, "OK",  (DialogInterface.OnClickListener) null);
		sendThresholdDialog.setButton(AlertDialog.BUTTON_NEGATIVE, "Cancel", (DialogInterface.OnClickListener) null);
		sendThresholdDialog.setCanceledOnTouchOutside(false);
		sendThresholdDialog.setCancelable(true);
		sendThresholdDialog.setOnShowListener(new DialogInterface.OnShowListener() {
			@Override
			public void onShow(DialogInterface dialog) {
				Button b = sendThresholdDialog.getButton(AlertDialog.BUTTON_POSITIVE);
				b.setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						//Form packet
						byte[] data = new byte[10];
						data[0] = (byte) 0xa7;
						data[1] = (byte) 0x02;
						
						String message = "Invalid inputs";
						
						try{
							double EValue = Double.parseDouble(inputEnergyThreshold.getText().toString());
							float PValue = Float.parseFloat(inputPowerThreshold.getText().toString());
							
							if(EValue<0 || EValue>429496.0){
								message = "Invalid inputs, energy threshold range: [0, 429496]";
								throw new NumberFormatException();
							}
							
							if(PValue<0 || PValue>4500.0){
								message = "Invalid inputs, energy threshold range: [0, 4500]";
								throw new NumberFormatException();
							}
							
							byte[] modEValue = Common.longToByteArray(Math.round(EValue * 10000));
							byte[] modPValue = Common.intToByteArray(Math.round(PValue * 1000));
							
							System.arraycopy(Common.longToByteArray(Math.round(EValue * 10000)), 0, data, 2, 4);
							System.arraycopy(Common.intToByteArray(Math.round(PValue * 1000)), 0, data, 6, 4);
							
							//Send packet
							if(thisDevice.isCloudConnectionLive()){
								thisDevice.sendCloudData(SmartPlugDevice.ALIAS_E_THRESHOLD, Double.toString(EValue));
								thisDevice.sendCloudData(SmartPlugDevice.ALIAS_P_THRESHOLD, Float.toString(PValue));
							}else
						    	thisDevice.sendLocalData(data);
							
							sendMessageHandlers.get(AToDHandler.PACKET_SET_THRESHOLD).startTimer(true);
							sendThresholdDialog.dismiss();
							
							message = "Threshold values sent";
							
						}catch(NumberFormatException e){
						}
						
						Toast.makeText(ACT, message, Toast.LENGTH_SHORT).show();
					}
				});
			}
		});
		sendThresholdDialog.show();
	}

	/**
	 * Packet 3.2.3/3.4.3 - Packet to set power saving mode
	 */
	private void sendPowerSaving(){
		if(thisDevice.getLocalConnectionStatus() == SmartPlugDevice.LOCAL_CONNECTION_LIVE){ //Socket not created, alert user and no action will be taken
			//Proceed
		} else if(thisDevice.isCloudConnectionLive()){
			//Proceed
		} else{
			Toast.makeText(ACT, getString(R.string.message_no_connection), Toast.LENGTH_SHORT).show();
			return;
		}
		
		/* Show dialog for threshold input */
		final AlertDialog PowerSavingDialog = new AlertDialog.Builder(ACT).create();
		View v = ACT.getLayoutInflater().inflate(R.layout.dialog_power_saving, null, false);
		
		PowerSavingDialog.setTitle("Set Power Saving");
		PowerSavingDialog.setView(v);
		PowerSavingDialog.setButton(AlertDialog.BUTTON_POSITIVE, "OK", (DialogInterface.OnClickListener) null);
		PowerSavingDialog.setButton(AlertDialog.BUTTON_NEGATIVE, "Cancel", (DialogInterface.OnClickListener) null);
		PowerSavingDialog.setCancelable(true);
		PowerSavingDialog.setOnShowListener(new DialogInterface.OnShowListener() {
			@Override
			public void onShow(DialogInterface dialog) {
				Button b = PowerSavingDialog.getButton(AlertDialog.BUTTON_POSITIVE);
				b.setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						
						//Form packet
						byte[] data  = new byte[4];
						data[0] = (byte) 0xa7;
						data[1] = (byte) 0x04;
						
						CheckBox checkbox_power_saving = (CheckBox) PowerSavingDialog.findViewById(R.id.checkbox_power_saving);
						
						try{
							int interval_value = Integer.parseInt(((EditText) PowerSavingDialog.findViewById(R.id.edittext_updateInterval)).getText().toString());
							
							if(checkbox_power_saving.isChecked()){
								if(interval_value<5 || interval_value>60){
									throw new NumberFormatException("When power saving mode is enabled, update interval should be 5-60 seconds");
								}
							}else{
								if(interval_value<1 || interval_value>60){
									throw new NumberFormatException("Update interval should be 5-60 seconds");
								}
							}
							
							data[2] = (byte) (checkbox_power_saving.isChecked() ? 0x01 : 0x00);
							data[3] = Common.intToByteArray(interval_value)[0];
							
							if(thisDevice.getLocalConnectionStatus() == SmartPlugDevice.LOCAL_CONNECTION_LIVE){
								thisDevice.setLocalSocketReadTimeout(interval_value * 1000, false);
								thisDevice.sendLocalData(data);
							} else if(thisDevice.isCloudConnectionLive()){
								thisDevice.sendCloudData(SmartPlugDevice.ALIAS_POWER_SAVING, checkbox_power_saving.isChecked() ? "1" : "0");
								thisDevice.sendCloudData(SmartPlugDevice.ALIAS_INTERVAL, Integer.toString(interval_value));
							} else{
								throw new Exception(getString(R.string.message_no_connection));
							}					
							
//							sendMessageHandlers.get(AToDHandler.PACKET_SET_POWER_SAVING).startTimer();
							Toast.makeText(ACT.getApplicationContext(), "Power saving info sent", Toast.LENGTH_SHORT).show();
							PowerSavingDialog.dismiss();
							
						}catch(Exception e){
							Toast.makeText(ACT.getApplicationContext(), e.getMessage(), Toast.LENGTH_SHORT).show();
						}
					}
				});
			}
		});
		PowerSavingDialog.show();
	}

	/**
	 * Packet 3.2.4
	 * This request is only valid for local connection
	 */
	private void sendRequest24HourAvg(){
		// We send the request to retrieve 24hour data. The dialog will appear once the data comes.
		if(thisDevice.getLocalConnectionStatus() != SmartPlugDevice.LOCAL_CONNECTION_LIVE){ //Socket not created, alert user and no action will be taken
			Toast.makeText(ACT, "Device was Disconnected", Toast.LENGTH_SHORT).show();
			return;
		}
		
		if(thisDevice.isCloudConnectionLive()){
			// [SPEC NOT SUPPORTED]
			Toast.makeText(ACT, "24 Hour Data Not supported in Cloud mode", Toast.LENGTH_SHORT).show();
			return;
		}
		
		button_24hour_average.setEnabled(false);
    	thisDevice.sendLocalData(Common.hexStringToByteArray("a708"));
		sendMessageHandlers.get(AToDHandler.PACKET_REQ_24H_AVG).startTimer(true);
		Toast.makeText(ACT, "24 Hour Data request sent...", Toast.LENGTH_SHORT).show();
	}
	
	/**
	 * Packet 3.2.5
	 * Only send request if connected locally. This only happens in local connection.
	 */
	private void sendRequestSwitchTable(){
		if(thisDevice.getLocalConnectionStatus() != SmartPlugDevice.LOCAL_CONNECTION_LIVE){ //Socket not created, alert user and no action will be taken
			Toast.makeText(ACT, getString(R.string.message_no_connection), Toast.LENGTH_SHORT).show();
			return;
		}
		
		button_switch_table.setEnabled(false);
		
		thisDevice.sendLocalData(Common.hexStringToByteArray("a710"));
		switchTableRequested = true;
		sendMessageHandlers.get(AToDHandler.PACKET_REQ_SWITCH_TABLE).startTimer(true);
		Toast.makeText(ACT, "Scheduling table request sent...", Toast.LENGTH_SHORT).show();
	}
	
	/**
	 * Packet 3.2.6
	 */
	private void sendRequestCalibration(){
		if(thisDevice.getLocalConnectionStatus() != SmartPlugDevice.LOCAL_CONNECTION_LIVE){ //Socket not created, alert user and no action will be taken
			Toast.makeText(ACT, getString(R.string.message_no_connection), Toast.LENGTH_SHORT).show();
			return;
		}
		
		button_calibrate.setEnabled(false);
		thisDevice.sendLocalData(Common.hexStringToByteArray("a720"));
		sendMessageHandlers.get(AToDHandler.PACKET_REQ_CALIB).startTimer(true);
		Toast.makeText(ACT, "Calibration request sent...", Toast.LENGTH_SHORT).show();
	}
	
	/**
	 * Packet 3.2.7
	 * NOTE: this is changed to display the currently known details rather than requesting for a new one.
	 * The reason being that the cloud info is constantly updated so there's no need to request for it.
	 */
	private void sendRequestCloudInfo(){
		button_detail.setEnabled(false);
		button_enable_cloud_TLS.setEnabled(false);
		
		thisDevice.sendLocalData(Common.hexStringToByteArray("a740"));
		sendMessageHandlers.get(AToDHandler.PACKET_REQ_CLOUD).startTimer(true);
		Toast.makeText(ACT, "Cloud Information request sent...", Toast.LENGTH_SHORT).show();
	}
	
	/**
	 * Packet 3.2.8
	 */
	private void sendRequestDeviceInfo(){
		if(thisDevice.getLocalConnectionStatus() != SmartPlugDevice.LOCAL_CONNECTION_LIVE){ //Socket not created, alert user and no action will be taken
			Toast.makeText(ACT, getString(R.string.message_no_connection), Toast.LENGTH_SHORT).show();
			return;
		}
		
		button_set_threshold.setEnabled(false);
		button_power_saving.setEnabled(false);
		
		thisDevice.sendLocalData(Common.hexStringToByteArray("a780"));
		sendMessageHandlers.get(AToDHandler.PACKET_REQ_DEV_INFO).startTimer(true);
		Toast.makeText(ACT, "Device Information request sent...", Toast.LENGTH_SHORT).show();
	}
	
	/**
	 * Packet 3.2.9 
	 * @param values
	 * @param enable
	 */
	private void sendSetSwitchTable(int[] values, boolean enable){
		
		int dayOfWeek = 0;
		switch(Calendar.getInstance().get(Calendar.DAY_OF_WEEK)){
			case Calendar.SUNDAY:
				dayOfWeek = 0x00;
				break;
			case Calendar.MONDAY:
				dayOfWeek = 0x01;
				break;
			case Calendar.TUESDAY:
				dayOfWeek = 0x02;
				break;
			case Calendar.WEDNESDAY:
				dayOfWeek = 0x03;
				break;
			case Calendar.THURSDAY:
				dayOfWeek = 0x04;
				break;
			case Calendar.FRIDAY:
				dayOfWeek = 0x05;
				break;
			case Calendar.SATURDAY:
				dayOfWeek = 0x06;
				break;
		}
		
		if(thisDevice.isCloudConnectionLive()){
			StringBuilder sb = new StringBuilder();
			for(int i=0; i<values.length; i+=2){
				sb.append(String.format(Locale.ENGLISH, "%02d%02d,", values[i], values[i+1]));
			}
			
			sb.append(String.format(Locale.ENGLISH, "%02d%02d%02d,",
					dayOfWeek,
					Calendar.getInstance().get(Calendar.HOUR_OF_DAY),
					Calendar.getInstance().get(Calendar.MINUTE)));
			sb.append(enable? "01":"00");
			
			thisDevice.sendCloudData(SmartPlugDevice.ALIAS_SCHEDULE_TABLE, sb.toString());
			Toast.makeText(ACT, "Scheduling Information sent...", Toast.LENGTH_SHORT).show();
			
		}else if(thisDevice.getLocalConnectionStatus() == SmartPlugDevice.LOCAL_CONNECTION_LIVE){ //Socket not created, alert user and no action will be taken
			//OP Code
			byte[] data = new byte[30];
			data[0] = (byte) 0xa7;
			data[1] = (byte) 0x91;
			
			//Switch table values
			for(int i=0; i<24; i++){
				data[i+2] = (byte) values[i];
			}
			
			data[26] = (byte) dayOfWeek;
			data[27] = (byte) Calendar.getInstance().get(Calendar.HOUR_OF_DAY);
			data[28] = (byte) Calendar.getInstance().get(Calendar.MINUTE);
			data[29] = (byte) (enable? 0x01:0x00);
			
			thisDevice.sendLocalData(data);
			Toast.makeText(ACT, "Scheduling Information sent...", Toast.LENGTH_SHORT).show();
		}else{
			Toast.makeText(ACT, getString(R.string.message_no_connection), Toast.LENGTH_SHORT).show();
		}
	}
	
	/**
	 * Packet 3.2.10
	 */
	private void sendSetCalibration(float desiredV, float desiredC){
		if(thisDevice.getLocalConnectionStatus() != SmartPlugDevice.LOCAL_CONNECTION_LIVE){ //Socket not created, alert user and no action will be taken
			Toast.makeText(ACT, getString(R.string.message_no_connection), Toast.LENGTH_SHORT).show();
			return;
		}
		
		byte[] data = new byte[10];
		data[0] = (byte) 0xa7;
		data[1] = (byte) 0x92;
		
		int desiredVRescale = (int)(desiredV*100000);
		int desiredCRescale = (int)(desiredC*100000);
		
		System.arraycopy(
				Common.intToByteArray(desiredVRescale),
				0,
				data,
				2, 4);
		System.arraycopy(
				Common.intToByteArray(desiredCRescale),
				0,
				data,
				6, 4);
		
		thisDevice.setLocalSocketReadTimeout(CALIB_READ_TIMEOUT, true);
		thisDevice.sendLocalData(data);
		Toast.makeText(ACT, "Calibration Update sent...", Toast.LENGTH_SHORT).show();
		
		calibrationInProgressDialog = new ProgressDialog(ACT);
		calibrationInProgressDialog.setTitle("Calibration In Progress");
		calibrationInProgressDialog.setMessage(getString(R.string.message_please_wait));
		calibrationInProgressDialog.setCancelable(false);
		calibrationInProgressDialog.setCanceledOnTouchOutside(false);
		calibrationInProgressDialog.show();
		sendMessageHandlers.get(AToDHandler.PACKET_SET_CALIB).startTimer(5000, false);
	}
	
	/**
	 * Packet 3.2.11
	 * @param enable
	 */
	private void sendSetEnableCloudTLS(boolean enable){
		if(thisDevice.getLocalConnectionStatus() != SmartPlugDevice.LOCAL_CONNECTION_LIVE){ //Socket not created, alert user and no action will be taken
			Toast.makeText(ACT, getString(R.string.message_no_connection), Toast.LENGTH_SHORT).show();
			return;
		}
		
		button_enable_cloud_TLS.setEnabled(false);
		button_update_cert.setEnabled(false);
		
		byte[] data = new byte[3];
		data[0] = (byte) 0xa7;
		data[1] = (byte) 0x94;
		data[2] = (byte) (enable? 0x01:0x00);
		thisDevice.sendLocalData(data);
		Toast.makeText(ACT, String.format(Locale.ENGLISH, "TLS Updating..."), Toast.LENGTH_SHORT).show();
		
		// Display the cloud info after sending. Just for confirmation.
		displayDeviceDetailDialog = true;
		sendMessageHandlers.get(AToDHandler.PACKET_REQ_CLOUD).startTimer(true);
	}
	
	/**
	 * Packet 3.2.12
	 */
	private void sendSetExositeCertificate(ArrayList<Byte> data){
		if(thisDevice.getLocalConnectionStatus() != SmartPlugDevice.LOCAL_CONNECTION_LIVE){ //Socket not created, alert user and no action will be taken
			Toast.makeText(ACT, getString(R.string.message_no_connection), Toast.LENGTH_SHORT).show();
			return;
		}
		
		int length = data.size() - 1;
		
		byte[] dataByte = new byte[length + 5];
		
		// Fill OP code
		dataByte[0] = (byte) 0xa7;
		dataByte[1] = (byte) 0x98;
		
		// Fill length
		dataByte[2] = (byte) length;
		dataByte[3] = (byte) (length>>8);
		
		// Fill data
		for (int i=0; i<length; i++) {
			dataByte[i+4] = data.get(i);
	    }
		
		// Fill checksum
		dataByte[length+4] = data.get(length);
		
		thisDevice.sendLocalData(dataByte);
		sendMessageHandlers.get(AToDHandler.PACKET_SET_EXOSITE_CA).startTimer(true);
		Toast.makeText(ACT, "Certificate sent...", Toast.LENGTH_SHORT).show();
	}
	
	@Override
	public void timeoutUpdate(int packet){
		
		StringBuilder sb = new StringBuilder();
		sb.append("Data receive time out for getting ");
		
		switch(packet){
			case AToDHandler.PACKET_SET_RELAY:
				sb.append("relay");
				break;
				
			case AToDHandler.PACKET_SET_THRESHOLD:
				sb.append("threshold");
				break;
				
			case AToDHandler.PACKET_SET_POWER_SAVING:
				sb.append("power saving");
				break;
				
			case AToDHandler.PACKET_REQ_24H_AVG:
				sb.append("24h average power");
				button_24hour_average.setEnabled(true);
				break;
				
			case AToDHandler.PACKET_REQ_SWITCH_TABLE:
				sb.append("schedule table (1)");
				break;
				
			case AToDHandler.PACKET_REQ_CALIB:
				button_calibrate.setEnabled(true);
				sb.append("calibration");
				break;
				
			case AToDHandler.PACKET_REQ_CLOUD:
				button_detail.setEnabled(true);
				button_enable_cloud_TLS.setEnabled(true);
				sb.append("cloud control");
				break;
				
			case AToDHandler.PACKET_REQ_DEV_INFO:
				sb.append("device information");
				break;
				
			case AToDHandler.PACKET_SET_SWITCH_TABLE:
				sb.append("schedule table (2)");
				break;
				
			case AToDHandler.PACKET_SET_CALIB:
				//Nothing
				break;
				
			case AToDHandler.PACKET_SET_EXOSITE_SSL:
				sb.append("Cloud SSL confirmation");
				break;
				
			case AToDHandler.PACKET_SET_EXOSITE_CA:
				try{
					updateCertDialog.dismiss();
				} catch (Exception e){}
				sb.append("Cloud Certificate update confirmation");
				break;
				
			default: //Unknown packet
				sb.append("UNKNOWN");
				break;
		}
		
		sb.append(" data");
		Log.e(DEBUG_CLASS, sb.toString());
		Toast.makeText(getActivity(), sb.toString(), Toast.LENGTH_SHORT).show();
	}
	
	//================================= GUI Updates from Threads =================================
	
	/**
	 * Same API, but won't implement the GuiUpdateCallback because this method is only accessable from the parent Activity
	 * @param tcpThreadCode
	 * @param device
	 * @param args
	 * @return
	 */
	public int LocalConnectionUpdate(final short tcpThreadCode, SmartPlugDevice device) {
		switch (tcpThreadCode) {
			case LocalConnectionThread.OP_CODE_3_1_1: // Packet 3.1.1 - Packet to Update Metrology Data
				updateNormalData();
				break;
	
			case LocalConnectionThread.OP_CODE_3_1_2: // Packet 3.1.2 - Packet to update average energy at hourly frequency
				updateHourlyAverage();
				break;
	
			case LocalConnectionThread.OP_CODE_3_1_3: // Packet 3.1.3 - Packet to update device status
				updateDeviceStatus(true);
				break;
	
			case LocalConnectionThread.OP_CODE_3_1_4: // Packet 3.1.4 - Warning message for over energy and power consumption threshold
				updateWarningMessage(WARNING_UPDATE_ALL);
				break;
	
			case LocalConnectionThread.OP_CODE_3_1_5: // Packet 3.1.5 - Average energy (KWh) of last 24 hours
				update24HourEnergy();
				break;
				
			case LocalConnectionThread.OP_CODE_3_1_6: // Packet 3.1.6 - Device's default switch table
				if(switchTableRequested){
					switchTableRequested = false;
					showSwitchTable();
				}
				break;
				
			case LocalConnectionThread.OP_CODE_3_1_7: // Packet 3.1.7 - Cloud specific requirements
				updatedCloudInfo();
				break;
				
			case LocalConnectionThread.OP_CODE_3_1_8: // Packet 3.1.8 - Metrology calibration specific requirements
				updatedCalibration();
				break;
				
			case LocalConnectionThread.OP_CODE_3_1_9: // Packet 3.1.9 - Indicating success in updating Exosite SSL CA certificate
				updatedCertificate();
				break;
	
			case LocalConnectionThread.SOCKET_CREATION_SUCCESS: // Socket creation success, set text
				updateConnectionCreationSuccess(false);
				break;
	
			case LocalConnectionThread.SOCKET_CREATION_FAIL: // Socket creation error & retry dialog
				updateSocketCreationError();
				break;
				
			case LocalConnectionThread.DATA_READ_FAIL:
				// Nothing
				break;
				
			case LocalConnectionThread.DATA_SEND_FAIL:
				// Nothing
				break;
				
			case LocalConnectionThread.THREAD_END_MSG_NORMAL:
				resetToDefaultState();
				break;
				
			case LocalConnectionThread.THREAD_END_MSG_ERROR_SOCKET: // Cannot create Socket dialog
				if(userTriggeredOff)
					resetToDefaultState();
				else
					enter15SecRetryState();
				break;
				
			case LocalConnectionThread.THREAD_END_MSG_ERROR_SEND: // Data Send error
				if(userTriggeredOff)
					resetToDefaultState();
				else
					enter15SecRetryState();
				break;
				
			case LocalConnectionThread.THREAD_END_MSG_ERROR_RECV: // Data Receive error
				if(userTriggeredOff)
					resetToDefaultState();
				else
					enter15SecRetryState();
				break;
				
			default:
				break;
		}
		
		return 0;
	}
	
	/**
	 * Same API, but won't implement the GuiUpdateCallback because this method is only accessable from the parent Activity
	 * @param tcpThreadCode
	 * @param device
	 * @param arg
	 * @return
	 */
	public int CloudConnectionUpdate(short cloudThreadCode, SmartPlugDevice device, String msg) {
		switch(cloudThreadCode){
			case CloudConnectionThread.CLOUD_UPDATE_3_3_1:
				updateNormalData();
				break;
				
			case CloudConnectionThread.CLOUD_UPDATE_3_3_2: // Packet 3.1.2 - Packet to update average energy at hourly frequency
				updateHourlyAverage();
				break;
	
			case CloudConnectionThread.CLOUD_UPDATE_3_3_3: // Packet 3.1.3 - Packet to update device status
				updateDeviceStatus(true);
				break;
	
			case CloudConnectionThread.CLOUD_UPDATE_3_3_4: // Packet 3.3.4 - Warning message for over energy consumption threshold
				updateWarningMessage(WARNING_UPDATE_OVER_ENERGY);
				break;
				
			case CloudConnectionThread.CLOUD_UPDATE_3_3_5: // Packet 3.3.5 - Warning message for over power consumption threshold
				updateWarningMessage(WARNING_UPDATE_OVER_POWER);
				break;
				
			case CloudConnectionThread.CLOUD_UPDATE_3_3_6: // Packet 3.3.6 - Device's default switch table
				// Since cloud packets are updated constantly, there's no need to display it here.
				// User can click the button_switch_table to get the latest table
				break;
				
			case CloudConnectionThread.CLOUD_CONNECTION_SUCCESS: // Socket creation success, set text
				updateConnectionCreationSuccess(true);
				break;
			
			case CloudConnectionThread.CLOUD_END_NORMAL_BECAUSE_SP_NOT_ACTIVE: // Socket creation success, set text
				resetToDefaultState();
				AlertDialog spActiveDialog = new AlertDialog.Builder(getActivity()).create();
				spActiveDialog.setTitle(getString(R.string.dialog_title_warning));
				spActiveDialog.setMessage(getString(R.string.message_sp_not_running));
				spActiveDialog.setButton(DialogInterface.BUTTON_NEUTRAL, "OK", (DialogInterface.OnClickListener)null);
				spActiveDialog.show();
				break;
				
			case CloudConnectionThread.CLOUD_UPDATE_ERROR:
				resetToDefaultState();
				updateCloudConnectionError(msg);
				break;
				
			case CloudConnectionThread.CLOUD_END_NORMAL:
				resetToDefaultState();
				break;
				
			case CloudConnectionThread.CLOUD_END_ERROR:
				resetToDefaultState();
				warnUserConnectionError(msg);
				break;
				
			default:
				Toast.makeText(ACT, "Unknown incoming cloud data: CODE:" + cloudThreadCode, Toast.LENGTH_SHORT).show();
				break;
		}
		return 0;
	}
	
	/**
	 * Packet 3.1.1/3.3.1
	 */
	private void updateNormalData(){
		
		// Sometime metrology data comes right after any command sent. Add some delay for receiving.
		if(calibrationInProgressDialog != null && calibrationInProgressDialog.isShowing()){
			if(sendMessageHandlers.get(AToDHandler.PACKET_SET_CALIB).isIdle()){
				thisDevice.setLocalSocketReadTimeout(thisDevice.currentReadTimeout, true);
				calibrationInProgressDialog.dismiss();
				Toast.makeText(ACT, "Calibration Successful", Toast.LENGTH_SHORT).show();
			}
		}
		
		final long dataReceivedTime = thisDevice.getLatestData().getLastDataTime();
		// activity.autoscroll.setEnabled(true);
		
		/* Set GUI Text */
		power		.setText(String.format(Locale.ENGLISH, "%,.3f", thisDevice.getLatestData().activePower));
		averagePower.setText(String.format(Locale.ENGLISH, "%,.3f", thisDevice.getLatestData().avgPower));
		totalEnergy	.setText(String.format(Locale.ENGLISH, "%,.4f",	thisDevice.getLatestData().KWh));
		voltage		.setText(String.format(Locale.ENGLISH, "%,.3f", thisDevice.getLatestData().voltage));
		current		.setText(String.format(Locale.ENGLISH, "%,.3f", thisDevice.getLatestData().current));
		frequency	.setText(String.format(Locale.ENGLISH, "%,.3f", thisDevice.getLatestData().frequency));
		cos			.setText(String.format(Locale.ENGLISH, "%,.4f", thisDevice.getLatestData().cos));
		var			.setText(String.format(Locale.ENGLISH, "%,.3f", thisDevice.getLatestData().reactivePower));
		va			.setText(String.format(Locale.ENGLISH, "%,.3f", thisDevice.getLatestData().apparentPower));
		
		/* To save resources and maintain speed, delete old data when auto-scroll */
		// TODO - [MAY NOT FIX] - due to design issue, graph may not be ready when data arrives.
		// There's roughly 1 second where this case will fail if there's incoming data from service but the graph has not been generated yet.
		// Datum during this time period will be dropped in the graph, but retains in the database.
		if(graphReady){
			chartStartingTime = dataReceivedTime;
			
			multiple_chartRenderer.setXAxisMin(dataReceivedTime - DATA_RANGE);
			multiple_chartRenderer.setXAxisMax(dataReceivedTime);
			
			// Only remove data that is out of graphing area
			if (mTSActivePower.getItemCount() > 0 && (dataReceivedTime - DATA_RANGE) > mTSActivePower.getX(0)) {
				mTSActivePower.remove(0);
				mTSReactivePower.remove(0);
				mTSApparentPower.remove(0);
				mTSVoltage.remove(0);
				mTSCurrent.remove(0);
				mTSPowerFactor.remove(0);
				mTSFrequency.remove(0);
			}
			
			/* aChartEngine data update */
			mTSActivePower.add(dataReceivedTime, thisDevice.getLatestData().activePower);
			mTSReactivePower.add(dataReceivedTime, thisDevice.getLatestData().reactivePower);
			mTSApparentPower.add(dataReceivedTime, thisDevice.getLatestData().apparentPower);
			mTSVoltage.add(dataReceivedTime, thisDevice.getLatestData().voltage);
			mTSCurrent.add(dataReceivedTime, thisDevice.getLatestData().current);
			mTSPowerFactor.add(dataReceivedTime, thisDevice.getLatestData().cos);
			mTSFrequency.add(dataReceivedTime, thisDevice.getLatestData().frequency);

			/* Set appropriate graphing area for the Y axis */
			TimeSeries series = getTimeSeriesFromSelection(thisDevice.CurrentGraphSelection);
			int hardMin = -1;
			int hardMax = -1;
			if(series.getTitle().equals(mNamePowerFactor)){	//In case of power factor, data range is strictly [0, 1]
				hardMin = 0;
				hardMax = 1;
			}
			adjustGraphingAreaYAxis(series, hardMin, hardMax);
			
			/* Repaint plot */
			mChart.repaint();
		}
	}
	
	/**
	 * Packet 3.1.2/3.3.2
	 */
	private void updateHourlyAverage(){
		hourlyAveragePower.setText(String.valueOf(thisDevice.latestAvgEnergyHourly.avgEnergy));
	}
	
	/**
	 * Packet 3.1.3/3.3.3
	 */
	private void updateDeviceStatus(boolean showStatusToast){
		sendMessageHandlers.get(AToDHandler.PACKET_REQ_DEV_INFO).cancelTimer();
		button_set_threshold.setEnabled(true);
		button_power_saving.setEnabled(true);
		
		if(showStatusToast){
			Toast.makeText(ACT,
					"Power saving "+ (thisDevice.latestDeviceStatus.powerSavingEnable? "enabled":"disabled")
					+ "\nUpdate Interval: " + String.valueOf(thisDevice.latestDeviceStatus.updateInterval)
					+ "\nDevice Turned " + (thisDevice.latestDeviceStatus.deviceOn? "On" : "Off"),
					Toast.LENGTH_SHORT).show();
		}
		
		//Set relay switch
		silentSetOnOffSwitChcheck(thisDevice.latestDeviceStatus.deviceOn, true);
		
		// Display dialogs accordingly.
		if(displayThresholdStatusDialog){
			
			displayThresholdStatusDialog = false;
			AlertDialog thresholdDialog = new AlertDialog.Builder(ACT).create();
			thresholdDialog.setTitle("Device Threshold Status");
			
			View v = ACT.getLayoutInflater().inflate(R.layout.dialog_threshold_status, null, false);
			TextView value_E_threshold = (TextView) v.findViewById(R.id.text_energy_threshold);
			value_E_threshold.setText(String.valueOf(thisDevice.latestDeviceStatus.energyThreshold));
			TextView value_P_threshold = (TextView) v.findViewById(R.id.text_power_threshold);
			value_P_threshold.setText(String.valueOf(thisDevice.latestDeviceStatus.powerThreshold));
			thresholdDialog.setView(v);
			
			thresholdDialog.setButton(AlertDialog.BUTTON_POSITIVE, "OK", (DialogInterface.OnClickListener) null);
			thresholdDialog.setButton(AlertDialog.BUTTON_NEUTRAL, "Update Values", new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog, int which) {
					// Update values
					sendSetThreshold();
				}
			});
			thresholdDialog.show();
			
		}else if(displayPowerSavingStatusDialog){
			
			displayPowerSavingStatusDialog = false;
			AlertDialog powerSavingDialog = new AlertDialog.Builder(ACT).create();
			powerSavingDialog.setTitle("Power Saving Status");
			
			View v = ACT.getLayoutInflater().inflate(R.layout.dialog_power_saving_status, null, false);
			TextView status_PowerSaving = (TextView) v.findViewById(R.id.text_power_saving);
			status_PowerSaving.setText(thisDevice.latestDeviceStatus.powerSavingEnable? "On":"Off");
			TextView value_PowerSavingInterval = (TextView) v.findViewById(R.id.text_power_saving_interval);
			value_PowerSavingInterval.setText(String.valueOf(thisDevice.latestDeviceStatus.updateInterval));
			powerSavingDialog.setView(v);
			
			powerSavingDialog.setButton(AlertDialog.BUTTON_POSITIVE, "OK", (DialogInterface.OnClickListener) null);
			powerSavingDialog.setButton(AlertDialog.BUTTON_NEUTRAL, "Update Power Saving", new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog, int which) {
					// Update values
					sendPowerSaving();
				}
			});
			powerSavingDialog.show();
		}
		
		/* Change the switch accordingly */
		
	}
		
	/**
	 * Packet 3.1.4, 3.3.4, and 3.3.5
	 */
	private void updateWarningMessage(short warning){
		
	    if((warning & WARNING_UPDATE_OVER_ENERGY) == WARNING_UPDATE_OVER_ENERGY){
	    	if (thisDevice.latestWarningOverEnergy.hasNewValue){
				totalEnergy.setTextColor(getResources().getColor(R.color.TI_COLOR_PANTONE_1807));
				totalEnergy.startAnimation(new WarningAlphaAnimation(totalEnergy));
		    }
	    }
	    
	    if((warning & WARNING_UPDATE_OVER_POWER) == WARNING_UPDATE_OVER_POWER){
	    	if (thisDevice.latestWarningOverPower.hasNewValue){
		    	power.setTextColor(getResources().getColor(R.color.TI_COLOR_PANTONE_1807));
		    	power.startAnimation(new WarningAlphaAnimation(power));
		    }
	    }
	    
	    if((warning & WARNING_UPDATE_MOD_ERROR) == WARNING_UPDATE_MOD_ERROR){
	    	
	    }
	}
		
	/**
	 * Packet 3.1.5
	 */
	private void update24HourEnergy(){
		sendMessageHandlers.get(AToDHandler.PACKET_REQ_24H_AVG).cancelTimer();
		button_24hour_average.setEnabled(true);
		
		ArrayList<Float> data = thisDevice.latestAvgEnergyDaily.get24HourData();
		
		List<HashMap<String, String>> fillMaps = new ArrayList<HashMap<String, String>>();
		String[] from = new String[] {"col_1", "col_2"};
		
		// 1st data
		HashMap<String, String> map = new HashMap<String, String>();
		map.put("col_1", "Last hour: ");
		map.put("col_2", data.get(0).toString());
		fillMaps.add(map);
		
		// 2nd-24th data
		for(int i=1; i<24; i++){
			map = new HashMap<String, String>();
			map.put("col_1", "Past " + i + "-" + Integer.toString(i + 1) + " hour:");
			map.put("col_2", data.get(i).toString());
			fillMaps.add(map);
		}
		
		SimpleAdapter a = new SimpleAdapter(
				ACT,
				fillMaps,
				R.layout.listview_item_two_textview,
				from,
				new int[] {R.id.lv_text1, R.id.lv_text2});
		
		/* Show dialog for threshold input */				
		AlertDialog.Builder builder = new AlertDialog.Builder(ACT);
		builder.setTitle("24 Hour Average History");
		builder.setAdapter(a, (DialogInterface.OnClickListener) null);
		builder.setNeutralButton("OK", (DialogInterface.OnClickListener) null);
		builder.setCancelable(true);
		builder.create();
		builder.show();
	}
	
	/**
	 * Displays the switch table
	 */
	private void showSwitchTable(){
		
		if(!thisDevice.isCloudConnectionLive()){
			//Local connection
			//If timer is idle, it means the message is sent from Smart Plug voluntarily. Therefore don't display it.
			if(sendMessageHandlers.get(AToDHandler.PACKET_REQ_SWITCH_TABLE).isIdle()){
				return;
			}
			
			button_switch_table.setEnabled(true);
			sendMessageHandlers.get(AToDHandler.PACKET_REQ_SWITCH_TABLE).cancelTimer();
		}
		
		View v = ACT.getLayoutInflater().inflate(R.layout.dialog_scheduling, null, false);
		
		final ArrayList<EditText> tableofEditText = new ArrayList<EditText>(24);
		
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_weekday_wakeup_hour));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_weekday_wakeup_minute));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_weekday_leave_hour));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_weekday_leave_minute));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_weekday_return_hour));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_weekday_return_minute));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_weekday_sleep_hour));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_weekday_sleep_minute));
		
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_saturday_wakeup_hour));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_saturday_wakeup_minute));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_saturday_leave_hour));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_saturday_leave_minute));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_saturday_return_hour));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_saturday_return_minute));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_saturday_sleep_hour));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_saturday_sleep_minute));
		
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_sunday_wakeup_hour));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_sunday_wakeup_minute));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_sunday_leave_hour));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_sunday_leave_minute));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_sunday_return_hour));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_sunday_return_minute));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_sunday_sleep_hour));
		tableofEditText.add((EditText) v.findViewById(R.id.schedule_sunday_sleep_minute));

		//Set Text
		for(int i=0; i<8; i++){
			tableofEditText.get(i).setText(Integer.toString(thisDevice.latestSwitchTable.weekdaySchedule[i]));
		}
		for(int i=0; i<8; i++){
			tableofEditText.get(8+i).setText(Integer.toString(thisDevice.latestSwitchTable.saturdaySchedule[i]));
		}
		for(int i=0; i<8; i++){
			tableofEditText.get(16+i).setText(Integer.toString(thisDevice.latestSwitchTable.sundaySchedule[i]));
		}
		
		//Active
		final CheckBox active = ((CheckBox) v.findViewById(R.id.checkbox_enable_scheduling));
		active.setChecked(thisDevice.latestSwitchTable.active);
		active.setOnCheckedChangeListener(new OnCheckedChangeListener(){
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				for(EditText e : tableofEditText){
					e.setEnabled(isChecked);
				}
			}
		});
		
		final AlertDialog scheduleDialog = new AlertDialog.Builder(ACT).create();
		scheduleDialog.setView(v);
		scheduleDialog.setButton(AlertDialog.BUTTON_NEUTRAL, "OK", (DialogInterface.OnClickListener) null);
		scheduleDialog.setButton(AlertDialog.BUTTON_POSITIVE, "Send Update", (DialogInterface.OnClickListener) null);
		scheduleDialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);
		scheduleDialog.setOnShowListener(new DialogInterface.OnShowListener() {
			@Override
			public void onShow(DialogInterface dialog) {
				
				Button b = scheduleDialog.getButton(AlertDialog.BUTTON_POSITIVE);
				b.setOnClickListener(new View.OnClickListener(){

					@Override
					public void onClick(View v) {
						int[] values = new int[24];
						
						for(int i=0; i<24; i++){
							
							EditText e = tableofEditText.get(i);
							
							if(e.getText().toString() == null){
								Toast.makeText(ACT, "Invalid values", Toast.LENGTH_SHORT).show();
								return;
							}
							
							values[i] = Integer.parseInt(e.getText().toString());
							if(i%2==0){ //hour
								if(values[i]<0 || values[i]>23){
									Toast.makeText(ACT, "Invalid value. Hour value range: [0, 23]", Toast.LENGTH_SHORT).show();
									return;
								}
							}else{ //minute
								if(values[i]<0 || values[i]>60){
									Toast.makeText(ACT, "Invalid value. Minute value range: [0, 59]", Toast.LENGTH_SHORT).show();
									return;
								}
							}
							
							//All checking passes, proceed to next value.
						}
						
						sendSetSwitchTable(values, active.isChecked());
						scheduleDialog.dismiss();
					}
				});
			}
		});
		scheduleDialog.show();
	}
	
	/**
	 * Packet 3.1.7
	 */
	private void updatedCloudInfo(){
		if(sendMessageHandlers.get(AToDHandler.PACKET_REQ_CLOUD).isIdle()){
			return;
		}
		
		//If we have a timer running, it means it's requested previously
		sendMessageHandlers.get(AToDHandler.PACKET_REQ_CLOUD).cancelTimer();
		button_detail.setEnabled(true);
		button_enable_cloud_TLS.setEnabled(true);
		button_update_cert.setEnabled(true);
		
		if(displayDeviceDetailDialog){ //Device Detail button requested update, show dialog
			showDeviceDetailDialog(true);
			displayDeviceDetailDialog = false;
			
		} else {  // Enable TLS button requested update, DON'T show dialog. Show the TLS dialog instead.
			AlertDialog enTLSDialog = new AlertDialog.Builder(ACT).create();
			enTLSDialog.setTitle("Secure Cloud Connection");
			final CheckBox en = new CheckBox(ACT);
			en.setText("Enable");
			en.setChecked(thisDevice.isDeviceCloudConnectionSecure());
			enTLSDialog.setView(en);
			enTLSDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel", (DialogInterface.OnClickListener) null);
			enTLSDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Update", new DialogInterface.OnClickListener(){
				@Override
				public void onClick(DialogInterface dialog, int which) {
					sendSetEnableCloudTLS(en.isChecked());
				}
			});
			enTLSDialog.show();
		}
	}
	
	/**
	 * Packet 3.1.8
	 */
	private void updatedCalibration(){
		if(sendMessageHandlers.get(AToDHandler.PACKET_REQ_CALIB).isIdle()){
			return;
		}
		
		// Stop the timer
		sendMessageHandlers.get(AToDHandler.PACKET_REQ_CALIB).cancelTimer();
		
		button_calibrate.setEnabled(true);
		
		// Show calibration stats gathering dialog
		// Show the dialog for calibration
		AlertDialog dialog = new AlertDialog.Builder(ACT).create();
		dialog.setTitle("Calibration");
		LayoutInflater inflater = (LayoutInflater) ACT.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		View layout = inflater.inflate(R.layout.dialog_calibration, null, false);
		
		final EditText desired_voltage = (EditText) layout.findViewById(R.id.calib_edittext_desired_voltage);
		final EditText desired_current = (EditText) layout.findViewById(R.id.calib_edittext_desired_current);
		
		TextView textVNoise = (TextView) layout.findViewById(R.id.calib_textview_voltage_noise);
		textVNoise.setText(String.format(Locale.ENGLISH, "%,.4f", thisDevice.latestMetrologyCalibration.voltageChannelNoise));
		TextView textCNoise = (TextView) layout.findViewById(R.id.calib_textview_current_noise);
		textCNoise.setText(String.format(Locale.ENGLISH, "%,.4f", thisDevice.latestMetrologyCalibration.currentChannelNoise));
		final TextView textVScale = (TextView) layout.findViewById(R.id.calib_textview_voltage_scale);
		textVScale.setText(String.format(Locale.ENGLISH, "%,.4f", thisDevice.latestMetrologyCalibration.voltageChannelScaleFactor));
		final TextView textCScale = (TextView) layout.findViewById(R.id.calib_textview_current_scale);
		textCScale.setText(String.format(Locale.ENGLISH, "%,.4f", thisDevice.latestMetrologyCalibration.currentChannelScaleFactor));
		
		dialog.setView(layout);
		dialog.setButton(AlertDialog.BUTTON_NEGATIVE, "Cancel", (DialogInterface.OnClickListener) null);
		dialog.setButton(AlertDialog.BUTTON_POSITIVE, "Update", new DialogInterface.OnClickListener(){
			@Override
			public void onClick(DialogInterface dialog, int which) {
				sendSetCalibration(
						Float.parseFloat(desired_voltage.getText().toString()),
						Float.parseFloat(desired_current.getText().toString())
						);
			}
		});
		dialog.show();
	}
	
	/**
	 * Packet 3.1.9
	 */
	private void updatedCertificate(){
		if(sendMessageHandlers.get(AToDHandler.PACKET_SET_EXOSITE_CA).isIdle()){
			return;
		}
		
		sendMessageHandlers.get(AToDHandler.PACKET_SET_EXOSITE_CA).cancelTimer();
		updateCertDialog.dismiss();
		
		AlertDialog successDialog = new AlertDialog.Builder(ACT).create();
		successDialog.setTitle("Success!");
		successDialog.setMessage("Certificate has been updated successfully.");
		successDialog.setButton(DialogInterface.BUTTON_NEUTRAL, "OK", (DialogInterface.OnClickListener) null);
		successDialog.show();
	}
	
	/**
	 * Updates the GUI about connection success
	 * @param isCloud - If it's cloud connection
	 */
	private void updateConnectionCreationSuccess(boolean isCloud){
		// Enable all fields for user to use
		button_switch_table.setEnabled(true);
		button_set_threshold.setEnabled(true);
		device_on_off.setEnabled(true);
		silentlySetConnectionSwitChcheck(true, true);
		button_power_saving.setEnabled(true);

		// Display Title
		if(isCloud){
			deviceStatus.setText("Cloud Connection (" + thisDevice.getCIK().substring(0, 6) + "...)");
		}else{
			// these buttons are only available for local connection
			button_enable_cloud_TLS.setEnabled(true);
			button_update_cert.setEnabled(true);
			button_24hour_average.setEnabled(true);
			button_calibrate.setEnabled(true);
			deviceStatus.setText("Local Connection (" + thisDevice.getLocalAddress().getHostAddress() + ")");
		}
		
		deviceStatus.setTextColor(getResources().getColor(R.color.TI_COLOR_PANTONE_3125));
		LocalBroadcastManager.getInstance(getActivity()).sendBroadcast(new Intent(DeviceListFragment.BROADCAST_DEVICE_LIST_REFRESH));
	}
	
	private void updateSocketCreationError(){
		deviceStatus.setText("Local Connection Internal Retry...(" + thisDevice.getLocalAddress().getHostAddress() + ")");
	}
	
	private void enter15SecRetryState(){
		silentlySetConnectionSwitChcheck(true, true);
		deviceStatus.setTextColor(getResources().getColor(R.color.TI_COLOR_PANTONE_1807));
		deviceStatus.setText("Local Connection Reties in " + (SmartPlugDevice.SOCKET_RECREATION_TIME/1000) + "s (" + thisDevice.getLocalAddress().getHostAddress() + ")");
		thisDevice.startLocalReconnectionTimer();
	}
	
	// == Cloud Reconnection Callbacks ==
	
	@Override
	public void onReconnectionTimerTick(long millisUntilFinished) {
		deviceStatus.setText("Local Connection Reties in " + (millisUntilFinished/1000) + "s (" + thisDevice.getLocalAddress().getHostAddress() + ")");
	}

	@Override
	public void onReconnectionTimerFinish() {
		deviceStatus.setText("Local Connection Connecting... (" + thisDevice.getLocalAddress().getHostAddress() + ")");
		ACT.getService().executeLocalConnection(thisDevice);	
	}
	
	// == Cloud Reconnection Callbacks END ==
	
	private void resetToDefaultState(){
		deviceStatus.setText("Offline");
		deviceStatus.setTextColor(getResources().getColor(R.color.TI_COLOR_PANTONE_1807));
		
		// Stop threads
		thisDevice.stopLocalThread();
    	thisDevice.stopCloudThread();
    	
    	// Terminate Reconnection Timer
    	thisDevice.stopLocalReconnectionTimer();
		
		/* Get the switch back to off */
		silentlySetConnectionSwitChcheck(false, true);
		
		// Reset everything to default
		button_24hour_average.setEnabled(false);
		button_switch_table	.setEnabled(false);
		button_set_threshold.setEnabled(false);
		device_on_off		.setEnabled(false);
		button_power_saving	.setEnabled(false);
		button_calibrate	.setEnabled(false);
		button_enable_cloud_TLS.setEnabled(false);
		button_update_cert	.setEnabled(false);
		
		LocalBroadcastManager.getInstance(getActivity()).sendBroadcast(new Intent(DeviceListFragment.BROADCAST_DEVICE_LIST_REFRESH));
	}
	
	private void updateCloudConnectionError(String msg){
		AlertDialog errorConnectionDialog = new AlertDialog.Builder(ACT).create();
		errorConnectionDialog.setTitle("Cloud Connection Error");
		errorConnectionDialog.setMessage(msg);
		errorConnectionDialog.setButton(DialogInterface.BUTTON_NEUTRAL, "OK", (DialogInterface.OnClickListener) null);
	}
	
	private void warnUserConnectionError(String msg){
		AlertDialog connectionFailDialog = new AlertDialog.Builder(ACT).create();
		connectionFailDialog.setTitle(getString(R.string.dialog_title_error));
		//connectionFailDialog.setMessage(getString(R.string.message_cloud_connection_error));
		connectionFailDialog.setMessage(msg);
		connectionFailDialog.setButton(DialogInterface.BUTTON_POSITIVE, "Re-Enable", new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				//TODO - [Exosite API Limitation]
				Toast.makeText(ACT, getText(R.string.message_feature_for_future), Toast.LENGTH_SHORT).show();
			}
		});
		connectionFailDialog.setButton(DialogInterface.BUTTON_NEUTRAL, "Cancel", (DialogInterface.OnClickListener) null);
		connectionFailDialog.show();
	}
	
	//==================================== Miscellaneous Methods ====================================
	
	public void silentlySetConnectionSwitChcheck(boolean check, boolean enable){
		connection_switch.setEnabled(enable);
		Common.silentlySwitchCheck(connection_switch, check, connection_switch_listener);
	}
	
	public void silentSetOnOffSwitChcheck(boolean check, boolean enable){
		device_on_off.setEnabled(enable);
		Common.silentlySwitchCheck(device_on_off, check, device_on_off_listener);
	}
}

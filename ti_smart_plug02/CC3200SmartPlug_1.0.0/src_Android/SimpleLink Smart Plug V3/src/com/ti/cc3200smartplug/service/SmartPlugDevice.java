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

package com.ti.cc3200smartplug.service;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketException;
import java.security.cert.Certificate;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;

import javax.net.ssl.SSLSocket;

import android.content.Context;
import android.os.AsyncTask;
import android.os.CountDownTimer;

import com.ti.cc3200smartplug.data.DToA_AvgEnergyDaily;
import com.ti.cc3200smartplug.data.DToA_AvgEnergyHourly;
import com.ti.cc3200smartplug.data.DToA_CloudInfo;
import com.ti.cc3200smartplug.data.DToA_DeviceStatus;
import com.ti.cc3200smartplug.data.DToA_MetrologyCalibration;
import com.ti.cc3200smartplug.data.DToA_MetrologyData;
import com.ti.cc3200smartplug.data.DToA_SwitchTable;
import com.ti.cc3200smartplug.data.DToA_Warning;

/**
 * This device class contains essential info for a Smart Plug device.
 */
public class SmartPlugDevice {
	
	public interface ReconnectionTimerTickUpdate{
		public void onReconnectionTimerTick(long millisUntilFinished);
		public void onReconnectionTimerFinish();
	}

	// Exosite Default Data
	public static final String DEFAULT_MODEL	= "smartplugv2";
	public static final String DEFAULT_VENDOR	= "texasinstruments";

	// Exosite Data Aliases
	// 3.3.1 - Metrology Data
	public static final String ALIAS_POWER		= "power";
	public static final String ALIAS_VOLTAGE	= "volt";
	public static final String ALIAS_CURRENT	= "current";
	public static final String ALIAS_FREQUENCY	= "freq";
	public static final String ALIAS_VAR		= "var";
	public static final String ALIAS_PF			= "pf";
	public static final String ALIAS_VA			= "va";
	public static final String ALIAS_KWH		= "kwh";
	public static final String ALIAS_AVG_POWER	= "pow_avg";

	// 3.3.2 - Hourly Average Energy
	public static final String ALIAS_HOURLY_E_AVG	= "enr_avg";

	// 3.3.3 - Relay and Control Data
	public static final String ALIAS_RELAY			= "control";
	public static final String ALIAS_POWER_SAVING	= "powmode";
	public static final String ALIAS_INTERVAL		= "reportint";
	public static final String ALIAS_E_THRESHOLD	= "enthld";
	public static final String ALIAS_P_THRESHOLD	= "powthld";

	// 3.3.4 - Over Energy Warning Message
	public static final String ALIAS_WARNING_OVER_E = "enr_mesg";

	// 3.3.5 - Over Power Warning Message
	public static final String ALIAS_WARNING_OVER_P = "pow_mesg";

	// 3.3.6 - Schedule Table
	public static final String ALIAS_SCHEDULE_TABLE = "schedule";
	
	// Available Connection
	private short mAvailableConnection;
	public static short AVAILABLE_CONNECTION_NONE	= 0x00;
	public static short AVAILABLE_CONNECTION_CLOUD	= 0x01;
	public static short AVAILABLE_CONNECTION_LOCAL	= 0x02;
	public static short AVAILABLE_CONNECTION_BOTH	= 0x03;
	
	// Connection status for local connection
	public static short LOCAL_CONNECTION_OFF		= 0x10;
	public static short LOCAL_CONNECTION_LIVE		= 0x11;
	public static short LOCAL_CONNECTION_TRYING		= 0x12;
	
	// Connection status for cloud connection
	public static short CLOUD_CONNECTION_OFF		= 0x20;
	public static short CLOUD_CONNECTION_LIVE		= 0x21;
	public static short CLOUD_CONNECTION_TRYING		= 0x22;
	
	public int currentReadTimeout;
	
	// Associated user
	private String assoUser;

	// Context for Database
	private Context mContext;
	
	public CountDownTimer socketReCreationTimer;
	public static final int SOCKET_RECREATION_TIME = 15000;
	private ReconnectionTimerTickUpdate callback;			//Strictly used for timer and fragment
	public boolean isTimerRunning;

	/** The connection threads */
	private LocalConnectionThread LCT;
	private CloudConnectionThread CCT;

	/** The local connection socket */
	private SSLSocket localSocket;

	// Identity fields
	public int database_id; // SQL database item id
	private String rid;
	private String name;
	private String mac;
	private String model;
	private String vendor;
	private String cik;
	private InetAddress localConnectionAddress; // The local connection address

	/* All Device-To-Android data */
	private DToA_MetrologyData		latestMetrologyData;
	public DToA_AvgEnergyHourly		latestAvgEnergyHourly;
	public DToA_DeviceStatus		latestDeviceStatus;
	public DToA_Warning				latestWarningOverEnergy;
	public DToA_Warning				latestWarningOverPower;
	public DToA_Warning				latestWarningModuleError;
	public DToA_AvgEnergyDaily		latestAvgEnergyDaily;
	public DToA_SwitchTable			latestSwitchTable; // Able to retrieve from SP, so no database needed
	public DToA_CloudInfo			latestCloudInfo;
	public DToA_MetrologyCalibration latestMetrologyCalibration;

	// Database
	public SPDeviceDataHandler historyDatabaseHandler;
	
	// Connection Preference
	private short	mConnectionPreference;
	public static final short CONNECTION_PREF_AUTO	= 0x00;
	public static final short CONNECTION_PREF_CLOUD	= 0x01;
	public static final short CONNECTION_PREF_LOCAL	= 0x02;
	
	// Graph selection
	public int CurrentGraphSelection;

	private boolean previousSession;

	/**
	 * Simple SmartPlugDevice constructor. No connectivity to anything unless
	 * either a InetAddress or CIK is added.
	 * 
	 * @param context
	 *            Caller context
	 * @param name
	 *            Name of the device
	 * @param mac
	 *            MAC address of the device
	 */
	public SmartPlugDevice(Context context, String name, String mac) {
		init(context, null, name, mac, null, DEFAULT_MODEL, DEFAULT_VENDOR, null, CONNECTION_PREF_AUTO, null);
	}

	/**
	 * Simple SmartPlugDevice constructor with local address. No cloud
	 * connectivity since CIK is missing.
	 * 
	 * @param context
	 *            Caller context
	 * @param name
	 *            Name of the device
	 * @param mac
	 *            MAC address of the device
	 * @param localAddress
	 *            Local address of the Smart Plug
	 */
	public SmartPlugDevice(Context context, String name, String mac, InetAddress localAddress, short connPref, String associatedUser) {
		init(context, null, name, mac, null, DEFAULT_MODEL, DEFAULT_VENDOR, localAddress, connPref, associatedUser);
	}

	/**
	 * Simple SmartPlugDevice constructor with cloud information only. No local connectivity.
	 * 
	 * @param context
	 *            Caller context
	 * @param rid
	 *            RID of the device
	 * @param name
	 *            Name of the device
	 * @param mac
	 *            MAC address of the device
	 * @param cik
	 *            unique identified of the device
	 */
	public SmartPlugDevice(Context context, String rid, String name, String mac, String cik, short connPref, String associatedUser) {
		init(context, rid, name, mac, cik, DEFAULT_MODEL, DEFAULT_VENDOR, null, connPref, associatedUser);
	}

	/**
	 * Simple SmartPlugDevice constructor with both local address and CIK.
	 * 
	 * @param context
	 *            Caller context
	 * @param rid
	 *            RID of the device
	 * @param name
	 *            Name of the device
	 * @param mac
	 *            MAC address of the device
	 * @param cik
	 *            unique identified of the device
	 * @param localAddress
	 *            Local address of the Smart Plug
	 * @param connPref
	 * 			  Connection Preference. {@link #CONNECTION_PREF_AUTO}, {@link #CONNECTION_PREF_CLOUD}, or {@link #CONNECTION_PREF_LOCAL}.
	 * @param associatedUser
	 * 			  Associated user name
	 */
	public SmartPlugDevice(Context context,  String rid, String name, String mac, String cik, InetAddress localAddress, short connPref, String associatedUser) {
		init(context, rid, name, mac, cik, DEFAULT_MODEL, DEFAULT_VENDOR, localAddress, connPref, associatedUser);
	}

	/**
	 * Full size SmartPlugDevice constructor.
	 * 
	 * @param context
	 *            Caller context.
	 * @param rid
	 *            RID of the device
	 * @param name
	 *            Name of the device
	 * @param mac
	 *            MAC address of the device
	 * @param cik
	 *            Unique identified of the device
	 * @param model
	 *            Model name. Default is {@link SmartPlugDevice.DEFAULT_MODEL}
	 * @param vendor
	 *            Vendor name. Default is {@link SmartPlugDevice.DEFAULT_VENDOR}
	 * @param localAddress
	 *            Local address of the Smart Plug
	 * @param connPref
	 * 			  Connection Preference. {@link #CONNECTION_PREF_AUTO}, {@link #CONNECTION_PREF_CLOUD}, or {@link #CONNECTION_PREF_LOCAL}.
	 * @param associatedUser
	 * 			  Associated user name
	 */
	public SmartPlugDevice(Context context,  String rid, String name, String mac, String cik, String model, String vendor, InetAddress localAddress, short connPref, String associatedUser) {
		init(context, rid, name, mac, cik, model, vendor, localAddress, connPref, associatedUser);
	}

	private void init(
			Context context,
			String rid,
			String name,
			String mac,
			String cik,
			String model,
			String vendor,
			InetAddress localConnectionAddress,
			short connPref,
			String associatedUser) {

		if (mac == null) {
			throw new NullPointerException("MAC address cannot be null");
		}
		
		socketReCreationTimer = null;
		isTimerRunning = false;
		
		mAvailableConnection = AVAILABLE_CONNECTION_NONE;
		
		assoUser = associatedUser;

		mContext = context;
		this.rid = rid;
		this.name = name;
		this.mac = mac.toUpperCase(Locale.ENGLISH);
		this.cik = cik;
		this.model = model;
		this.vendor = vendor;
		this.localConnectionAddress = localConnectionAddress;
		
		if(cik != null)
			mAvailableConnection |= AVAILABLE_CONNECTION_CLOUD;
		
		if(localConnectionAddress != null)
			mAvailableConnection |= AVAILABLE_CONNECTION_LOCAL;

//		activated = cik == null ? false : true;

		previousSession = false;
		
		mConnectionPreference = connPref; 

		// Initialize all values
		latestMetrologyData = new DToA_MetrologyData(0);
		latestAvgEnergyHourly = new DToA_AvgEnergyHourly(0);
		latestDeviceStatus = new DToA_DeviceStatus(0);
		latestWarningOverEnergy = new DToA_Warning(0);
		latestWarningOverPower = new DToA_Warning(0);
		latestWarningModuleError = new DToA_Warning(0);
		latestAvgEnergyDaily = new DToA_AvgEnergyDaily(0);
		latestSwitchTable = new DToA_SwitchTable(0);
		latestCloudInfo = new DToA_CloudInfo(0);
		latestMetrologyCalibration = new DToA_MetrologyCalibration(0);

		// Initialize database
		historyDatabaseHandler = new SPDeviceDataHandler(mContext, mac);
		
		CurrentGraphSelection = 0;
	}

	// ================================= Database Operations =================================

	/**
	 * Database usage only.
	 * 
	 * @return
	 */
	public long getDatabaseId() {
		return database_id;
	}

	/**
	 * Database usage only.
	 * 
	 * @return
	 */
	public void setId(int id) {
		this.database_id = id;
	}

	public boolean deleteMetrologyDatabase() {
		return historyDatabaseHandler.delteDatabase();
	}

	// ================================= Local/Exosite Connections =================================

	/**
	 * Starts the LocalConnection Thread communication.
	 * 
	 * @param smartPlugService
	 * @param inetAddresses
	 */
	public void startLocalThread(SmartPlugService smartPlugService, Certificate cert) {
		LCT = new LocalConnectionThread(smartPlugService, this, cert);
		LCT.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
	}

	/**
	 * Stops the LocalConnection Thread.
	 */
	public void stopLocalThread() {
		if (LCT != null)
			LCT.flag_TCPRunning = false;
	}

	/**
	 * Set's the local SSL socket for this device and starts connections. Throws
	 * exceptions if connection fails.
	 * 
	 * @param s
	 *            the SSL Socket
	 * @param i
	 *            socket address
	 * @param socket_create_timeout
	 *            socket creation timeout
	 * @throws IllegalArgumentException
	 *             if the given SocketAddress is invalid or not supported or the
	 *             timeout value is negative.
	 * @throws IOException
	 *             if the socket is already connected or an error occurs while
	 *             connecting.
	 */
	public void setLocalSocketAndConnect(SSLSocket s, InetSocketAddress i, int socket_create_timeout) throws IOException {
		localSocket = s;
		localSocket.connect(i, socket_create_timeout);
	}

	/**
	 * Starts the CloudConnection Thread communication.
	 */
	public void startCloudThread(SmartPlugService smartPlugService, boolean collectHistory) {
		CCT = new CloudConnectionThread(smartPlugService, this);
		CCT.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, collectHistory);
	}

	/**
	 * Stops the CloudConnection Thread.
	 */
	public void stopCloudThread() {
		if (CCT != null)
			CCT.flag_ExositeRunning = false;
	}

	public short getLocalConnectionStatus(){
		if (LCT == null)
			return LOCAL_CONNECTION_OFF;
		
		if (LCT.getStatus() == AsyncTask.Status.PENDING)	//This case should be extremely rare
			return LOCAL_CONNECTION_LIVE;
		
		if (LCT.getStatus() == AsyncTask.Status.RUNNING)
			return LOCAL_CONNECTION_LIVE;
		
		if (LCT.getStatus() == AsyncTask.Status.FINISHED){
			// Check timer
			if(socketReCreationTimer==null)
				return LOCAL_CONNECTION_OFF;
			else if(isTimerRunning)
				return CLOUD_CONNECTION_TRYING;
			else
				return LOCAL_CONNECTION_OFF;
		}
		
		// With everything case fails, return OFF
		return LOCAL_CONNECTION_OFF;
	}
	
	public void setTimerCallbackFragment(ReconnectionTimerTickUpdate callback){
		this.callback = callback;
	}
	
	public void startLocalReconnectionTimer(){
		socketReCreationTimer = new CountDownTimer(SOCKET_RECREATION_TIME, 1000){

			@Override
			public void onTick(long millisUntilFinished) {
				try{
					callback.onReconnectionTimerTick(millisUntilFinished);
				}catch(Exception e){
					// Fragmant may not be found. Do nothing here.
				};
			}

			@Override
			public void onFinish() {
				try{
					callback.onReconnectionTimerFinish();
				}catch(Exception e){
					// Fragmant may not be found. Do nothing here.
				};
			}
			
		};
		socketReCreationTimer.start();
	}
	
	public void stopLocalReconnectionTimer(){
		if(socketReCreationTimer != null){
    		socketReCreationTimer.cancel();
    	}
	}
	
	/**
	 * Checks if the cloud connection is live or not.
	 * 
	 * @return
	 */
	public boolean isCloudConnectionLive() {
		if (CCT == null)
			return false;
		
		if (!CCT.flag_ExositeRunning)
			return false;

		return true;
	}

	/**
	 * Send Data to the device through cloud.
	 * 
	 * @param alias
	 *            alias to write
	 * @param data
	 *            data to write
	 */
	public void sendCloudData(String alias, String data) {
		CCT.sendData(alias, data);
	}

	/**
	 * Send Data to the device through local connection.
	 * 
	 * @param data
	 *            data to write
	 */
	public void sendLocalData(byte[] data) {
		if (LCT != null)
			LCT.sendData(data);
	}

	public SSLSocket getLocalSocket() {
		return localSocket;
	}

	/**
	 * Set the local socket timeout, in miliseconds.
	 * 
	 * @param timeout
	 * @return true if set successful
	 */
	public boolean setLocalSocketReadTimeout(int timeout, boolean temporary) {
		if (localSocket == null)
			return false;

		try {
			localSocket.setSoTimeout(timeout);
			if(!temporary)
				currentReadTimeout = timeout;
		} catch (SocketException e) {
			e.printStackTrace();
			return false;
		}

		return true;
	}

	/**
	 * Close the local socket. This should only be used by the thread. For
	 * LocalConnection termination, call the method stopLocalThread().
	 */
	public void closeLocalSocket() {
		try {
			localSocket.close();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	// ================================= Get/Set Device Details =================================
	// ====== Setters should only be accessed via service in order to update the database =======

	public void setRID(String rid){
		this.rid = rid;
		
		if(rid == null)
			mAvailableConnection &= ~AVAILABLE_CONNECTION_CLOUD;
		else
			mAvailableConnection |= AVAILABLE_CONNECTION_CLOUD;
	}
	
	public String getRID(){
		return rid;
	}
	
	/**
	 * Gets device name.
	 * 
	 * @return name as string
	 */
	public String getName() {
		return name;
	}

	/**
	 * Sets device name.
	 * 
	 * @param name
	 *            as string
	 */
	public void setName(String name) {
		this.name = name;
	}

	/**
	 * Gets MAC address
	 * 
	 * @return MAC address as string, no colon
	 */
	public String getMAC() {
		return mac;
	}

	/**
	 * Sets MAC address
	 * 
	 * @param mac
	 *            MAC address as string, no colon
	 */
	public void setMAC(String mac) {
		this.mac = mac;
	}

	public String getModel() {
		return model;
	}

	public void setModel(String model) {
		this.model = model;
	}

	public String getVendor() {
		return vendor;
	}

	public void setVendor(String vendor) {
		this.vendor = vendor;
	}

	public String getCIK() {
		return cik;
	}

	public void setCIK(String cik) {
		this.cik = cik;
	}

	public void setLocalAddress(InetAddress localAddress) {
		localConnectionAddress = localAddress;
		
		if(localConnectionAddress == null)
			mAvailableConnection &= ~AVAILABLE_CONNECTION_LOCAL;
		else
			mAvailableConnection |= AVAILABLE_CONNECTION_LOCAL;
	}

	/**
	 * 
	 * @return the local {@link java.net.InetAddress InetAddress}. Can be null.
	 */
	public InetAddress getLocalAddress() {
		return localConnectionAddress;
	}

	/**
	 * One of these: {@link #AVAILABLE_CONNECTION_NONE}, {@link #AVAILABLE_CONNECTION_CLOUD}, {@link #AVAILABLE_CONNECTION_LOCAL}, or {@link #AVAILABLE_CONNECTION_BOTH}
	 * @return
	 */
	public short checkAvailableConnection(){
		return mAvailableConnection;
	}
	
	public void setAssociatedUser(String username){
		assoUser = username;
	}
	
	public String getAssociatedUser(){
		return assoUser;
	}
	
	public boolean isDeviceCloudConnectionSecure(){
		return (latestCloudInfo.cloudStatus & 0x04) == 0x04;
	}
	
	public void setConnectionPreference(short pref){
		mConnectionPreference = pref;
	}
	
	public short getConnectionPreference(){
		return mConnectionPreference;
	}
	
	// ================================= Metrology Data Operation =================================

	public void addData(DToA_MetrologyData d) {
		latestMetrologyData = d;

		// Add to database
		historyDatabaseHandler.addMetrologyData(d);
	}

	public DToA_MetrologyData getLatestData() {
		return latestMetrologyData;
	}

	/**
	 * Retrieves the entire Metrology data history from the database.
	 * 
	 * @param forGraph
	 *            indicates if this retrival is for graph or not. If yes, then
	 *            it'll only return data within the graphing range. If not, it
	 *            will retrieve all data.
	 * @param rang
	 * 			  the range of the graph, if the forGraph parameter is true.
	 * @return An ArrayList of history
	 */
	public ArrayList<DToA_MetrologyData> getPowerHistory(boolean forGraph, int range) {
		if(forGraph){
			long curTime = System.currentTimeMillis();
			return historyDatabaseHandler.getMetrologyDataInRange(curTime - range, curTime);
		}
		return historyDatabaseHandler.getAllMetrologyData();
	}

	public DToA_MetrologyData getVeryFirstData() {
		return historyDatabaseHandler.getMetrologyData(0);
	}

	// ================================= Miscellaneous =================================

	/**
	 * Indication if there was a previous session or not.
	 * 
	 * @param b
	 */
	public void setPreviousSession(boolean b) {
		previousSession = b;
	}

	/**
	 * If there was a previous session.
	 * 
	 * @return
	 */
	public boolean hasPreviousSession() {
		return previousSession;
	}
	
	/**
	 * Returns a list of HashMap, describing every possible value of this device.
	 * @param text1
	 * @param text2
	 * @return
	 */
	public List<HashMap<String, String>> getEverything(String text1, String text2, boolean showCloudInfo) {
		
		List<HashMap<String, String>> ret = new ArrayList<HashMap<String, String>>();
		
		HashMap<String, String> item1 = new HashMap<String, String>();
		item1.put(text1, "Name");
		item1.put(text2, name);
		ret.add(item1);
		
		HashMap<String, String> item2 = new HashMap<String, String>();
		item2.put(text1, "MAC Address");
		item2.put(text2, mac);
		ret.add(item2);
		
		HashMap<String, String> item3 = new HashMap<String, String>();
		item3.put(text1, "Exosite Client Model");
		item3.put(text2, model);
		ret.add(item3);
		
		HashMap<String, String> item4 = new HashMap<String, String>();
		item4.put(text1, "Exosite Vendor");
		item4.put(text2, vendor);
		ret.add(item4);
		
		HashMap<String, String> item5 = new HashMap<String, String>();
		item5.put(text1, "Device RID");
		item5.put(text2, rid);
		ret.add(item5);
		
		HashMap<String, String> item6 = new HashMap<String, String>();
		item6.put(text1, "Device CIK");
		item6.put(text2, cik);
		ret.add(item6);
		
		HashMap<String, String> item7 = new HashMap<String, String>();
		item7.put(text1, "Associated User");
		item7.put(text2, assoUser);
		ret.add(item7);
		
		HashMap<String, String> item8 = new HashMap<String, String>();
		item8.put(text1, "Local Address");
		String addr;
		try{
			addr = localConnectionAddress.getHostAddress();
		} catch(Exception e){
			addr = "null";
		}			
		item8.put(text2, addr);
		ret.add(item8);
		
		HashMap<String, String> item9 = new HashMap<String, String>();
		item9.put(text1, "Cloud Connection");
		item9.put(text2, Boolean.toString(isCloudConnectionLive()));
		ret.add(item9);
		
		HashMap<String, String> item10 = new HashMap<String, String>();
		item10.put(text1, "Local Connection");
		item10.put(text2, Boolean.toString(getLocalConnectionStatus() != SmartPlugDevice.LOCAL_CONNECTION_LIVE? true:false));
		ret.add(item10);
		
		HashMap<String, String> item11 = new HashMap<String, String>();
		item11.put(text1, "Last Metrology Time");
		item11.put(text2, new SimpleDateFormat("yyyy/MM/dd HH:mm:ss", Locale.ENGLISH).format(new Date(latestMetrologyData.dataReceivedTime)));
		ret.add(item11);
		
		HashMap<String, String> item12 = new HashMap<String, String>();
		item12.put(text1, "Metrology Database Size");
		item12.put(text2, "" + getPowerHistory(false, 0).size());
		ret.add(item12);
		
		// Cloud info from device
		if(showCloudInfo){
			byte data = latestCloudInfo.cloudStatus;
			boolean initialized				= (data & 0x01)==0x01? true:false;
			boolean cloud_connection_active	= (data & 0x02)==0x02? true:false;
			boolean cloud_connection_secure	= (data & 0x04)==0x04? true:false;
			boolean valid_information		= (data & 0x08)==0x08? true:false;
			boolean valid_CIK				= (data & 0x10)==0x10? true:false;
			
			HashMap<String, String> item13 = new HashMap<String, String>();
			item13.put("col_1", "Cloud Info: Initialized");
			item13.put("col_2", Boolean.toString(initialized));
			ret.add(item13);
			
			HashMap<String, String> item14 = new HashMap<String, String>();
			item14.put("col_1", "Cloud Info: Activated");
			item14.put("col_2", Boolean.toString(cloud_connection_active));
			ret.add(item14);
			
			HashMap<String, String> item15 = new HashMap<String, String>();
			item15.put("col_1", "Cloud Info: Secrued Cloud");
			item15.put("col_2", Boolean.toString(cloud_connection_secure));
			ret.add(item15);
			
			HashMap<String, String> item16 = new HashMap<String, String>();
			item16.put("col_1", "Cloud Info: Valid Info");
			item16.put("col_2", Boolean.toString(valid_information));
			ret.add(item16);
			
			HashMap<String, String> item17 = new HashMap<String, String>();
			item17.put("col_1", "Cloud Info: Valid CIK");
			item17.put("col_2", Boolean.toString(valid_CIK));
			ret.add(item17);
		}
		
		return ret;
	}
}

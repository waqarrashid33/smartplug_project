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

package com.ti.cc3200smartplug.service;

import java.io.IOException;
import java.io.InputStream;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.security.cert.Certificate;
import java.security.cert.CertificateFactory;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningServiceInfo;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.BitmapFactory;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.AsyncTask;
import android.os.Binder;
import android.os.IBinder;
import android.os.PowerManager;
import android.preference.PreferenceManager;
import android.support.v4.app.NotificationCompat;
import android.util.Log;
import android.widget.Toast;

import com.exosite.api.onep.OneException;
import com.exosite.api.onep.RPC;
import com.ti.cc3200smartplug.Common;
import com.ti.cc3200smartplug.DeviceDetailActivity;
import com.ti.cc3200smartplug.DeviceListActivity;
import com.ti.cc3200smartplug.GuiUpdateCallback;
import com.ti.cc3200smartplug.R;
import com.ti.cc3200smartplug.SmartPlugBaseActivity;

/**
 * This service will be running all the time with wake-lock unless user decides to kill it
 * with clicking on "Exit Application" from the activity menu.
 */
public class SmartPlugService extends Service implements
		LocalConnectionThread.LCTCallbacks,
		CloudConnectionThread.CCTCallbacks{
	
	private String DEBUG_CLASS = "SmartPlugService";
	private PowerManager.WakeLock wl;
	private GuiUpdateCallback activityCallback;
	
	//public static final String INTENT_STR = "com.ti.cc31xxsmartplug.SmartPlugService";
	
	/**
	 * An interface for cloud synchronization callback.
	 */
	public interface CloudSyncCallback{
		/**
		 * 
		 * @param success True if process finishes normally. In this case, three list will be returned.
		 * @param message additional message with regards to syncing status
		 * @param existLocal_noAssoUser
		 * @param existLocal_isNotCurAssoUser
		 * @param notExistLocal
		 */
		public void onSyncComplete(
				boolean success,
				String message,
				ArrayList<SmartPlugDevice> existLocal_noAssoUser,
				ArrayList<SmartPlugDevice> existLocal_isNotCurAssoUser);
	}
	
	// The entire collection of known devices on Android
	private SPDeviceHandler devicePoolDatabase;
	
	// The dynamic collection, always referring to "devicePoolDatabase"
	private ArrayList<SmartPlugDevice> devicePool;
	
	// Notification
	public NotificationCompat.Builder SPSNotificationBuilder;
	public Notification notification;
	
	// The receiver for WiFi status
	private BroadcastReceiver mWiFiStateReceiver;
	
	private Timer checkCloudConnectionTimer;
	private static final short CLOUD_TIMER_INTERVAL_IN_SECOND = 5;

	/* The Binder */
	private final IBinder mBinder = new LocalBinder();
	
	public class LocalBinder extends Binder{
		public SmartPlugService getService(SmartPlugBaseActivity act){
			Intent intent;
			activityCallback = act;
			
			if(act instanceof DeviceListActivity)
				intent = new Intent((DeviceListActivity)act, DeviceListActivity.class);
			else
				intent = new Intent(act, DeviceDetailActivity.class);
			
			PendingIntent pIntent = PendingIntent.getActivity(act, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_ONE_SHOT);
			SPSNotificationBuilder.setContentIntent(pIntent);
			notification = SPSNotificationBuilder.build();
			startForeground(1, notification);
			return SmartPlugService.this;
		}
	}

	//================================= Service Lifecycle =================================
	
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		Toast.makeText(this, "Smart Plug Service created ...", Toast.LENGTH_SHORT).show();
		
		activityCallback = null;
		
		PowerManager pm = (PowerManager) this.getSystemService(Context.POWER_SERVICE);
		wl = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "SmartPlugService");
		wl.acquire();
		
		devicePoolDatabase = new SPDeviceHandler(getApplicationContext());
		devicePool = devicePoolDatabase.getAllSPDevices();
		
	    // Building Notification
		SPSNotificationBuilder = new NotificationCompat.Builder(this);
		SPSNotificationBuilder.setLargeIcon(BitmapFactory.decodeResource(getResources(), R.drawable.main_icon));
		SPSNotificationBuilder.setSmallIcon(R.drawable.main_icon);
		SPSNotificationBuilder.setContentTitle("Smart Plug");
		SPSNotificationBuilder.setContentText("App still running. If you want to exit the app, please click \"Exit Application\" from the menu.");
		SPSNotificationBuilder.setAutoCancel(true);
		
		mWiFiStateReceiver = new BroadcastReceiver(){
			@Override
			public void onReceive(Context context, Intent intent) {
				Log.w(DEBUG_CLASS, "WiFi Status changed");
				statusUpdateWiFi();
			}
		};
		
		registerReceiver(mWiFiStateReceiver, new IntentFilter("android.net.conn.CONNECTIVITY_CHANGE"));
		
		/* Notification sound */
//		Uri alarmSound = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION);
//		SPSNotificationBuilder.setSound(alarmSound);
	    return START_NOT_STICKY;
	}
	
	private void statusUpdateWiFi(){
		ConnectivityManager connManager = (ConnectivityManager) getSystemService(android.content.Context.CONNECTIVITY_SERVICE);
//		WifiManager wifiManager = (WifiManager) getSystemService(WIFI_SERVICE);
		
		boolean WiFi = connManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI).isConnected();
		boolean Cellular = false;
		NetworkInfo infoCellular = connManager.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
		if(infoCellular != null){
			Cellular = infoCellular.isConnected();
		}
		
		//Note: this broadcast receiver update only stops threads with corresponding connection type, and send an update message to notify user.
		
		// Check WiFi Connection
		if(!WiFi){
			// Stop all local connection
			for(SmartPlugDevice device : devicePool){
				device.stopLocalThread();
			}
		}
		
		// Check Cellular Connection
		if(Cellular){
			//Nothing
		} else {
			if(!WiFi){
				// Both cellular and wifi are down. Stop cloud connection
				for(SmartPlugDevice device : devicePool){
					device.stopCloudThread();
				}
			}
		}
	}
	
	@Override
	public IBinder onBind(Intent arg0) {
		return mBinder;
	}
	
	@Override
	public boolean onUnbind(Intent arg0){
		activityCallback = null;
		return super.onUnbind(arg0);
	}
	
	@Override
	public void onDestroy() {
		wl.release();
		
		Toast.makeText(this, "Smart Plug Service destroyed ...", Toast.LENGTH_SHORT).show();
		
		stopForeground(true);
		
		unregisterReceiver(mWiFiStateReceiver);
		
		Log.d(DEBUG_CLASS, "Destroyed");
		super.onDestroy();
	}
	
	/**
	 * Checks if this service is running.
	 * @param act - Service name will be checked based on the activity name.
	 * @return true if running
	 */
	public static boolean isMyServiceRunning(Activity act) {
	    ActivityManager manager = (ActivityManager) act.getSystemService(Context.ACTIVITY_SERVICE);
	    for (RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
	        if (SmartPlugService.class.getName().equals(service.service.getClassName())) {
	            return true;
	        }
	    }
	    return false;
	}
	
	//================================= Thread Execution =================================
	
	/**
	 * Starts Local Connection.
	 * @param d The SmartPlugDevice
	 * @return True if thread started successfully
	 */
	public boolean executeLocalConnection(SmartPlugDevice d){
		
		boolean ret = true;
		
		// Load CAs from an InputStream (could be from a resource or ByteArrayInputStream or ...)
		InputStream caInput = null;
		try{
			CertificateFactory cf = CertificateFactory.getInstance("X.509");
			caInput = getResources().openRawResource(R.raw.serverca);
			Certificate cert = cf.generateCertificate(caInput);
			
			//Starts thread
			d.startLocalThread(this, cert);
			
			// Sets timer to routinely check cloud connection
			checkCloudConnectionTimer = new Timer();
			checkCloudConnectionTimer.schedule(new TimerTask(){
				@Override
				public void run() {
					//   
				}
			}, CLOUD_TIMER_INTERVAL_IN_SECOND * 1000);
			
		}catch (Exception e){
			ret = false;
			Toast.makeText(this, "ERROR: Certification may have been expired", Toast.LENGTH_SHORT).show();
		}finally {
			try {caInput.close();}
			catch (IOException e) {}
		}
		
		return ret;
	}
	
	/**
	 * Starts Cloud Connection.
	 * @param d The SmartPlugDevice
	 * @return True if thread started successfully
	 */
	public boolean executeCloudConnection(SmartPlugDevice d){
		d.startCloudThread(this, true);
		return true;
	}
	
	@Override
	public void LCTProgressUpdate(final short tcpThreadCode, SmartPlugDevice device){
		if(activityCallback!=null)
			activityCallback.LocalConnectionUpdate(tcpThreadCode, device);
	}
	
	@Override
	public void LCTCompletionUpdate(Short end_thread_status, SmartPlugDevice device) {
		
		if(activityCallback!=null)
			activityCallback.LocalConnectionUpdate(end_thread_status, device);
		
		/* Notify MainActivity that thread finished */
		SPSNotificationBuilder.setContentText(device.toString() + " disconnected.");
		startForeground(1, SPSNotificationBuilder.build());
	}
	
	@Override
	public void CCTProgressUpdate(final short updateCode, SmartPlugDevice device, final String msg){
		if(activityCallback!=null)
			activityCallback.CloudConnectionUpdate(updateCode, device, msg);
	}
	
	//================================= Device Pool Operation =================================
		
	/**
	 * Add a device to the pool.
	 * This method will also perform checking before adding, making sure there's no duplicated item.
	 * @return true if successful. List adapter should refresh after this call.
	 */
	public final boolean poolAddDevice(SmartPlugDevice newDevice){
		
		// A matching device means it's already in the list:
		// 1. If there are any information mismatch in this device, update the information
		// 2. If not, do nothing
		SmartPlugDevice localDevice = findDeviceByMac(newDevice.getMAC());
		if(localDevice!=null){
			checkAndUpdateDeviceInfo(localDevice, newDevice);
			return false;
		}
		
		devicePoolDatabase.addSPDevice(newDevice);
		devicePool.add(newDevice);
		
		return true;
	}
	
	/**
	 * Check information change in the newly fetched device. If there are changes, apply the change and return true.
	 * @param localDevice
	 * @param newDevice
	 * @return True if information updated.
	 */
	public boolean checkAndUpdateDeviceInfo(SmartPlugDevice localDevice, SmartPlugDevice newDevice){
		boolean ret = false;
		
		String newName = newDevice.getName();
		String newRID = newDevice.getRID();
		String newCIK = newDevice.getCIK();
		
		// Update Name
		if(localDevice.getName() == null){
			ret = true;
			localDevice.setName(newName);
			
		}else if(!localDevice.getName().equals(newName)){
			ret = true;
			localDevice.setName(newName);
		}
		
		// Update RID
		if(localDevice.getRID() == null){
			ret = true;
			localDevice.setName(newRID);
			
		}else if(!localDevice.getRID().equals(newRID)){
			ret = true;
			localDevice.setRID(newRID);
		}
			
		// Update CIK
		if(localDevice.getCIK() == null){
			ret = true;
			localDevice.setCIK(newCIK);
			
		}else if(!localDevice.getCIK().equals(newCIK)){
			ret = true;
			localDevice.setCIK(newCIK);
		}
		
		return ret;
	}
	
	/**
	 * Get the device based on MAC address
	 * @param mac
	 * @return The SPDevice. null if no matching item.
	 */
	public SmartPlugDevice poolGetDevice(String mac){
		for(int i=0; i<devicePool.size(); i++){
			if(devicePool.get(i).getMAC().equals(mac)){
				return devicePool.get(i);
			}
		}
		
		return null;
	}
	
	/**
	 * Get the device based on requested index
	 * @param index
	 * @return The SPDevice. null if no matching item.
	 */
	public SmartPlugDevice poolGetDevice(int index){
		return devicePool.get(index);
	}
	
	/**
	 * Get the number of devices.
	 * @return number, in integer
	 */
	public int poolGetCount(){
		return devicePool.size();
	}
	
	/**
	 * Get the index of the device in the device pool.
	 * @param device The SmartPlugDevice to search
	 * @return Index in integer. -1 if not found.
	 */
	public int poolIndexOf(SmartPlugDevice device){
		return devicePool.indexOf(device);
	}
	
	/**
	 * Checks if the Pool contains the device with a specific MAC address.
	 * @param mac MAC address to be searched.
	 * @return the SmartPlugDevice if exists. Null otherwise
	 */
	public SmartPlugDevice poolContains(String mac){
		
		String capitalizedMac = mac.toUpperCase(Locale.ENGLISH);
		
		for(SmartPlugDevice d : devicePool){
			if(d.getMAC().equals(capitalizedMac)){
				return d;
			}
		}
		
		return null;
	}
	
	/**
	 * Deletes the device from local collection.
	 * @param device
	 * @return true if successful.
	 */
	public boolean poolDelete(SmartPlugDevice device){
		device.deleteMetrologyDatabase();
		devicePoolDatabase.deleteSPDevice(device);
		return devicePool.remove(device);
	}
	
	/**
	 * Nothing other than ListAdapter should call this.
	 * @return
	 */
	public ArrayList<SmartPlugDevice> getPoolForListAdapter(){
		return devicePool;
	};
	
	/**
	 * Find the SPDevice inside our devicePool. MAC format can be mixed with upper or lower case.
	 * However, no ":" symbol allowed
	 * @param mac - The MAC string to search for. 
	 * @return the SPDevice if found. Null otherwise.
	 */
	public SmartPlugDevice findDeviceByMac(String mac){
		String upperMAC = mac.toUpperCase(Locale.ENGLISH);
		
		for(SmartPlugDevice spd: devicePool){
			if(spd.getMAC().equals(upperMAC))
				return spd;
		}
		
		return null;
	}

	//================================= Cloud Synchronization =================================
	
	/**
	 * Synchronize devices
	 */
	public void syncCloudDevices(final CloudSyncCallback callback){	
		Common.checkCredential(this, new Common.CheckCredentialCallBack(){
			@Override
			public void onReturn(boolean success, String message) {
				if(success){
					new LoadDevicesTask(callback).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
					return;
				}
				// Sync failed
				callback.onSyncComplete(false, message, null, null);
			}
		});
	}
	
	/**
	 * Synchronize device data
	 */
	public void syncCloudData(final CloudSyncCallback callback){
		Common.checkCredential(this, new Common.CheckCredentialCallBack(){
			@Override
			public void onReturn(boolean success, String message) {
				if(success){
					//TODO - [Consider Delete] because the CCT has taken care of getting historical data in the graphing interval
					return;
				}
				callback.onSyncComplete(false, message, null, null);
			}
		});
	}
	
	/**
	 * Represents a task that loads information about devices from OneP on a background thread.
	 */
    private class LoadDevicesTask extends AsyncTask<Void, Integer, ArrayList<SmartPlugDevice>> {
        private static final String TAG = "LoadDevicesTask";
        private Exception exception;
        
        private CloudSyncCallback callback;

        public LoadDevicesTask(CloudSyncCallback callback) {
			this.callback = callback;
		}

        @Override
		protected ArrayList<SmartPlugDevice> doInBackground(Void... params) {
            //A list of devices to return for later
            ArrayList<SmartPlugDevice> tempDevices = new ArrayList<SmartPlugDevice>();
            
            RPC rpc = new RPC();
            exception = null;
            
            try {
            	//Get the portal list
                SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(SmartPlugService.this);
                JSONArray mPortalList = new JSONArray(sharedPreferences.getString("portal_list", "[]"));
                
                JSONObject infoOptions = new JSONObject();
                infoOptions.put("description", true);
                infoOptions.put("key", true);
                
                for (int i = 0; i < mPortalList.length(); i++) {		//Looping through all portals
                    JSONObject portal = (JSONObject)mPortalList.get(i); //Get a Portal
                    String pcik = portal.getString("key");				//Portal CIK
                    JSONArray types = new JSONArray();
                    types.put("client");
                    
                    JSONObject infoListing = rpc.infoListing(pcik, types, infoOptions);		
                    JSONObject clientsInfoListing = infoListing.getJSONObject("client");	//Get list of devices in a portal

                    Iterator<?> iter = clientsInfoListing.keys();
                    while (iter.hasNext()) {												//Loop through list of devices
                        String rid = (String) iter.next();									//Get RID of a device
                        JSONObject info = clientsInfoListing.getJSONObject(rid);
                        
                        JSONObject meta = new JSONObject(info.getJSONObject("description").getString("meta"));
                        String cik = info.getString("key");
                        
                        String tmpModel = meta.getJSONObject("device").getString("model");
                        String tmpVendor = meta.getJSONObject("device").getString("vendor");
                        
                        // Only add if model and vendor is matching
                        if(tmpModel.equals(SmartPlugDevice.DEFAULT_MODEL) && tmpVendor.equals(SmartPlugDevice.DEFAULT_VENDOR)){
                        	tempDevices.add(new SmartPlugDevice(
                            		getApplicationContext(),
                            		rid,
                            		info.getJSONObject("description").getString("name"),
                            		meta.getJSONObject("device").getString("sn").toUpperCase(Locale.ENGLISH),
                            		cik,
                            		meta.getJSONObject("device").getString("model"),
                            		meta.getJSONObject("device").getString("vendor"),
                            		null,
                    				SmartPlugDevice.CONNECTION_PREF_AUTO,
                    				PreferenceManager.getDefaultSharedPreferences(getApplicationContext()).getString(Common.PREFERENCE_EMAIL, null)));
                        }
                    }
                }
                return tempDevices;

            } catch (JSONException e) {
                exception = e;
                Log.e(TAG, "JSONException in ReadPortals.doInBackground: " + e.toString());
            } catch (OneException e) {
                exception = e;
                Log.e(TAG, "OneException: " + e.toString());
            }
            return null;
        }

		@Override
        protected void onPostExecute(ArrayList<SmartPlugDevice> tempDevices) {
        	 if (exception == null) {
        		 
        		 ArrayList<SmartPlugDevice> existLocal_noAssoUser = new ArrayList<SmartPlugDevice>();
        		 ArrayList<SmartPlugDevice> existLocal_isNotCurAssoUser = new ArrayList<SmartPlugDevice>();
        		 ArrayList<SmartPlugDevice> notExistLocal = new ArrayList<SmartPlugDevice>();
        		 
        		 //We have a list of temporary models, now check with local devices and see if there's any matching ones.
        		 for(SmartPlugDevice tempD : tempDevices){
        			 boolean deviceInPool = false;
        			 
        			 // Check if device is in pool or not
        			 for(SmartPlugDevice poolD : devicePool){
        				 
        				 if(tempD.getMAC().equals(poolD.getMAC())){
        					//Device is found in pool, now check associated user
        					 deviceInPool = true;
        					 
        					 if(poolD.getAssociatedUser() == null){
        						 existLocal_noAssoUser.add(tempD);
        						 
        					 }else if(poolD.getAssociatedUser().equals(tempD.getAssociatedUser())){
        						 //Nothing, device is in list
        						 
        					 }else{
        						 //This device belongs to another user, warn user
        						 existLocal_isNotCurAssoUser.add(tempD);
        					 }
            			 }
        			 }
        			 
        			//Device is not found in pool
        			 if(!deviceInPool){
        				 notExistLocal.add(tempD);
        			 }
        		 }
        		 
        		 // Device is not found in pool, add to list directly
        		 for(SmartPlugDevice d : notExistLocal){
        			 poolAddDevice(d);
    			 }
        		 
        		 callback.onSyncComplete(true, "", existLocal_noAssoUser, existLocal_isNotCurAssoUser);
        		 
        	 }else{
        		 callback.onSyncComplete(false, "ERROR when syncing with cloud: " + exception.getMessage(), null, null);
        	 }
        }
    }
	
  //================================= Update Device Information =================================
    
    public String updateLocalName(SmartPlugDevice device, String localName) {
    	device.setName(localName);
    	devicePoolDatabase.updateKey(device.getMAC(), SPDeviceHandler.KEY_NAME, localName);
		return null;
	}
    
    public String updateLocalAddress(SmartPlugDevice device, String localAddressString) {
    	if(localAddressString == null){
    		device.setLocalAddress(null);
    	}else{
    		try {
				device.setLocalAddress(InetAddress.getByName(localAddressString));
			} catch (UnknownHostException e) {
				Log.e(DEBUG_CLASS, e.getMessage());
				return e.getMessage();
			}
    	}
    	devicePoolDatabase.updateKey(device.getMAC(), SPDeviceHandler.KEY_LOCAL_ADDR, localAddressString);
		return null;
	}
    
	public String updateLocalAddress(SmartPlugDevice device, InetAddress address) {
		device.setLocalAddress(address);
    	devicePoolDatabase.updateKey(device.getMAC(), SPDeviceHandler.KEY_LOCAL_ADDR, address.getHostAddress());
		return null;
	}
    
	public String updateLocalRID(SmartPlugDevice device, String rid) {
		device.setRID(rid);
    	devicePoolDatabase.updateKey(device.getMAC(), SPDeviceHandler.KEY_RID, rid);
		return null;
	}
	
	public String updateLocalCIK(SmartPlugDevice device, String cik) {
		device.setCIK(cik);
    	devicePoolDatabase.updateKey(device.getMAC(), SPDeviceHandler.KEY_CIK, cik);
		return null;
	}
	
    public void updateConnectionPreference(SmartPlugDevice device, short connPref) {
    	device.setConnectionPreference(connPref);
    	devicePoolDatabase.updateKey(device.getMAC(), SPDeviceHandler.KEY_CONN_PREF, connPref);
    }

}

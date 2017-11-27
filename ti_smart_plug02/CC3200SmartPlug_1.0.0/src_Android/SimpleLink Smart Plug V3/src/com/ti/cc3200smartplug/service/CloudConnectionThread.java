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

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;

import org.json.JSONException;
import org.json.JSONObject;

import android.os.AsyncTask;
import android.util.Log;

import com.exosite.api.onep.OneException;
import com.exosite.api.onep.RPC;
import com.exosite.api.onep.TimeSeriesPoint;
import com.ti.cc3200smartplug.DeviceDetailFragment;
import com.ti.cc3200smartplug.data.*;

public class CloudConnectionThread extends AsyncTask<Boolean, Object, Object>{
	
	public interface CCTCallbacks{
		
		/**
		 * CCT progress update callback.
		 * @param updateCode CLOUD_UPDATE_NORMAL, CLOUD_UPDATE_ERROR, CLOUD_END_NORMAL, or CLOUD_END_ERROR
		 * @param device The Device associated with the update
		 * @param msg Message with this update
		 * @return
		 */
		public void CCTProgressUpdate(final short updateCode, SmartPlugDevice device, final String msg);
	}
	
	/* References to SmartPlugService & DeviceDetailFragment */
	private CCTCallbacks	ThreadCaller;
	private SmartPlugDevice	thisDevice;
	private String			DEBUG_CLASS;
	
	private List<String>	aliasesToReadMetrology;
	private List<String>	aliasesToRead;
	
	private String			errorMessage;
	
	/** The flag to keep this thread running. Once it's set to false, thread will exit. */
	public volatile boolean flag_ExositeRunning;
	
	public static final short CLOUD_UPDATE_3_3_1 = 1;
	public static final short CLOUD_UPDATE_3_3_2 = 2;
	public static final short CLOUD_UPDATE_3_3_3 = 3;
	public static final short CLOUD_UPDATE_3_3_4 = 4;
	public static final short CLOUD_UPDATE_3_3_5 = 5;
	public static final short CLOUD_UPDATE_3_3_6 = 6;
	public static final short CLOUD_UPDATE_3_3_7 = 7;
	public static final short CLOUD_UPDATE_ERROR = -1;
	
	public static final short CLOUD_CONNECTION_SUCCESS = 100;
	
	public static final short CLOUD_END_NORMAL							= 200;
	public static final short CLOUD_END_NORMAL_BECAUSE_SP_NOT_ACTIVE	= 201;
	public static final short CLOUD_END_ERROR							= 202;
	
	public static final short CLOUD_DATA_TIMEOUT = 60;
	
	// TX queue
	public volatile LinkedList<TXStrings> TxBuffer;
	private class TXStrings{
		public String alias;
		public String data;
		
		public TXStrings(String alias, String data){
			this.alias = alias;
			this.data = data;
		}
	}
	
	public CloudConnectionThread(CCTCallbacks ThreadCaller, SmartPlugDevice device){
		this.ThreadCaller = ThreadCaller;
		thisDevice = device;
		
		flag_ExositeRunning = true;
		
		errorMessage = null;
		
		TxBuffer = new LinkedList<TXStrings>();
		
		DEBUG_CLASS = thisDevice.getMAC() + " ExositeThread";
		
		// Creates a list of aliases to read
		aliasesToReadMetrology = new ArrayList<String>();
		aliasesToReadMetrology.add(SmartPlugDevice.ALIAS_POWER);
		aliasesToReadMetrology.add(SmartPlugDevice.ALIAS_VOLTAGE);
		aliasesToReadMetrology.add(SmartPlugDevice.ALIAS_CURRENT);
		aliasesToReadMetrology.add(SmartPlugDevice.ALIAS_FREQUENCY);
		aliasesToReadMetrology.add(SmartPlugDevice.ALIAS_VAR);
		aliasesToReadMetrology.add(SmartPlugDevice.ALIAS_PF);
		aliasesToReadMetrology.add(SmartPlugDevice.ALIAS_VA);
		aliasesToReadMetrology.add(SmartPlugDevice.ALIAS_KWH);
		aliasesToReadMetrology.add(SmartPlugDevice.ALIAS_AVG_POWER);
        
        aliasesToRead = new ArrayList<String>();
        aliasesToRead.addAll(aliasesToReadMetrology);
        aliasesToRead.add(SmartPlugDevice.ALIAS_HOURLY_E_AVG);
        aliasesToRead.add(SmartPlugDevice.ALIAS_RELAY);
        aliasesToRead.add(SmartPlugDevice.ALIAS_POWER_SAVING);
        aliasesToRead.add(SmartPlugDevice.ALIAS_INTERVAL);
        aliasesToRead.add(SmartPlugDevice.ALIAS_E_THRESHOLD);
        aliasesToRead.add(SmartPlugDevice.ALIAS_P_THRESHOLD); 
        aliasesToRead.add(SmartPlugDevice.ALIAS_WARNING_OVER_E);
        aliasesToRead.add(SmartPlugDevice.ALIAS_WARNING_OVER_P);
        aliasesToRead.add(SmartPlugDevice.ALIAS_SCHEDULE_TABLE);
	}
	
	@Override
	protected Object doInBackground(Boolean... params) {
		/* 
		 * The boolean flag is set to true initially to get data from before the connection time,
		 * in order to plot the graph.
		 */
		boolean getHistoryFromCloud = params[0];
		
		// Return value
		short backgroundRet = CLOUD_END_NORMAL;
		
		long time1;
		long time2;
		boolean connection_success_updated = false;
		
		RPC rpc = new RPC();
		
		// Collect history because the mobile device was disconnected from the cloud a while ago.
		// Now fetch the data with the time range [now - DeviceDetailFragment.DATA_RANGE, now].
		if(getHistoryFromCloud){
			try {
				JSONObject options = new JSONObject();
				int currentTimeInSecond = (int)(System.currentTimeMillis()/1000);
				options.put("starttime", currentTimeInSecond - (DeviceDetailFragment.DATA_RANGE/1000));
				options.put("endtime", currentTimeInSecond);
				options.put("limit", DeviceDetailFragment.DATA_RANGE/1000);
				options.put("selection", "all");
				
				//HashMap<String, List<TimeSeriesPoint>> results = rpc.read(thisDevice.getCIK(), aliasesToReadMetrology, options);
				List<String> temp = new ArrayList<String>();
				temp.add(SmartPlugDevice.ALIAS_POWER);
				HashMap<String, List<TimeSeriesPoint>> results = rpc.read(thisDevice.getCIK(), temp, options);
				
				if(connection_success_updated==false){
					publishProgress(CLOUD_CONNECTION_SUCCESS);
					connection_success_updated = true;
				}
				
				//Print all power history
				List<TimeSeriesPoint> powerHistory = results.get(SmartPlugDevice.ALIAS_POWER);
				int size = powerHistory.size();
//				for(int i=0; i<size; i++){
//					Log.e(DEBUG_CLASS, SmartPlugDevice.ALIAS_POWER + "=" + powerHistory.get(i).toString());
//				}
				
				// if no data within the give range, it means the SP is not currently active. Notify user and end thread.
				if(size == 0){
					backgroundRet = CLOUD_END_NORMAL_BECAUSE_SP_NOT_ACTIVE;
					flag_ExositeRunning = false;
				}
				
			} catch (OneException e) {
				// No result, stop connection
				flag_ExositeRunning = false;
				backgroundRet = CLOUD_END_ERROR;
				errorMessage = e.getMessage();
	            Log.e("EXOSITE", "Exception " + errorMessage);
			} catch (JSONException e) {
				e.printStackTrace();
				Log.e("EXOSITE", "JSON put exception");
			}
		}
		
		while(flag_ExositeRunning){
			try {
				
				// Send Data
				if(TxBuffer.size() != 0){
					TXStrings dataToSend = TxBuffer.getFirst();
					rpc.write(thisDevice.getCIK(), dataToSend.alias, dataToSend.data);
					TxBuffer.removeFirst();
				}
				
				time1 = System.currentTimeMillis();
				HashMap<String, TimeSeriesPoint> result = rpc.readLatest(thisDevice.getCIK(), aliasesToRead);
				
				//temporarily store data
				TimeSeriesPoint power		= result.get(SmartPlugDevice.ALIAS_POWER);			// 0: activePower
				TimeSeriesPoint voltage		= result.get(SmartPlugDevice.ALIAS_VOLTAGE);		// 1: voltage
				TimeSeriesPoint current		= result.get(SmartPlugDevice.ALIAS_CURRENT);		// 2: current
				TimeSeriesPoint freq		= result.get(SmartPlugDevice.ALIAS_FREQUENCY);		// 3: frequency
				TimeSeriesPoint var			= result.get(SmartPlugDevice.ALIAS_VAR);			// 4: var
				TimeSeriesPoint pf			= result.get(SmartPlugDevice.ALIAS_PF);				// 5: cos
				TimeSeriesPoint va			= result.get(SmartPlugDevice.ALIAS_VA);				// 6: va
				TimeSeriesPoint kwh			= result.get(SmartPlugDevice.ALIAS_KWH);			// 7: kWh
				TimeSeriesPoint avg_power	= result.get(SmartPlugDevice.ALIAS_AVG_POWER);		// 8: avgPower
				
				TimeSeriesPoint h_e_avg		= result.get(SmartPlugDevice.ALIAS_HOURLY_E_AVG);	// 9: hourlyAvgPower
				
				//Settings related fields
				TimeSeriesPoint relay		= result.get(SmartPlugDevice.ALIAS_RELAY);			//10: relay
				TimeSeriesPoint power_saving= result.get(SmartPlugDevice.ALIAS_POWER_SAVING);	//11: powerSaving
				TimeSeriesPoint interval	= result.get(SmartPlugDevice.ALIAS_INTERVAL);		//12: reportInterval
				TimeSeriesPoint e_threshold = result.get(SmartPlugDevice.ALIAS_E_THRESHOLD);	//13: eThreshold
				TimeSeriesPoint p_threshold = result.get(SmartPlugDevice.ALIAS_P_THRESHOLD);	//14: pThreshold
				
				//Over power/energy
				TimeSeriesPoint e_warning	= result.get(SmartPlugDevice.ALIAS_WARNING_OVER_E);	//15: overEMsg
				TimeSeriesPoint p_warning	= result.get(SmartPlugDevice.ALIAS_WARNING_OVER_P);	//16: overPMsg
				
				//Schedule table, comma seperated
				TimeSeriesPoint schedule	= result.get(SmartPlugDevice.ALIAS_SCHEDULE_TABLE);	//17: schedule
				
				// Check time stamp
				// If it's the same as before, it means there's no update and device is not active
				// This variable is being reused for all packets
				int pendingNewTime;
				
				if(power != null){
					
					pendingNewTime = power.getTimeStamp();
					int timeSinceLastData = pendingNewTime - thisDevice.getLatestData().getLastDataTimeInSecond();
					
					// Packet 3.3.1 - Packet to Update Metrology Data
					if(timeSinceLastData > 0){ //New data, record data and update GUI
						
						// If the CLOUD_CONNECTION_SUCCESS message hasn't been updated, perform update
						if(connection_success_updated==false){
							
							// If time elapsed since last data acquisition is more than CLOUD_DATA_TIMEOUT,  Notify user and end thread.
							if(timeSinceLastData > CLOUD_DATA_TIMEOUT){
								backgroundRet = CLOUD_END_NORMAL_BECAUSE_SP_NOT_ACTIVE;
								flag_ExositeRunning = false;
							}else{
								publishProgress(CLOUD_CONNECTION_SUCCESS);
							}
							
							connection_success_updated = true;
						}
						
//						Log.d(DEBUG_CLASS, "New Metrology Data");
						thisDevice.addData(
								new DToA_MetrologyData(
										((long) pendingNewTime)*1000,
										Float.parseFloat(power		.getValue().toString()),
										Float.parseFloat(voltage	.getValue().toString()),
										Float.parseFloat(current	.getValue().toString()),
										Float.parseFloat(freq		.getValue().toString()),
										Float.parseFloat(var		.getValue().toString()),
										Float.parseFloat(pf			.getValue().toString()),
										Float.parseFloat(va			.getValue().toString()),
										Float.parseFloat(kwh		.getValue().toString()),
										Float.parseFloat(avg_power	.getValue().toString()),
										0));
						
						backgroundRet = CLOUD_END_NORMAL;
						
						publishProgress(CLOUD_UPDATE_3_3_1);
						
					}else{	//No new data, check the time and compare with device update interval
							//If exceeding the device update interval, it means the device might not be active anymore
						
						time2 = System.currentTimeMillis();
						long timeSinceLastValidValue = time2 - time1;
						
						int tempUpdateInterval = 1;
						if(thisDevice.latestDeviceStatus.powerSavingEnable == true)
							tempUpdateInterval = thisDevice.latestDeviceStatus.updateInterval;
						
						if(timeSinceLastValidValue > (((long) tempUpdateInterval)*1000 + 1000)){ //extra 1 second to be safe
							//No new data after the specified interval time, warn user through GUI
							publishProgress(CLOUD_UPDATE_ERROR, "Cloud metrology data reading timeout");
							flag_ExositeRunning = false;
						}
					}
				}
				
				
				//For the rest of the updates, only check if there's update.
				//We don't care about intervals in these updates.
				
				// Packet 3.3.2 - Packet to update average energy at hourly frequency
				if(h_e_avg != null){
					pendingNewTime = h_e_avg.getTimeStamp();
					if(pendingNewTime > thisDevice.latestAvgEnergyHourly.getLastDataTimeInSecond()){
						Log.d(DEBUG_CLASS, "New Hourly Data");
						thisDevice.latestAvgEnergyHourly = new DToA_AvgEnergyHourly(
								((long)pendingNewTime)*1000,
								Float.parseFloat(h_e_avg.getValue().toString()),
								0);
						publishProgress(CLOUD_UPDATE_3_3_2);
					}
				}
				
				// Packet 3.3.3 - Packet to update device status
				if(relay != null){
					ArrayList<Integer> listNewTime = new ArrayList<Integer>();
					listNewTime.add(relay.getTimeStamp());
					listNewTime.add(power_saving.getTimeStamp());
					listNewTime.add(interval.getTimeStamp());
					listNewTime.add(e_threshold.getTimeStamp());
					listNewTime.add(p_threshold.getTimeStamp());
					pendingNewTime = Collections.max(listNewTime);
					
					if(pendingNewTime > thisDevice.latestDeviceStatus.getLastDataTimeInSecond()){
						Log.d(DEBUG_CLASS, "New Device Status Data, Old Time=" + Integer.toString(thisDevice.latestDeviceStatus.getLastDataTimeInSecond()) + ", New Time=" + pendingNewTime);
						thisDevice.latestDeviceStatus = new DToA_DeviceStatus(
								((long)pendingNewTime)*1000,
								relay.getValue().toString().equals("0") ? false:true,
								power_saving.getValue().toString().equals("0") ? false:true,
								Byte.valueOf(interval.getValue().toString()),
								Float.parseFloat(e_threshold.getValue().toString()),
								Float.parseFloat(p_threshold.getValue().toString()),
								0);
						publishProgress(CLOUD_UPDATE_3_3_3);
					}
				}
				
				// Packet 3.3.4 - Warning message for over energy consumption threshold
				if(e_warning != null){
					pendingNewTime = e_warning.getTimeStamp();
					if(pendingNewTime > thisDevice.latestWarningOverEnergy.getLastDataTimeInSecond()){
						Log.d(DEBUG_CLASS, "New Warning Message for over Energy");
						thisDevice.latestWarningOverEnergy = new DToA_Warning(
								((long)pendingNewTime)*1000,
								e_warning.getValue().toString().equals("") ? false:true,
								0);
						publishProgress(CLOUD_UPDATE_3_3_4);
					}
				}
				
				// Packet 3.3.5 - Warning message for over power consumption threshold
				if(p_warning != null){
					pendingNewTime = p_warning.getTimeStamp();
					if(pendingNewTime > thisDevice.latestWarningOverPower.getLastDataTimeInSecond()){
						Log.d(DEBUG_CLASS, "New Warning Message for over Power");
						thisDevice.latestWarningOverPower = new DToA_Warning(
								((long)pendingNewTime)*1000,
								p_warning.getValue().toString().equals("") ? false:true,
								0);
						publishProgress(CLOUD_UPDATE_3_3_5);
					}
				}
				
				// Packet 3.3.6 - Device's default switch table.
				if(schedule != null){
					pendingNewTime = schedule.getTimeStamp();
					if(pendingNewTime > thisDevice.latestSwitchTable.getLastDataTimeInSecond()){
						Log.d(DEBUG_CLASS, "New Scheule");
						String[] scheduleInStrings = schedule.getValue().toString().split("[,]");
						
						thisDevice.latestSwitchTable = new DToA_SwitchTable(
								((long)pendingNewTime)*1000,
								new byte[]{
										Byte.parseByte(scheduleInStrings[0].substring(0, 2)),	//Weekday.WakeUp.Hour
										Byte.parseByte(scheduleInStrings[0].substring(2, 4)),	//Weekday.WakeUp.Minute
										Byte.parseByte(scheduleInStrings[1].substring(0, 2)),	//Weekday.Leave.Hour
										Byte.parseByte(scheduleInStrings[1].substring(2, 4)),	//Weekday.Leave.Minute
										Byte.parseByte(scheduleInStrings[2].substring(0, 2)),	//Weekday.Return.Hour
										Byte.parseByte(scheduleInStrings[2].substring(2, 4)),	//Weekday.Return.Minute
										Byte.parseByte(scheduleInStrings[3].substring(0, 2)),	//Weekday.Sleep.Hour
										Byte.parseByte(scheduleInStrings[3].substring(2, 4))},	//Weekday.Sleep.Minute
								new byte[]{
										Byte.parseByte(scheduleInStrings[4].substring(0, 2)),	//Saturday.WakeUp.Hour
										Byte.parseByte(scheduleInStrings[4].substring(2, 4)),	//Saturday.WakeUp.Minute
										Byte.parseByte(scheduleInStrings[5].substring(0, 2)),	//Saturday.Leave.Hour
										Byte.parseByte(scheduleInStrings[5].substring(2, 4)),	//Saturday.Leave.Minute
										Byte.parseByte(scheduleInStrings[6].substring(0, 2)),	//Saturday.Return.Hour
										Byte.parseByte(scheduleInStrings[6].substring(2, 4)),	//Saturday.Return.Minute
										Byte.parseByte(scheduleInStrings[7].substring(0, 2)),	//Saturday.Sleep.Hour
										Byte.parseByte(scheduleInStrings[7].substring(2, 4))},	//Saturday.Sleep.Minute
								new byte[]{
										Byte.parseByte(scheduleInStrings[8].substring(0, 2)),	//Sunday.WakeUp.Hour
										Byte.parseByte(scheduleInStrings[8].substring(2, 4)),	//Sunday.WakeUp.Minute
										Byte.parseByte(scheduleInStrings[9].substring(0, 2)),	//Sunday.Leave.Hour
										Byte.parseByte(scheduleInStrings[9].substring(2, 4)),	//Sunday.Leave.Minute
										Byte.parseByte(scheduleInStrings[10].substring(0, 2)),	//Sunday.Return.Hour
										Byte.parseByte(scheduleInStrings[10].substring(2, 4)),	//Sunday.Return.Minute
										Byte.parseByte(scheduleInStrings[11].substring(0, 2)),	//Sunday.Sleep.Hour
										Byte.parseByte(scheduleInStrings[11].substring(2, 4))},	//Sunday.Sleep.Minute
								new byte[]{
										Byte.parseByte(scheduleInStrings[12].substring(0, 2)),	//LocalTime.Day
										Byte.parseByte(scheduleInStrings[12].substring(2, 4)),	//LocalTime.Hour
										Byte.parseByte(scheduleInStrings[12].substring(4, 6))},	//LocalTime.Minute
								scheduleInStrings[13].equals("01")? true:false
								);
						publishProgress(CLOUD_UPDATE_3_3_6);
					}
				}
				
				try{ Thread.sleep(500);} //Take some break before the next reading~
				catch(Exception e){};
				
			} catch (OneException e1) {
				
				// No result, stop connection
				flag_ExositeRunning = false;
				//publishProgress(CLOUD_UPDATE_ERROR);
				backgroundRet = CLOUD_END_ERROR;
				errorMessage = e1.getMessage();
	            Log.e("EXOSITE", "Exception " + errorMessage);
			}
		}
		
		Log.w(DEBUG_CLASS, "Cloud Thread Complete.");
		
		return backgroundRet;
	}
	
	@Override
	protected void onProgressUpdate(Object... progress){
		/* Parameters:
		 * progress[0] = update type
		 * progress[1] = additional message, depending on the update type
		 */
		
		String msg = "";
		
		if(progress.length > 1){
			msg = (String) progress[1];
		}
		
		//This part will be processed in the main service thread, and then we use
		//broadcast to send messages to the the DDF in MainActivity
		// Starts a chain of updating the GUI
		ThreadCaller.CCTProgressUpdate((Short) progress[0], thisDevice, msg);
	}
	
	public void sendData(String alias, String data) {
		TxBuffer.add(new TXStrings(alias, data));
	}
	
	@Override
	public void onPostExecute(Object arg){
		ThreadCaller.CCTProgressUpdate((Short) arg, thisDevice, errorMessage);
	}
}

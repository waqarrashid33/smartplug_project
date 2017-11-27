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

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.security.KeyStore;
import java.security.cert.Certificate;
import java.util.LinkedList;

import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManagerFactory;

import android.os.AsyncTask;
import android.util.Log;

import com.ti.cc3200smartplug.Common;
import com.ti.cc3200smartplug.data.DToA_AvgEnergyDaily;
import com.ti.cc3200smartplug.data.DToA_AvgEnergyHourly;
import com.ti.cc3200smartplug.data.DToA_CloudInfo;
import com.ti.cc3200smartplug.data.DToA_DeviceStatus;
import com.ti.cc3200smartplug.data.DToA_MetrologyCalibration;
import com.ti.cc3200smartplug.data.DToA_MetrologyData;
import com.ti.cc3200smartplug.data.DToA_SwitchTable;
import com.ti.cc3200smartplug.data.DToA_Warning;

/**
 * This TCP thread will be in charge of network communication with the Smart Plug.
 */
public class LocalConnectionThread extends AsyncTask<Void, Short, Short>{
	
	public interface LCTCallbacks{
		
		/**
		 * LCT progress update callback.
		 * @param tcpThreadCode
		 * @param device
		 * @param arg
		 * @return
		 */
		public void LCTProgressUpdate(final short tcpThreadCode, SmartPlugDevice device);
		
		/**
		 * LCT completion update callback.
		 * @param d The Device associated with the update
		 * @param end_thread_status Updating status
		 */
		public void LCTCompletionUpdate(Short end_thread_status, SmartPlugDevice device);
	}
		
	public	static final int TCP_PORT					= 1204;
	public	static final int SOCKET_CREATION_RETRY		= 3;
	private static final int SOCKET_CREATION_TIMEOUT	= 5000;
	private static final int SEND_RETRY_MAX				= 3;
	private static final int RECV_RETRY_MAX				= 3;
	public	static final int RECEIVE_TIME_MAX			= 2000;
	public	static final int MAX_BUFF_SIZE				= 1024;
	public	static final int READ_TIMEOUT				= 5000;
	
	/* GUI update codes */
	public static final short SOCKET_CREATION_SUCCESS	= 0x00;
	public static final short SOCKET_CREATION_FAIL		= 0xFF;
	public static final short DATA_READ_SUCCESS 		= 0x01;
	public static final short DATA_READ_FAIL			= 0x02;
	public static final short DATA_SEND_FAIL 			= 0x04;
	
	public static final short THREAD_END_MSG_NORMAL			= 0x20;
	public static final short THREAD_END_MSG_ERROR_SOCKET	= 0x21;
	public static final short THREAD_END_MSG_ERROR_SEND		= 0x22;
	public static final short THREAD_END_MSG_ERROR_RECV		= 0x23;
	
	// OP_CODE of Device-To-Android data. Also used by GUI update
	public static final short OP_CODE_3_1_1 = 0x7A01;
	public static final short OP_CODE_3_1_2 = 0x7A02;
	public static final short OP_CODE_3_1_3 = 0x7A04;
	public static final short OP_CODE_3_1_4 = 0x7A08;
	public static final short OP_CODE_3_1_5 = 0x7A10;
	public static final short OP_CODE_3_1_6 = 0x7A20;
	public static final short OP_CODE_3_1_7 = 0x7A40;
	public static final short OP_CODE_3_1_8 = 0x7A80;
	public static final short OP_CODE_3_1_9 = 0x7A91;
	
	/* References to SmartPlugService & DeviceDetailFragment */
	private LCTCallbacks	ThreadCaller;
	private SmartPlugDevice	thisDevice;
	private String			DEBUG_CLASS;
	
	// TX queue
	public volatile LinkedList<byte[]> TxBuffer;
	
	/** The flag to keep this thread running. Once it's set to false, thread will exit. */
	public volatile boolean flag_TCPRunning;
	
	// Certificate
	private Certificate cert;
	
	public LocalConnectionThread(LCTCallbacks caller, SmartPlugDevice device, Certificate cert) {
		this.ThreadCaller = caller;
		thisDevice = device;
		this.cert = cert;
		
		flag_TCPRunning = true;
		
		TxBuffer = new LinkedList<byte[]>();
		
		DEBUG_CLASS = thisDevice + " TCPThread";
		
		Log.d(DEBUG_CLASS, "Initialization Done");
	}
	
	/**
	 * Add data to TX queue.
	 * @param data
	 */
	public void sendData(byte[] data){
		TxBuffer.add(data);
	}

	@Override
	protected Short doInBackground(Void... param) {
		
		short threadEndMessage = THREAD_END_MSG_NORMAL;
		
		// TLS Creation, method internally reties if creation fails
		if(socketCreation()) {
			// The infinite loop will keep running until the flag "flag_TCPRunning" is set to false
			while(flag_TCPRunning){
				
				boolean shouldRead = true;
				
				if(isCancelled()){ // Check user input for cancellation
					Log.d(DEBUG_CLASS, "[acceptingServerData] User cancels");
					
					thisDevice.closeLocalSocket();
					return null;
				}
				
				/* Send Data */
				if(TxBuffer.size() != 0){
					byte[] dataToSend = TxBuffer.getFirst();
					boolean sendDataSuccessful = sendingDataToServer(dataToSend);
					
					if(sendDataSuccessful){
						TxBuffer.removeFirst();
						shouldRead = true;
					} else {
						publishProgress(DATA_SEND_FAIL);
						flag_TCPRunning = false;
						shouldRead = false;
						threadEndMessage = THREAD_END_MSG_ERROR_SEND;
					}
				}
				
				/* For unknown reason, packets are not correct without this sleep */
				try {Thread.sleep(100);}
				catch (InterruptedException e) {}
				
				/* Receive data */
				if(shouldRead){
					boolean acceptDataSuccessful = acceptingServerDataAndUpdateGUI();
					if(!acceptDataSuccessful){
						flag_TCPRunning = false;
						threadEndMessage = THREAD_END_MSG_ERROR_RECV;
					}
				}
			}
		}else{
			if(flag_TCPRunning) //If this flag is false, it means user has turned off connection, so no timer to retry in the GUI
				threadEndMessage = THREAD_END_MSG_ERROR_SOCKET;
		}
		
		/* Close TCP Socket */
		if(thisDevice.getLocalSocket()!=null){
			thisDevice.closeLocalSocket();
		}
		
		Log.d(DEBUG_CLASS, "Stops receiving data");
		return threadEndMessage;
	}
	
	/**
	 * Creates a TLS socket. This method internally retires connection up to {@link #SOCKET_CREATION_RETRY} times.
	 * @param address - The address of platform
	 * @return true if socket creation is successful
	 */
	private boolean socketCreation(){
		InetAddress address = thisDevice.getLocalAddress();
		if(address==null)
			return false;
		
		// Try connecting to server for a fix amount of time
		for(short retry=1; retry<=SOCKET_CREATION_RETRY; retry++){
			try {
				thisDevice.setLocalSocketAndConnect(mySSLSocket(address), new InetSocketAddress(address, TCP_PORT), SOCKET_CREATION_TIMEOUT);
				
				/* Confirm host exist & record the host */
				thisDevice.setLocalAddress(address);
				
				/* Set read() timeout */
				thisDevice.setLocalSocketReadTimeout(READ_TIMEOUT, false);
				
				Log.d(DEBUG_CLASS, "[socketCreation] getting data...");
				publishProgress(SOCKET_CREATION_SUCCESS);
				
				//We are done here, exit this method
				return true;
				
			} catch (IOException e1) {
				if(!flag_TCPRunning) //User request cancel
					return false;
				
				Log.e(DEBUG_CLASS, "[socketCreation] Host " + address.getHostAddress() + " Error, retry# " + retry);
				publishProgress(SOCKET_CREATION_FAIL);
			} catch (IllegalArgumentException e2){
				Log.e(DEBUG_CLASS, "[socketCreation] IllegalArgumentException Error, retry# " + retry);
			}
		}
		
		return false;
	}
	
	/**
	 * Creates an SSL socket based on the given address.
	 * @param address
	 * @return an SSLSocket upon success. Null otherwise.
	 */
	private SSLSocket mySSLSocket(InetAddress address) {
		SSLContext context = null;
		
		try{	
			// Create a KeyStore containing our trusted CAs
			KeyStore keyStore = KeyStore.getInstance(KeyStore.getDefaultType());
			keyStore.load(null, null);
			keyStore.setCertificateEntry("serverca", cert);
	
			// Create a TrustManager that trusts the CAs in our KeyStore
			String tmfAlgorithm = TrustManagerFactory.getDefaultAlgorithm();
			TrustManagerFactory tmf = TrustManagerFactory.getInstance(tmfAlgorithm);
			tmf.init(keyStore);
	
			// Create an SSLContext that uses our TrustManager
			context = SSLContext.getInstance("TLS");
			context.init(null, tmf.getTrustManagers(), null);
		} catch (Exception e){
			e.printStackTrace();
			Log.e("ERROR", "Cannot get from file");
			return null;
		}
		
		SSLSocketFactory factory = context.getSocketFactory();
		//factory = (SSLSocketFactory) SSLSocketFactory.getDefault();
		//factory = getFactoryFromAddingRegistry();
		 
		//publishProgress("Creating a SSL Socket For " + address + " on port " + TCP_PORT);
		 
		SSLSocket socket;
		
		try {
			socket = (SSLSocket) factory.createSocket();
		} catch (UnknownHostException e) {
			Log.e("ERROR", "1");
			e.printStackTrace();
			return null;
		} catch (IOException e) {
			Log.e("ERROR", "2");
			e.printStackTrace();
			return null;
		}
		
		return socket;
	}

	/**
	 * Send data to Smart Plug Server. True will return if successfully sent.
	 * This method internally retires connection up to {@link #SEND_RETRY_MAX} times.
	 * @return
	 */
	private boolean sendingDataToServer(byte[] data){
		BufferedOutputStream out = null;
		
		for(int i=0; i<SEND_RETRY_MAX; i++){
			try {
				out = new BufferedOutputStream(thisDevice.getLocalSocket().getOutputStream());
				
				try{					
					out.write(data);
					out.flush();
					Log.d(DEBUG_CLASS, "[sendingDataToServer] " + String.format("0x%2x%2x", data[0], data[1]) + " Command sent");
					
					try {Thread.sleep(100);}
					catch (InterruptedException e) {}
					
					//Buffer successfully sent, return.
					out.close();
					return true;
					
				} catch (IOException e3) {
					Log.e(DEBUG_CLASS, "[sendingDataToServer] Send command error");
				}
				
			} catch (IOException e1) {
				Log.e(DEBUG_CLASS, "Output Stream Establishment failed");
				
				//Sleep for some time and try again later.
				try {Thread.sleep(100);}
				catch (InterruptedException e) {}
			}
		}
		
		return false;
	}
	
	/**
	 * Reads data from server and update GUI. If reading fails, do nothing on the GUI.
	 * @return true if reading successful. false otherwise
	 */
	private boolean acceptingServerDataAndUpdateGUI() {	
		byte[] packetBuffer = new byte[MAX_BUFF_SIZE];
		
		for(int i=0; i<RECV_RETRY_MAX; i++){
			try {
				BufferedInputStream in = new BufferedInputStream(thisDevice.getLocalSocket().getInputStream());
				long dataReceivedTime = System.currentTimeMillis(); //Record receiving time
				
				/* Receive Data */
				try{
					int res = in.read(packetBuffer, 0, MAX_BUFF_SIZE);
					if(res==-1){
						Log.e(DEBUG_CLASS, "[acceptingServerData] buffer read end of stream");
					}else{
						/* This is where all data is received correctly, will update GUI accordingly */						
						decodePacketAndUpdateGUI(packetBuffer, res, dataReceivedTime);
					}
				
					// Data process finished successfully, return true
					return true;
					
				} catch (SocketTimeoutException e2){
					Log.e(DEBUG_CLASS, "[acceptingServerData] buffer read timeout");
				}
				
			} catch (IOException e1) {
				Log.e(DEBUG_CLASS, "[acceptingServerData] input stream error");
			}
		}
		
		return false;
	}
	
	/**
	 * This method decodes packets based to the specification, and performs actions accordingly.
	 * @param packetBuffer - Full recevied packet
	 * @param bufferSize - length of butter.
	 * @param dataReceivedTime - Time when the packet is received
	 */
	private void decodePacketAndUpdateGUI(byte[] packetBuffer, int bufferSize, long dataReceivedTime) {
		/* Convert the entire byte array to integer array for easier calculation */
		int[] intarr = new int[(int) bufferSize];
		for(int i=0; i<bufferSize; i++){
			intarr[i] = packetBuffer[i] & 0x000000FF;
		}
		
		short OP_CODE = (short) ((intarr[0] << 8) | intarr[1]);
		
//		String latestDebugMessage = "packet[0-1]=0x" + Integer.toHexString(intarr[0]) + Integer.toHexString(intarr[1]) + " OP_CODE=0x" + Integer.toHexString(OP_CODE);
//		Log.v(DEBUG_CLASS, latestDebugMessage);
		
		/* All Packets: first two bytes for OP_CODE */
		
		switch(OP_CODE){
			case OP_CODE_3_1_1:	// Packet 3.1.1 - Packet to Update Metrology Data
				/* This is where all data is received correctly, will update data & graph */	
				float activePower	= (float) (Common.ByteArrayToInt(packetBuffer, 2) / 1000.0);
				float voltage		= (float) (Common.ByteArrayToInt(packetBuffer, 6) / 1000.0);
				float current		= (float) (Common.ByteArrayToInt(packetBuffer, 10) / 1000.0);
				float frequency		= (float) (Common.ByteArrayToInt(packetBuffer, 14) / 1000.0);
				float var			= (float) (Common.ByteArrayToInt(packetBuffer, 18) / 1000.0);
				float cos			= (float) (Common.ByteArrayToInt(packetBuffer, 22) / 10000.0);
				float va			= (float) (Common.ByteArrayToInt(packetBuffer, 26) / 1000.0);
				float kWh			= (float) (Common.ByteArrayToInt(packetBuffer, 30) / 10000.0);
				float avgPower		= (float) (Common.ByteArrayToInt(packetBuffer, 34) / 1000.0);
				int timestamp1		= Common.ByteArrayToInt(packetBuffer, 38);
				
				thisDevice.addData(new DToA_MetrologyData(
						dataReceivedTime,	activePower,
						voltage,			current,
						frequency,			var,
						cos,				va,
						kWh,				avgPower,
						timestamp1));
				
				/* Final GUI update */
				publishProgress(OP_CODE_3_1_1);
				break;
				
			case OP_CODE_3_1_2:	// Packet 3.1.2 - Packet to update average energy at hourly frequency
				thisDevice.latestAvgEnergyHourly = new DToA_AvgEnergyHourly(
						dataReceivedTime,
						(float) (Common.ByteArrayToInt(packetBuffer, 2) / 10000.0),
						Common.ByteArrayToInt(packetBuffer, 6));
				publishProgress(OP_CODE_3_1_2);
				break;
				
			case OP_CODE_3_1_3:	// Packet 3.1.3 - Packet to update device status
				thisDevice.latestDeviceStatus = new DToA_DeviceStatus(
						dataReceivedTime,
						packetBuffer[2] == 0x00 ? false:true,
						packetBuffer[3] == 0x00 ? false:true,
						packetBuffer[4],
						(float) (Common.ByteArrayToLong(packetBuffer, 5) / 10000.0),
						(float) (Common.ByteArrayToInt(packetBuffer, 9) / 1000.0),
						Common.ByteArrayToInt(packetBuffer, 13));			
				publishProgress(OP_CODE_3_1_3);
				break;
				
			case OP_CODE_3_1_4:	// Packet 3.1.4 - Warning message for over energy and power consumption threshold
				//Divide into three parts due to each one being unique
				//over energy
				thisDevice.latestWarningOverEnergy = new DToA_Warning(
						dataReceivedTime,
						packetBuffer[2] == 0x00 ? false:true,
						Common.ByteArrayToInt(packetBuffer, 5));
				
				//over power
				thisDevice.latestWarningOverPower = new DToA_Warning(
						dataReceivedTime,
						packetBuffer[3] == 0x00 ? false:true,
						Common.ByteArrayToInt(packetBuffer, 5));
				
				//module error
				thisDevice.latestWarningModuleError = new DToA_Warning(
						dataReceivedTime,
						packetBuffer[4] == 0x00 ? false:true,
						Common.ByteArrayToInt(packetBuffer, 5));
				publishProgress(OP_CODE_3_1_4);
				break;
				
			case OP_CODE_3_1_5:	// Packet 3.1.5 - Average energy (KWh) of last 24 hours
				float list[] = new float[24];
				for (short i=0; i<24; i++){
					list[i] = (float) (Common.ByteArrayToInt(packetBuffer, (4*i)+2) / 10000.0);
				}
				thisDevice.latestAvgEnergyDaily = new DToA_AvgEnergyDaily(
						dataReceivedTime,
						list,
						Common.ByteArrayToInt(packetBuffer, 98));
				publishProgress(OP_CODE_3_1_5);
				break;
				
			case OP_CODE_3_1_6:	// Packet 3.1.6 - Device's default switch table
				thisDevice.latestSwitchTable = new DToA_SwitchTable(dataReceivedTime);
				for(short i=0; i<8; i++){
					thisDevice.latestSwitchTable.weekdaySchedule[i] = packetBuffer[2 + i];
					thisDevice.latestSwitchTable.saturdaySchedule[i] = packetBuffer[10 + i];
					thisDevice.latestSwitchTable.sundaySchedule[i] = packetBuffer[18 + i];
				}
				System.arraycopy(packetBuffer, 26, thisDevice.latestSwitchTable.smartPlugTime, 0, 3);
				thisDevice.latestSwitchTable.active = (packetBuffer[29] == 0x00? false:true);
				publishProgress(OP_CODE_3_1_6);
				break;
				
			case OP_CODE_3_1_7:	// Packet 3.1.7 - Cloud specific requirements			
				thisDevice.latestCloudInfo = new DToA_CloudInfo(dataReceivedTime);
				try {
					thisDevice.latestCloudInfo.vendor = new String(packetBuffer, 2, 17, "UTF-8");
				} catch (UnsupportedEncodingException e) {
					Log.e("LocalTCPConnectionThread", "Packet OP_CODE_3_1_7: Vendor Chartset error. Default to texasinstruments");
					thisDevice.latestCloudInfo.vendor = "texasinstruments";
				}
				try {
					thisDevice.latestCloudInfo.model = new String(packetBuffer, 19, 12, "UTF-8");
				} catch (UnsupportedEncodingException e) {
					Log.e("LocalTCPConnectionThread", "Packet OP_CODE_3_1_7: Model Chartset error. Default to smartplugv2");
					thisDevice.latestCloudInfo.model = "smartplugv2";
				}
				System.arraycopy(packetBuffer, 31, thisDevice.latestCloudInfo.MAC, 0, 6);
				thisDevice.latestCloudInfo.cloudStatus = packetBuffer[37];
				thisDevice.latestCloudInfo.timestamp = Common.ByteArrayToInt(packetBuffer, 38);
				publishProgress(OP_CODE_3_1_7);
				break;
				
			case OP_CODE_3_1_8:	// Packet 3.1.8 - Metrology calibration specific requirements
				thisDevice.latestMetrologyCalibration = new DToA_MetrologyCalibration(
						dataReceivedTime,
						(float) (Common.ByteArrayToInt(packetBuffer, 2) / 100000.0),
						(float) (Common.ByteArrayToInt(packetBuffer, 6) / 100000.0),
						(float) (Common.ByteArrayToInt(packetBuffer, 10) / 100000.0),
						(float) (Common.ByteArrayToInt(packetBuffer, 14) / 100000.0));
				publishProgress(OP_CODE_3_1_8);
				break;
				
			case OP_CODE_3_1_9:	// Packet 3.1.9 - Indicating success in updating Exosite SSL CA certificate
				publishProgress(OP_CODE_3_1_9);
				break;
				
			default:
				StringBuilder sb = new StringBuilder();
				sb.append("Unknown packet: OP_CODE=");
				sb.append(String.format("0x%04X", OP_CODE));
				sb.append(", data=");
				sb.append(Common.ByteArrayToString(packetBuffer, 2, bufferSize, false));
				Log.e("LocalTCPConnectionThread", sb.toString());
		}
	}
	
	@Override
	protected void onProgressUpdate(Short... progress){
		/* Parameters:
		 * progress[0] = GUI update type
		 */
		
		//This part will be processed in the main service thread, and then we use
		//broadcast to send messages to the the DDF in MainActivity
		// Starts a chain of updating the GUI
		ThreadCaller.LCTProgressUpdate(progress[0], thisDevice);
	}
	
	@Override
	public void onPostExecute(Short end_thread_status){
		ThreadCaller.LCTCompletionUpdate(end_thread_status, thisDevice);
		Log.d(DEBUG_CLASS, "EXIT");
	}
}

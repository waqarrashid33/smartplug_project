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

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

public class SPDeviceHandler extends SQLiteOpenHelper{
	
	private static final int DATABASE_VERSION = 1;
	private static final String DATABASE_NAME = "SPDeviceCollection.db";
	
	// Device Detail Table
    private static final String TABLE_DEVICE	= "SPDevices";
    private static final String KEY_ID			= "id";
    public static final String KEY_RID			= "rid";
    public static final String KEY_NAME			= "name";
    public static final String KEY_MAC			= "mac";
    public static final String KEY_CIK			= "cik";
    public static final String KEY_MODEL		= "model";
    public static final String KEY_VENDOR		= "vendor";
    public static final String KEY_LOCAL_ADDR	= "localAddr";
    public static final String KEY_CONN_PREF	= "connPref";
    public static final String KEY_ASSO_ACCOUNT	= "associatedAccount";
    
    private static final int	KEY_INDEX_ID	= 0;
    private static final int	KEY_INDEX_RID	= 1;
    private static final int	KEY_INDEX_NAME	= 2;
    private static final int	KEY_INDEX_MAC	= 3;
    private static final int	KEY_INDEX_CIK	= 4;
    private static final int	KEY_INDEX_MODEL = 5;
    private static final int	KEY_INDEX_VENDOR = 6;
    private static final int	KEY_INDEX_LOCAL_ADDR	= 7;
    private static final int	KEY_INDEX_CONN_PREF		= 8;
    private static final int	KEY_INDEX_ASSO_ACCOUNT	= 9;
    
    private Context mContext;
    
    public SPDeviceHandler(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
        mContext = context;
    }
 
    // Creating Tables
    @Override
    public void onCreate(SQLiteDatabase db) {
        String CREATE_CONTACTS_TABLE = "CREATE TABLE " + TABLE_DEVICE + "("
                + KEY_ID		+ " INTEGER PRIMARY KEY,"
                + KEY_RID		+ " TEXT,"
        		+ KEY_NAME		+ " TEXT,"
        		+ KEY_MAC		+ " TEXT,"
        		+ KEY_CIK		+ " TEXT,"
        		+ KEY_MODEL		+ " TEXT,"
                + KEY_VENDOR	+ " TEXT,"
                + KEY_LOCAL_ADDR	+ " TEXT,"
                + KEY_CONN_PREF		+ " INTEGER,"
                + KEY_ASSO_ACCOUNT	+ " TEXT"+ ")";
        db.execSQL(CREATE_CONTACTS_TABLE);
    }
 
    // Upgrading database
    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        // Drop older table if existed
        db.execSQL("DROP TABLE IF EXISTS " + TABLE_DEVICE);
 
        // Create tables again
        onCreate(db);
    }
    
	// Adding new contact
    public void addSPDevice(SmartPlugDevice device) {
    	SQLiteDatabase db = this.getWritableDatabase();
    	 
        ContentValues values = new ContentValues();
        // ID is automatically added
        
        // RID may be null
        String rid = device.getRID();
        if(rid==null)
        	rid = "null";
        values.put(KEY_RID, rid);
        
        values.put(KEY_NAME, device.getName());
        values.put(KEY_MAC, device.getMAC());
        
        // CIK may be null
        String cik = device.getCIK();
        if(cik==null)
        	cik = "null";
        values.put(KEY_CIK, cik);
        
        values.put(KEY_MODEL, device.getModel());
        values.put(KEY_VENDOR, device.getVendor());
        
        // address may be null
        String add = "null";
        InetAddress address = device.getLocalAddress();
        if(address!=null)
        	add = address.getHostAddress();
        
        values.put(KEY_LOCAL_ADDR, add);
        values.put(KEY_CONN_PREF, device.getConnectionPreference());
     
        String asso = device.getAssociatedUser();
        if(asso==null)
        	asso = "null";
        values.put(KEY_ASSO_ACCOUNT, asso);
        
        // Inserting Row
        db.insert(TABLE_DEVICE, null, values);
        db.close(); // Closing database
    }
     
    /**
     * Returns an SPDevice object. null of the index item cannot be found.
     * @param id
     * @return
     */
    public SmartPlugDevice getSPDevice(int id) {
    	SQLiteDatabase db = this.getReadableDatabase();
    	 
        Cursor cursor = db.query(
        		TABLE_DEVICE,
        		new String[] {KEY_ID, KEY_RID, KEY_NAME, KEY_MAC, KEY_CIK, KEY_MODEL, KEY_VENDOR, KEY_LOCAL_ADDR, KEY_CONN_PREF},
        		KEY_ID + "=?",
                new String[] { String.valueOf(id) },
                null,
                null,
                null,
                null);
        
        if (cursor != null)
            cursor.moveToFirst();
     
        SmartPlugDevice device;
        
		try {
			String rid = cursor.getString(KEY_INDEX_RID);
	        if(rid.equals("null"))
	        	rid = null;
			
			String cik = cursor.getString(KEY_INDEX_CIK);
	        if(cik.equals("null"))
	        	cik = null;
	        
	        String address = cursor.getString(KEY_INDEX_LOCAL_ADDR);
	        if(address.equals("null"))
	        	address = null;
	        
	        String asso = cursor.getString(KEY_INDEX_ASSO_ACCOUNT);
	        if(asso.equals("null"))
	        	asso = null;
			
			device = new SmartPlugDevice(mContext,
					rid,
					cursor.getString(KEY_INDEX_NAME),	//Name
					cursor.getString(KEY_INDEX_MAC),	//MAC address
					cik,								//CIK
					cursor.getString(KEY_INDEX_MODEL),	//Model Name
					cursor.getString(KEY_INDEX_VENDOR),	//Vendor
					InetAddress.getByName(address),		//local address
					cursor.getShort(KEY_INDEX_CONN_PREF),
					asso);	//Connection Preference
			
			device.database_id = Integer.parseInt(cursor.getString(KEY_INDEX_ID));	//ID
		} catch (UnknownHostException e) {
			return null;
		} finally {
			db.close();
		}
		
        // return contact
        return device;
    }
     
    // Getting All SPDevices
    public ArrayList<SmartPlugDevice> getAllSPDevices() {
    	ArrayList<SmartPlugDevice> list = new ArrayList<SmartPlugDevice>(getSPDevicesCount());
    	
    	String selectQuery = "SELECT  * FROM " + TABLE_DEVICE;
    	SQLiteDatabase db = this.getWritableDatabase();
        Cursor cursor = db.rawQuery(selectQuery, null);
        
        if (cursor.moveToFirst()) {
            do {
            	SmartPlugDevice device = null;
            	try {
            		String rid = cursor.getString(KEY_INDEX_RID);
        	        if(rid.equals("null"))
        	        	rid = null;
            		
            		String cik = cursor.getString(KEY_INDEX_CIK);
        	        if(cik.equals("null"))
        	        	cik = null;
        	        
        	        InetAddress address = null;
        	        
        	        String addressStr = cursor.getString(KEY_INDEX_LOCAL_ADDR);
        	        if(!addressStr.equals("null"))
        	        	address = InetAddress.getByName(addressStr);
        	        
        	        String asso = cursor.getString(KEY_INDEX_ASSO_ACCOUNT);
        	        if(asso.equals("null"))
        	        	asso = null;
            		
        			device = new SmartPlugDevice(mContext,
        					rid,
        					cursor.getString(KEY_INDEX_NAME), //Name
        					cursor.getString(KEY_INDEX_MAC), //MAC address
        					cik,
        					cursor.getString(KEY_INDEX_MODEL), //Model Name
        					cursor.getString(KEY_INDEX_VENDOR), //Vendor
        					address,
        					cursor.getShort(KEY_INDEX_CONN_PREF), //IPv4 address in String
        					asso);
        			
        			device.database_id = Integer.parseInt(cursor.getString(KEY_INDEX_ID));	//ID
        		} catch (UnknownHostException e) {
        			Log.e("SPDeviceHandler", "SPDevice construction error: UnknownHostException");
        		}
            	
                // Adding contact to list
            	list.add(device);
            } while (cursor.moveToNext());
        }
		
        db.close();
    	return list;
    }
     
    /**
     * Returns the number of SPDevices in database.
     * @return
     */
    public int getSPDevicesCount() {
    	String countQuery = "SELECT  * FROM " + TABLE_DEVICE;
    	SQLiteDatabase db = this.getReadableDatabase();
    	Cursor cursor = db.rawQuery(countQuery, null);
    	int count = cursor.getCount();
    	cursor.close();
  
    	// return count
    	return count;
    }
        
    /**
     * Updates SPDevice kay-value pair.
     * @param device
     * @return 0 if successful. A negative value if error.
     */
    public int updateKey(String mac, String key, String value){
    	String newVal = "null";
    	
    	if(value!=null){
    		newVal = value;
    	}
    	
    	SQLiteDatabase db = this.getWritableDatabase();
    	String strFilter = String.format("%s='%s'", KEY_MAC, mac);
    	ContentValues args = new ContentValues();
    	args.put(key, newVal);
    	db.update(TABLE_DEVICE, args, strFilter, null);
    	db.close();
    	return 0;
    }
    
    public int updateKey(String mac, String key, int value) {
    	SQLiteDatabase db = this.getWritableDatabase();
    	String strFilter = String.format("%s='%s'", KEY_MAC, mac);
    	ContentValues args = new ContentValues();
    	args.put(key, value);
    	db.update(TABLE_DEVICE, args, strFilter, null);
    	db.close();
    	return 0;
    }
     
    /**
     * Deletes a single SPDevice.
     * @param device
     */
    public void deleteSPDevice(SmartPlugDevice device) {
    	SQLiteDatabase db = this.getWritableDatabase();
        db.delete(TABLE_DEVICE, KEY_MAC + "='" + device.getMAC() + "'", null);
        db.close();
    }
}

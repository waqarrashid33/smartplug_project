package com.ti.cc3200smartplug.service;

import java.util.ArrayList;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

import com.ti.cc3200smartplug.data.DToA_AvgEnergyDaily;
import com.ti.cc3200smartplug.data.DToA_MetrologyData;

public class SPDeviceDataHandler extends SQLiteOpenHelper{
	
	public static final int MAX_METROLOGY_DATA_COUNT = 256;
	
	private Context mContext;
	
	/* SQL variables */
    private static final int DATABASE_VERSION = 1;
    private static String DATABASE_NAME = null;
    
	// Metrology Data table
    private static final String TABLE_M			= "MetrologyData";
    private static final String KEY_M_TIME		= "time";
    private static final String KEY_M_POWER		= "power";
    private static final String KEY_M_VOLTAGE	= "voltage";
    private static final String KEY_M_CURRENT	= "current";
    private static final String KEY_M_FREQUENCY	= "freq";
    private static final String KEY_M_VAR		= "var";
    private static final String KEY_M_COS		= "cos";
    private static final String KEY_M_VA		= "va";
    private static final String KEY_M_KWH		= "KWh";
    private static final String KEY_M_AVG		= "avg";
    private static final String KEY_M_TIMESTAMP	= "timestamp"; 
    
    // 24 Hour Data Table
    private static final String TABLE_D 		= "DailyAverageData";
    private static final String KEY_D_TIME		= "timeD";
    private static final String[] KEY_D_AVG_E	= new String[24];
    private static final String KEY_D_TIMESTAMP	= "timestampD";
    
    public SPDeviceDataHandler(Context context, String mac) {
        super(
        		context,
        		"SPDevice_" + mac + ".db",
        		null,
        		DATABASE_VERSION);
        
        mContext = context;
        DATABASE_NAME = "SPDevice_" + mac + ".db";
    }
    
	// Creating Tables
    @Override
    public void onCreate(SQLiteDatabase db) {
    	// Creates the Metrology table	
        String CREATE_METROLOGY_TABLE = "CREATE TABLE " + TABLE_M + "("
                + KEY_M_TIME	+ " INTEGER PRIMARY KEY,"
                + KEY_M_POWER	+ " TEXT,"
        		+ KEY_M_VOLTAGE	+ " TEXT,"
        		+ KEY_M_CURRENT	+ " TEXT,"
        		+ KEY_M_FREQUENCY + " TEXT,"
        		+ KEY_M_VAR		+ " TEXT,"
        		+ KEY_M_COS		+ " TEXT,"
        		+ KEY_M_VA		+ " TEXT,"
        		+ KEY_M_KWH		+ " TEXT,"
        		+ KEY_M_AVG		+ " TEXT,"
                + KEY_M_TIMESTAMP + " INTEGER" + ")";
        
        for (int i=0; i<24; i++){
        	KEY_D_AVG_E[i] = "hourEnergy" + i;
        }
        db.execSQL(CREATE_METROLOGY_TABLE);
        
        // Creates the 24hour table		
        StringBuilder CREATE_DAILY_TABLE = new StringBuilder();
        CREATE_DAILY_TABLE.append("CREATE TABLE " + TABLE_D + "(" + KEY_D_TIME + " INTEGER PRIMARY KEY,");
        for (int i=0; i<24; i++){
        	CREATE_DAILY_TABLE.append(KEY_D_AVG_E[i] + " TEXT,");
        }        
        CREATE_DAILY_TABLE.append(KEY_D_TIMESTAMP + " INTEGER" + ")");
        db.execSQL(CREATE_DAILY_TABLE.toString());
    }
    
	// Upgrading database
    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
    	Log.w(this.getClass().getName(),
    	        "Upgrading database from version " + oldVersion + " to " + newVersion + ", which will destroy all old data");
    	
        // Drop older table if existed
        db.execSQL("DROP TABLE IF EXISTS " + TABLE_M);
        db.execSQL("DROP TABLE IF EXISTS " + TABLE_D);
 
        // Create tables again
        onCreate(db);
    }
    
    public boolean delteDatabase(){
    	return mContext.deleteDatabase(DATABASE_NAME);
    }
    
	// ================================= Metrology Data Operation ================================
    
    /**
     * Adds a single Metrology data.
     * @param data Data to be added
     * @param receiveTime data received time
     * @return Current size of Metrology data table. Maximum is {@link #MAX_METROLOGY_DATA_COUNT}.
     */
    public int addMetrologyData(DToA_MetrologyData data) {
    	SQLiteDatabase db = getWritableDatabase();

    	ContentValues values = new ContentValues();
    	values.put(KEY_M_TIME,		data.dataReceivedTime);
    	values.put(KEY_M_POWER,		data.activePower);
    	values.put(KEY_M_VOLTAGE,	data.voltage);
    	values.put(KEY_M_CURRENT,	data.current);
    	values.put(KEY_M_FREQUENCY,	data.frequency);
    	values.put(KEY_M_VAR,		data.reactivePower);
    	values.put(KEY_M_COS,		data.cos);
    	values.put(KEY_M_VA,		data.activePower);
    	values.put(KEY_M_KWH,		data.KWh);
    	values.put(KEY_M_AVG,		data.avgPower);
    	values.put(KEY_M_TIMESTAMP,	data.timestamp);
     
        // Inserting Row
        db.insert(TABLE_M, null, values);
        db.close();
        
        if(getMetrologyDataCount() > 100){
        	deleteMetrologyDataFirst();
        }
    	
    	return getMetrologyDataCount();
    }
        
    /**
     * Retrieves Metrology data from a givn index.
     * @param index ranges from 0 to {@link #MAX_METROLOGY_DATA_COUNT} -1
     * @return the requested data. null if item cannot be found
     */
    public DToA_MetrologyData getMetrologyData(int index) {
        String selectQuery = "SELECT  * FROM " + TABLE_M;
     
        SQLiteDatabase db = this.getWritableDatabase();
        Cursor cursor = db.rawQuery(selectQuery, null);
        
        if(!cursor.moveToPosition(index)){
        	//no such data
        	return null;
        }
        
        return new DToA_MetrologyData(
    			Long.parseLong(cursor.getString(0)),	//DataReceiveTime
    			Float.parseFloat(cursor.getString(1)),	//ActivePower
    			Float.parseFloat(cursor.getString(2)),	//Voltage
    			Float.parseFloat(cursor.getString(3)),	//Current
    			Float.parseFloat(cursor.getString(4)),	//Frequency
    			Float.parseFloat(cursor.getString(5)),	//VAR
    			Float.parseFloat(cursor.getString(6)),	//COS
    			Float.parseFloat(cursor.getString(7)),	//VA
    			Float.parseFloat(cursor.getString(8)),	//KWh
    			Float.parseFloat(cursor.getString(9)),	//AveragePower
    			Integer.parseInt(cursor.getString(10))	//Timestamp
    			);
    }
        
    /**
     * Retrieves all historical Metrology data.
     * @return an ArrayList of Metrology data
     */
    public ArrayList<DToA_MetrologyData> getAllMetrologyData() {
    	ArrayList<DToA_MetrologyData> contactList = new ArrayList<DToA_MetrologyData>();
        // Select All Query
        String selectQuery = "SELECT * FROM " + TABLE_M;
     
        SQLiteDatabase db = this.getWritableDatabase();
        Cursor cursor = db.rawQuery(selectQuery, null);
     
        // looping through all rows and adding to list
        if (cursor.moveToFirst()) {
            do {
            	DToA_MetrologyData data = new DToA_MetrologyData(
            			Long.parseLong(cursor.getString(0)),	//DataReceiveTime
            			Float.parseFloat(cursor.getString(1)),	//ActivePower
            			Float.parseFloat(cursor.getString(2)),	//Voltage
            			Float.parseFloat(cursor.getString(3)),	//Current
            			Float.parseFloat(cursor.getString(4)),	//Frequency
            			Float.parseFloat(cursor.getString(5)),	//VAR
            			Float.parseFloat(cursor.getString(6)),	//COS
            			Float.parseFloat(cursor.getString(7)),	//VA
            			Float.parseFloat(cursor.getString(8)),	//KWh
            			Float.parseFloat(cursor.getString(9)),	//AveragePower
            			Integer.parseInt(cursor.getString(10))	//Timestamp
            			);
                // Adding contact to list
                contactList.add(data);
            } while (cursor.moveToNext());
        }
     
        // return contact list
        return contactList;
    }
    
    public ArrayList<DToA_MetrologyData> getMetrologyDataInRange(long startTime, long endTime){
//    	SQLiteQueryBuilder builder = new SQLiteQueryBuilder();
//        builder.setTables(TABLE_M);
//        Cursor cursor = builder.query(
//        		getReadableDatabase(),
//        		null,
//        		KEY_M_TIME + " BETWEEN ? AND ?",
//        		new String[]{Long.toString(startTime), Long.toString(endTime)},
//        		null, null, null);
        
    	SQLiteDatabase db = this.getReadableDatabase();
        Cursor cursor = db.rawQuery( "SELECT * FROM " + TABLE_M + " WHERE " + KEY_M_TIME + " BETWEEN " + startTime + " AND " + endTime, null);
        
        ArrayList<DToA_MetrologyData> ret = new ArrayList<DToA_MetrologyData>();
        
        if (cursor.moveToFirst()) {
            do {
            	DToA_MetrologyData data = new DToA_MetrologyData(
            			Long.parseLong(cursor.getString(0)),	//DataReceiveTime
            			Float.parseFloat(cursor.getString(1)),	//ActivePower
            			Float.parseFloat(cursor.getString(2)),	//Voltage
            			Float.parseFloat(cursor.getString(3)),	//Current
            			Float.parseFloat(cursor.getString(4)),	//Frequency
            			Float.parseFloat(cursor.getString(5)),	//VAR
            			Float.parseFloat(cursor.getString(6)),	//COS
            			Float.parseFloat(cursor.getString(7)),	//VA
            			Float.parseFloat(cursor.getString(8)),	//KWh
            			Float.parseFloat(cursor.getString(9)),	//AveragePower
            			Integer.parseInt(cursor.getString(10))	//Timestamp
            			);
                // Adding contact to list
                ret.add(data);
            } while (cursor.moveToNext());
        }
        
        return ret;
    }
    
    /**
     * Get the total number of Metrology data. Maximum is {@link #MAX_METROLOGY_DATA_COUNT}.
     * @return count
     */
    public int getMetrologyDataCount() {
    	String countQuery = "SELECT  * FROM " + TABLE_M;
        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cursor = db.rawQuery(countQuery, null);
        int count = cursor.getCount();
        cursor.close();
        return count;
    }
    
    /**
     * Delete the first Metrology data from the table
     * @return rows affected with this deletion
     */
    public int deleteMetrologyDataFirst() {
    	return deleteMetrologyData(getMetrologyData(0));
    }
    
    /**
     * Delete the last Metrology data from the table
     * @return rows affected with this deletion
     */
    public int deleteMetrologyDataLast() {
    	return deleteMetrologyData(getMetrologyData(getMetrologyDataCount()-1));
    }
    
    /**
     * Deletes all Metrology historical data
     * @return rows affected with this deletion
     */
    public int deleteMetrologyDataAll() {
    	int rows_affected = 0;
    	
    	SQLiteDatabase db = this.getWritableDatabase();
    	rows_affected = db.delete(TABLE_M, null, null);
        db.close();
        
        return rows_affected;
    }
    
    /**
     * Deletes a specific Metrology data
     * @param data item to be deleted
     * @return rows affected with this deletion
     */
    public int deleteMetrologyData(DToA_MetrologyData data){
    	int rows_affected = 0;
    	
    	SQLiteDatabase db = this.getWritableDatabase();
    	rows_affected = db.delete(TABLE_M,
        		KEY_M_TIME + " = ?",
                new String[] { String.valueOf(data.dataReceivedTime) });
        db.close();
        
        return rows_affected;
    }
    
	// ================================= 24H Data Operation ================================
    
    /**
     * Adds a 24hour average energy data.
     * @param data
     * @return total number of items in the table. Should be 1 only.
     */
    public int add24HourData(DToA_AvgEnergyDaily data) {
    	SQLiteDatabase db = getWritableDatabase();

    	ContentValues values = new ContentValues();
    	values.put(KEY_D_TIME, data.dataReceivedTime);
    	for (int i=0; i<24; i++){
    		values.put(KEY_D_AVG_E[i], data.getHourData(i));
        }
    	values.put(KEY_D_TIME, data.timestamp);
    	
    	// Inserting Row
        db.insert(TABLE_D, null, values);
        db.close();
    	
    	return get24HourDataCount();
    }
    
    /**
     * Get the total number of Daily Average Energy data
     * @return count
     */
    public int get24HourDataCount() {
    	String countQuery = "SELECT  * FROM " + TABLE_D;
        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cursor = db.rawQuery(countQuery, null);
        cursor.close();
        return cursor.getCount();
    }
     
    /**
     * Retrieves an Average Energy data with an given hour in the last 24 hour.
     * @param index hour to retrieve. 0 for last hour, 1 for past 1-2 hour. So on and so forth.
     * @return average energy value of that hour
     */
    public float getHourData(int index){
    	return get24HourData().getHourData(index);
    }
    
    /**
     * Retrieves the entire set of Average Energy data in the last 24 hour.
     * @return the set
     */
    public DToA_AvgEnergyDaily get24HourData(){
    	String selectQuery = "SELECT  * FROM " + TABLE_M;
        
        SQLiteDatabase db = this.getWritableDatabase();
        Cursor cursor = db.rawQuery(selectQuery, null);
        
        if(!cursor.moveToFirst()){
        	//no such data
        	return null;
        }
        
        float[] values = new float[24];
        for (int i=0; i<24; i++){
        	values[0] = Float.parseFloat(cursor.getString(1 + i));
        }
        
        return new DToA_AvgEnergyDaily(
    			Integer.parseInt(cursor.getString(0)),	//DataReceiveTime
    			values,
    			Integer.parseInt(cursor.getString(25))	//Timestamp
    			);
    }   

    /**
     * Deletes the only one 24Hour data
     * @param data item to be deleted
     * @return rows affected with this deletion
     */
    public int delete24HourData(){
    	int rows_affected = 0;
    	
    	SQLiteDatabase db = this.getWritableDatabase();
    	rows_affected = db.delete(TABLE_M, null, null);
        db.close();
        
        return rows_affected;
    }
    
}

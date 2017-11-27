package com.ti.cc3200smartplug;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.json.JSONArray;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.preference.PreferenceManager;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.Switch;

import com.exosite.api.ExoCallback;
import com.exosite.api.ExoException;
import com.exosite.api.portals.Portals;
import com.ti.cc3200smartplug.service.SmartPlugService;

public class Common {
	
	private static SmartPlugService sps;
	
	public static final String PREFERENCE_EMAIL = "email";
	public static final String PREFERENCE_PASSWORD = "password";
	public static final String PREFERENCE_LOGGED_IN = "logged_in";
	
	public static final String domainIdentifier = "((\\p{Alnum})([-]|(\\p{Alnum}))*(\\p{Alnum}))|(\\p{Alnum})"; 
	public static final String domainNameRule = "(" + domainIdentifier + ")((\\.)(" + domainIdentifier + "))*";
	public static final String oneAlpha="(.)*((\\p{Alpha})|[-])(.)*";
	public static final String REG_EXP_IP_ADDRESS = "^([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\." + "([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\." +
			"([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\." + "([01]?\\d\\d?|2[0-4]\\d|25[0-5])$";
//	public static final String REG_EXP_MAC_ADDRESS = "^([0-9a-f]{12})$";
	public static final String REG_EXP_EMAIL = "^[_A-Za-z0-9-\\+]+(\\.[_A-Za-z0-9-]+)*@" + "[A-Za-z0-9-]+(\\.[A-Za-z0-9]+)*(\\.[A-Za-z]{2,})$";

	/**
	 * This method is used for programmatically change Switch state in order to solve the problem of
	 * OnCheckedChangeListener being toggled programmatically even though the user has no such intention.
	 * @param s - The switch to be switched
	 * @param check
	 * @param originalListener - The original CompoundButton.OnCheckedChangeListener used
	 */
	public static void silentlySwitchCheck(Switch s, boolean check, CompoundButton.OnCheckedChangeListener originalListener){
		s.setOnCheckedChangeListener(null);
		s.setChecked(check);
		s.setOnCheckedChangeListener(originalListener);
	}
	
	public static void fixOrientation(Activity act){
		if(act.getResources().getBoolean(R.bool.portrait)){
			act.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
	    }else{
	    	act.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
	    }
	}
	
	public static boolean isEmpty(EditText etText) {
        return etText.getText().toString().trim().length() == 0;
    }
	
	/**
	 * Converts DP to PX. An equation from Google.
	 * @param dp
	 * @return pixel
	 */
	public static int dp_to_pixel(Context c, float dp){
		return (int) (dp * c.getResources().getDisplayMetrics().density + 0.5f);
	}
	
	public static void setService(SmartPlugService s){
		sps = s;
	}
	
	public static SmartPlugService getService(){
		return sps;
	}
	
	/**
	 * Converts a hex string to byte array.
	 * @param s
	 * @return
	 */
	public static byte[] hexStringToByteArray(String s) {
	    int len = s.length();
	    byte[] data = new byte[len / 2];
	    for (int i = 0; i < len; i += 2) {
	        data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
	                             + Character.digit(s.charAt(i+1), 16));
	    }
	    return data;
	}
	
	/**
	 * Converts a 4-byte section of a byte array into an integer.
	 * @param arr
	 * @param starting_position
	 * @return the converted integer. -1 if error occurs.
	 */
	public static int ByteArrayToInt(byte[] arr, int starting_position){
		
		if(starting_position < 0)
			return -1;
		
		if((starting_position+3) >= arr.length)
			return -1;
		
		return ByteBuffer.wrap(arr, starting_position, 4).order(ByteOrder.LITTLE_ENDIAN).getInt();
	}
	
	/**
	 * An extra method to handle unsigned 32-bit integer.
	 * So even though it produces a long value, this method will take only 4 bytes at a time, from the "starting_position".
	 * @param arr
	 * @param starting_position
	 * @return
	 */
	public static long ByteArrayToLong(byte[] arr, int starting_position){
		
		if(starting_position < 0)
			return -1;
		
		if((starting_position+3) >= arr.length)
			return -1;
		
		
		byte[] newArr = new byte[8];
		for(int i=0; i<4; i++){
			newArr[i] = arr[starting_position+i];
		}
		for(int i=4; i<8; i++){
			newArr[i] = 0x00;
		}
		
		return ByteBuffer.wrap(newArr, 0, 8).order(ByteOrder.LITTLE_ENDIAN).getLong();
	}
	
	/**
	 * Converts an byte array to String. If the "withSemicolon" flag is true, it will produce a MAC address-like string.
	 * @param arr
	 * @param starting_position
	 * @param length
	 * @param withSemicolon
	 * @return
	 */
	public static String ByteArrayToString(byte[] arr, int starting_position, int length, boolean withSemicolon){
		
		if(starting_position < 0)
			return null;
		
		if(length <= 0)
			return null;
		
		if((starting_position+3) >= length)
			return null;
		
		StringBuilder hex = new StringBuilder();
		for (int i=0; i<length; i++){
			hex.append(String.format("%02X", arr[i+starting_position]));
			if(withSemicolon && i<(length-1)){
				hex.append(":");
			}
		}
		    
		
		return hex.toString();
	}

	/**
	 * Converts an 32-bit signed integer to a 4-byte array. Little endian.
	 * @param value
	 * @return
	 */
	public static byte[] intToByteArray(int value){
		return new byte[] {
				(byte) value,
				(byte) (value >>> 8),
				(byte) (value >>> 16),
				(byte) (value >>> 24)
		};
	}
	
	/**
	 * This extra method is used for unsigned 32-bit integer scenario.
	 * So even though we are taking in a long type value, ultimately it will only return an array of 4 bytes.
	 * @param value
	 * @return
	 */
	public static byte[] longToByteArray(long value){
		
		byte[] a = new byte[] {
				(byte) value,
				(byte) (value >>> 8),
				(byte) (value >>> 16),
				(byte) (value >>> 24)
		};
		
		return a;
	}
	
	public interface CheckCredentialCallBack{
		public void onReturn(boolean success, String message);
	};

	/**
	 * Sets the logged in status.
	 * @param Activity any activity. If it's a {@link com.ti.cc3200smartplug.SmartPlugBaseActivity SmartPlugBaseActivity},
	 * {@link com.ti.cc3200smartplug.SmartPlugBaseActivity SmartPlugBaseActivity}.signedInStatusChanged() will be called.
	 * @param c - use getApplicationContext()
	 * @param status
	 */
	public static void setLoggedIn(Activity spba, Context c, boolean status){
		SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(c);
		sharedPreferences.edit().putBoolean(PREFERENCE_LOGGED_IN, status).commit();
		
		if(spba instanceof SmartPlugBaseActivity){
			((SmartPlugBaseActivity) spba).signedInStatusChanged();
		}
	}
	
	/**
	 * Gets the logged in status.
	 * @param c - use getApplicationContext()
	 */
	public static boolean isLoggedIn(Context c){
		SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(c);
		return sharedPreferences.getBoolean(PREFERENCE_LOGGED_IN, false);
	}
	
	public static String getEmail(Context c){
		SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(c);
		return sharedPreferences.getString(PREFERENCE_EMAIL, "Not Singed In");
	}
	
	/**
	 * Checks credential with email and password stored in SharedPreferences.
	 * @param c Context to be checked.
	 * @param callback A callback function once checking is complete.
	 */
	public static void checkCredential(Context c, final CheckCredentialCallBack callback){
		
		SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(c);
		String mEmail = sharedPreferences.getString("email", "");
		String mPassword = sharedPreferences.getString("password", "");

		Portals.listPortalsInBackground(mEmail, mPassword, new ExoCallback<JSONArray>() {
            @Override
            public void done(JSONArray result, ExoException e) {
                if (result != null) {
                	callback.onReturn(true, "SUCCESS");
                } else {
                	callback.onReturn(false, "Invalid email or password");
                }
            }
        });
	}
	
	/**
     * Shows the progress UI and hides the login form.
     * The act must contain R.id.form and R.id.status
     * @param act
     * @param show
     */
    public static void showProgressForForm(Activity act, final boolean show) {
    	final View mFormView = act.findViewById(R.id.form);
    	final View mStatusView = act.findViewById(R.id.status_ref);
        
    	int shortAnimTime = act.getResources().getInteger(android.R.integer.config_shortAnimTime);

        mStatusView.setVisibility(View.VISIBLE);
        mStatusView.animate()
        .setDuration(shortAnimTime)
        .alpha(show ? 1 : 0)
        .setListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                mStatusView.setVisibility(show ? View.VISIBLE : View.GONE);
            }
        });

        mFormView.setVisibility(View.VISIBLE);
        mFormView.animate()
        .setDuration(shortAnimTime)
        .alpha(show ? 0 : 1)
        .setListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                mFormView.setVisibility(show ? View.GONE : View.VISIBLE);
            }
        });
    }

    /**
     * Check the text against a regular expression.
     * @param text
     * @param regex
     * @return true if valid. False otherwise.
     */
    public static boolean checkTextRegex(String text, String regex){
    	Pattern pattern = Pattern.compile(regex);
		Matcher matcher = pattern.matcher(text);
		return matcher.matches();
    }
}

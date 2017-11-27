package com.ti.cc3200smartplug.data;

import android.os.CountDownTimer;

public class AToDHandler {
	
	public interface AtoDTimeoutUpdate{
		public void timeoutUpdate(int packet);
	}

	private static final int COUNT_DOWN_TIME = 3000;
	
	public static final int PACKET_SET_RELAY		= 1;
	public static final int PACKET_SET_THRESHOLD	= 2;
	public static final int PACKET_SET_POWER_SAVING	= 3;
	public static final int PACKET_REQ_24H_AVG		= 4;
	public static final int PACKET_REQ_SWITCH_TABLE	= 5;
	public static final int PACKET_REQ_CALIB		= 6;
	public static final int PACKET_REQ_CLOUD		= 7;
	public static final int PACKET_REQ_DEV_INFO		= 8;
	public static final int PACKET_SET_SWITCH_TABLE	= 9;
	public static final int PACKET_SET_CALIB		= 10;
	public static final int PACKET_SET_EXOSITE_SSL	= 11;
	public static final int PACKET_SET_EXOSITE_CA	= 12;
	
	private CountDownTimer T;
	private boolean idle;
	private AtoDTimeoutUpdate callback;
	private int index;
	
	public AToDHandler(AtoDTimeoutUpdate callback, int index){
		this.callback = callback;
		idle = true;
		this.index = index;
	}
	
	public void startTimer(final boolean enableCallback){
		startTimer(COUNT_DOWN_TIME, enableCallback);
	}
	
	/**
	 * Starts time. Handler is busy until stopTimer() or the callback function is called.
	 */
	public void startTimer(int time, final boolean enableCallback){
		T = new CountDownTimer(time, 1000) {
			public void onTick(long millisUntilFinished) {
				//Do Nothing
			}
			
			public void onFinish() {
				// Usually the time should be stopped by the caller to call stopTimer() if message is received.
				// Not calling stopTimer() would mean that we didn't get the message, thus call the callback function for timeout update.
				if(enableCallback)
					callback.timeoutUpdate(index);
				idle = true;
			}
		}.start();
		idle = false;
	}
	
	/**
	 * Cancel the timer. Handler becomes idle.
	 */
	public void cancelTimer(){
		if(T != null){
			T.cancel();
		}
			
		idle = true;
	}
	
	/**
	 * Handler is idle or not.
	 * @return
	 */
	public boolean isIdle(){
		return idle;
	}
}

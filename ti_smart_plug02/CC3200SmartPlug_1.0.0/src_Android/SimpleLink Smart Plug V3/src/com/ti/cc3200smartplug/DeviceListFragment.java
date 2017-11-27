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

import java.util.ArrayList;

import android.app.Activity;
import android.app.ListFragment;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.view.View;
import android.widget.ListView;

import com.ti.cc3200smartplug.service.SmartPlugDevice;

/**
 * A list fragment representing a list of Devices. This fragment also supports
 * tablet devices by allowing list items to be given an 'activated' state upon
 * selection. This helps indicate which item is currently being viewed in a
 * {@link DeviceDetailFragment}.
 */
public class DeviceListFragment extends ListFragment {

	/**
	 * A callback interface that all activities containing this fragment must
	 * implement. This mechanism allows activities to be notified of item
	 * selections.
	 */
	public interface DeviceListCallbacks {
		/**
		 * Callback method indicating that the item with the given ID was
		 * selected.
		 * <p>
		 * This is called when an item in the ListFragment is selected, then the
		 * Activity needs to insert the proper fragment into
		 * <i>device_detail_container</i>.
		 */
		public void onItemSelected(int position);
	}

	// The main list of all smart plugs.
	private DeviceListArrayAdapter mainListAdapter;

	private final String DEBUG_CLASS = "DeviceListFragment Fragment";

	/**
	 * The serialization (saved instance state) Bundle key representing the
	 * activated item position. Only used on tablets.
	 */
	private static final String STATE_ACTIVATED_POSITION = "activated_position";

	public static final String BROADCAST_DEVICE_LIST_REFRESH = "refresh_sp_list";
	private BroadcastReceiver mRefreshReceiver;

	/**
	 * The fragment's current callback object, which is notified of list item
	 * clicks.
	 */
	private DeviceListCallbacks mCallbacks = sDummyCallbacks;

	/**
	 * The current activated item position. Only used on tablets.
	 */
	private int mActivatedPosition = ListView.INVALID_POSITION;

	/**
	 * A dummy implementation of the {@link DeviceListCallbacks} interface that
	 * does nothing. Used only when this fragment is not attached to an
	 * activity.
	 */
	private static DeviceListCallbacks sDummyCallbacks = new DeviceListCallbacks() {
		@Override
		public void onItemSelected(int position) {
		}
	};

	// ================================= Fragment Lifecycle =================================

	/**
	 * Mandatory empty constructor for the fragment manager to instantiate the
	 * fragment (e.g. upon screen orientation changes).
	 */
	public DeviceListFragment() {
		
	}

	@Override
	public void onAttach(Activity activity) {
		super.onAttach(activity);

		// Activities containing this fragment must implement its callbacks.
		if (!(activity instanceof DeviceListCallbacks)) {
			throw new IllegalStateException("Activity must implement fragment's callbacks.");
		}
		
		if (!(activity instanceof SmartPlugBaseActivity)) {
			throw new IllegalStateException("Activity must be a sub class of SmartPlugBaseActivity");
		}

		mCallbacks = (DeviceListCallbacks) activity;
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		mainListAdapter = new DeviceListArrayAdapter(getActivity(), this, new ArrayList<SmartPlugDevice>());
		
		mRefreshReceiver = new BroadcastReceiver() {
			@Override
			public void onReceive(Context context, Intent intent) {
				Log.w(DEBUG_CLASS, "Got broadcast message, refreshing device list");
				mainListAdapter.notifyDataSetChanged();
			}
		};
		LocalBroadcastManager.getInstance(getActivity()).registerReceiver(mRefreshReceiver, new IntentFilter(BROADCAST_DEVICE_LIST_REFRESH));
	}

	// Never implement this because the default is to create a ListView
	// public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {}

	@Override
	public void onViewCreated(View view, Bundle savedInstanceState) {
		super.onViewCreated(view, savedInstanceState);

		// Restore the previously serialized activated item position.
		if (savedInstanceState != null && savedInstanceState.containsKey(STATE_ACTIVATED_POSITION)) {
			int position = savedInstanceState.getInt(STATE_ACTIVATED_POSITION);
			if (position == ListView.INVALID_POSITION) {
				getListView().setItemChecked(mActivatedPosition, false);
			} else {
				getListView().setItemChecked(position, true);
			}

			mActivatedPosition = position;
		}

		setActivateOnItemClick(true);
	}

	@Override
	public void onResume() {
		super.onResume();
		mainListAdapter.notifyDataSetChanged();
	}

	@Override
	public void onDestroy(){
		LocalBroadcastManager.getInstance(getActivity()).unregisterReceiver(mRefreshReceiver);
		super.onDestroy();
	}
	
	@Override
	public void onDetach() {
		Log.d(DEBUG_CLASS, "[onDetach] Done");

		// Reset the active callbacks interface to the dummy implementation.
		mCallbacks = sDummyCallbacks;

		super.onDetach();
	}

	@Override
	public void onSaveInstanceState(Bundle outState) {
		super.onSaveInstanceState(outState);
		if (mActivatedPosition != ListView.INVALID_POSITION) {
			// Serialize and persist the activated item position.
			outState.putInt(STATE_ACTIVATED_POSITION, mActivatedPosition);
		}
	}

	// ================================= List/Adapter Operation ================================
	
	public void setCustomAdapter(DeviceListArrayAdapter deviceListArrayAdapter){
		mainListAdapter = deviceListArrayAdapter;
		setListAdapter(mainListAdapter);
	}
	
	@Override
	public void onListItemClick(ListView listView, View view, int position, long id) {
		super.onListItemClick(listView, view, position, id);

		// Notify the active callbacks interface (the activity, if the
		// fragment is attached to one) that an item has been selected.
		mCallbacks.onItemSelected(position);
	}

	/**
	 * Turns on activate-on-click mode. When this mode is on, list items will be
	 * given the 'activated' state when touched.
	 */
	public void setActivateOnItemClick(boolean activateOnItemClick) {
		// When setting CHOICE_MODE_SINGLE, ListView will automatically
		// give items the 'activated' state when touched.
		getListView().setChoiceMode(activateOnItemClick ? ListView.CHOICE_MODE_SINGLE : ListView.CHOICE_MODE_NONE);
	}
}

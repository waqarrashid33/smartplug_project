package com.ti.cc3200smartplug;

import java.util.ArrayList;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.ti.cc3200smartplug.service.SmartPlugDevice;

public class DeviceListArrayAdapter extends ArrayAdapter<SmartPlugDevice>{
	
	private Context mContext;
	private LayoutInflater mInflater;
	
	private final static int LIST_VIEW_RESOURCE = R.layout.listview_item_sp;

	public DeviceListArrayAdapter(Context context, DeviceListFragment fragment, ArrayList<SmartPlugDevice> devicePool) {
		super(context, LIST_VIEW_RESOURCE, devicePool);
		mContext = context;
		mInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
	}
	
	@Override
	public View getView(int position, View convertView, ViewGroup parent){
		
		View view;

		/* Reuse the old view, if exists */
        if (convertView == null) {
            view = mInflater.inflate(LIST_VIEW_RESOURCE, parent, false);
        } else {
            view = convertView;
        }

        // Set views
        SmartPlugDevice item = getItem(position);
        ((TextView) view.findViewById(R.id.lvi_sp_name)).setText(item.getName());
        ((TextView) view.findViewById(R.id.lvi_sp_mac_address)).setText(item.getMAC());
        
        ImageView cloud_status = ((ImageView) view.findViewById(R.id.lvi_sp_cloud_status));
        ImageView local_status = ((ImageView) view.findViewById(R.id.lvi_sp_local_status));
        
        short status = item.checkAvailableConnection();
        /* Set cloud connection status icon */
        if((status & SmartPlugDevice.AVAILABLE_CONNECTION_CLOUD) == SmartPlugDevice.AVAILABLE_CONNECTION_CLOUD){
        	cloud_status.setVisibility(View.VISIBLE);
        	if(Common.isLoggedIn(mContext)){
        		if(item.isCloudConnectionLive()){
            		cloud_status.setImageResource(R.drawable.icon_cloud_online);
                }else{
                	cloud_status.setImageResource(R.drawable.icon_cloud_offline);
                }
        	}else{
        		cloud_status.setImageResource(R.drawable.icon_cloud_not_available);
        	}
        }
        /* Set local connection status icon */
        if((status & SmartPlugDevice.AVAILABLE_CONNECTION_LOCAL) == SmartPlugDevice.AVAILABLE_CONNECTION_LOCAL){
        	local_status.setVisibility(View.VISIBLE);
        	if(item.getLocalConnectionStatus() == SmartPlugDevice.LOCAL_CONNECTION_LIVE){
        		local_status.setImageResource(R.drawable.icon_local_online);
            }else{
            	local_status.setImageResource(R.drawable.icon_local_offline);
            }
        }

        return view;
	}
}

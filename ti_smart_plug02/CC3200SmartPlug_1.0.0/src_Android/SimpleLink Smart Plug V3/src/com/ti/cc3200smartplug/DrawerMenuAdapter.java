package com.ti.cc3200smartplug;

import java.util.ArrayList;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

public class DrawerMenuAdapter extends ArrayAdapter<DrawerMenuItem> {

	private LayoutInflater mInflater;
	
	private final static int LIST_VIEW_RESOURCE = R.layout.listview_item_drawer;
	
	public DrawerMenuAdapter(Context context, ArrayList<DrawerMenuItem> items) {
		super(context, LIST_VIEW_RESOURCE, items);
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
        DrawerMenuItem item = getItem(position);
        ((ImageView) view.findViewById(R.id.drawer_item_icon)).setImageResource(item.iconRes);
        
        if(item.text != null){
        	((TextView) view.findViewById(R.id.drawer_item_text)).setText(item.text);
        }else{
        	((TextView) view.findViewById(R.id.drawer_item_text)).setText(item.stringRes);
        }

        return view;
	}
}

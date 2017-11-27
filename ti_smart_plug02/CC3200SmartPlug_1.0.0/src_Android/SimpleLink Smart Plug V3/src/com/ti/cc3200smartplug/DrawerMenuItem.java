package com.ti.cc3200smartplug;

public class DrawerMenuItem {
	
	public interface Action{
		public void execute();
	}
		
	public String text;
	public int stringRes;
	public int iconRes;
	public Action action;
	
	public DrawerMenuItem(int stringRes, int iconRes, Action action){
		text = null;
		this.stringRes = stringRes;
		this.iconRes = iconRes;
		this.action = action;
	}
	
	public DrawerMenuItem(String text, int iconRes, Action action){
		this.text = text;
		stringRes = 0;
		this.iconRes = iconRes;
		this.action = action;
	}
}

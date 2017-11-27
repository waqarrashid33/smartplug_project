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

import org.acra.ACRA;
import org.acra.annotation.ReportsCrashes;
import org.acra.ReportingInteractionMode;

import android.app.Application;

@ReportsCrashes(formKey = "", // will not be used
	mailTo = " ",
	mode = ReportingInteractionMode.TOAST,
	resToastText = R.string.crash_toast_text)
public class SmartPlugApplication extends Application{
	@Override
    public void onCreate() {
        super.onCreate();

        // The following line triggers the initialization of ACRA
        ACRA.init(this);
    }
}

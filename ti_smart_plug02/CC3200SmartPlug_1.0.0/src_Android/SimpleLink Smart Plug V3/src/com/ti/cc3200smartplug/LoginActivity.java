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

package com.ti.cc3200smartplug;

import java.util.Locale;

import org.json.JSONArray;
import org.json.JSONObject;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.exosite.api.ExoCallback;
import com.exosite.api.ExoException;
import com.exosite.api.portals.Portals;
import com.exosite.api.portals.PortalsResponseException;

/**
 * Activity which displays a login screen to the user, offering registration as
 * well.
 */
public class LoginActivity extends Activity {
		
	public static final String BUNDLE_HIDE_SKIP = "show_skip";
	public static final String BUNDLE_START_NEW_LIST_ACTIVITY = "new_activity";
	public static final String BUNDLE_LIST_FRAGMENT_REFRESH_DEVICES = "f_refresh_devices";

	private boolean hideSkip;
	private boolean start_new_list_activity;
	
	enum LoginTask {
		SignIn, SignUp
	}

	/**
	 * Keep track of the login and password recovery tasks so we can cancel them
	 * if requested.
	 */
	private boolean mInProgress;

	// Values for email and password at the time of the login attempt.
	private String mEmail;
	private String mPassword;

	// UI references.
	private EditText mEmailView;
	private EditText mPasswordView;
	private TextView mLoginStatusMessageView;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		// Check if user is signed in or not. If yes, skip this activity.
		if(Common.isLoggedIn(getApplicationContext())){
			//SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(LoginActivity.this);
			//attemptLogin(sharedPreferences.getString(Common.PREFERENCE_EMAIL, "null"), sharedPreferences.getString(Common.PREFERENCE_PASSWORD, "null"));
			startActivity(new Intent(LoginActivity.this, DeviceListActivity.class));
			Portals.setDomain(Portals.PORTALS_DOMAIN);
			finish();
		}
		
		getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);

		setContentView(R.layout.activity_login);
		
		findViewById(R.id.status_ref).setVisibility(View.INVISIBLE);
		
		hideSkip = false;
		start_new_list_activity = true;

		// Set up the login form.
		SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(LoginActivity.this);
		mEmail = sharedPreferences.getString(Common.PREFERENCE_EMAIL, "");
		mEmailView = (EditText) findViewById(R.id.email);
		mEmailView.setText(mEmail);
		mEmailView.setOnEditorActionListener(new TextView.OnEditorActionListener() {
			@Override
			public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
				if (actionId == EditorInfo.IME_ACTION_NEXT) {
					mPasswordView.requestFocus();
					return true;
				}
				return false;
			}
		});
		
		mPasswordView = (EditText) findViewById(R.id.password);
		
		mPassword = sharedPreferences.getString(Common.PREFERENCE_PASSWORD, "");
		mPasswordView.setText(mPassword);
		
		mPasswordView.setOnEditorActionListener(new TextView.OnEditorActionListener() {
			@Override
			public boolean onEditorAction(TextView textView, int actionId, KeyEvent keyEvent) {
				if(checkCredentials())
					attemptLogin(mEmailView.getText().toString(), mPasswordView.getText().toString());
				return true;
			}
		});

		mLoginStatusMessageView = (TextView) findViewById(R.id.login_status_message);

		/* Sign In Button */
		findViewById(R.id.sign_in_button).setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View view) {
				if(checkCredentials())
					attemptLogin(mEmailView.getText().toString(), mPasswordView.getText().toString());
			}
		});

		/* Forget Password Button */
		findViewById(R.id.button_forget_password).setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				forgetPasswordAction();
			}
		});

		/* Sign Up Button */
		findViewById(R.id.button_sign_up).setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				signUpAction();
			}
		});

		// Skip button
		View skipButton = findViewById(R.id.button_local_connection);
		skipButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				skipLoginAction();
			}
		});
		
		// Make the skip button invisible if the caller activity requested to do so.
		Bundle extras = getIntent().getExtras();
		if (extras != null) {
		   hideSkip = extras.getBoolean(BUNDLE_HIDE_SKIP, false);
		   if (hideSkip)
			   skipButton.setVisibility(View.GONE);
		   
		   start_new_list_activity = extras.getBoolean(BUNDLE_START_NEW_LIST_ACTIVITY, true);
		}
		
//		mEmailView.setText("vlin@ti.com");
//		mPasswordView.setText("a0221015*");
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		menu.clear();
		return true;
	}

	private void forgetPasswordAction() {
		// Show a progress spinner, and kick off a background task to
		// perform the password recovery email attempt.
		mLoginStatusMessageView.setText(R.string.login_progress_recovering_password);

		// Store value at the time of the reset attempt.
		mEmail = mEmailView.getText().toString();

		Common.showProgressForForm(this, true);
		Portals.resetPasswordInBackground(mEmail, new ExoCallback<Void>() {
			@Override
			public void done(Void result, ExoException e) {
				mInProgress = false;
				Common.showProgressForForm(LoginActivity.this, false);
				if (result != null) {
					Toast.makeText(getApplicationContext(), String.format("Password recovery email sent to %s", mEmail), Toast.LENGTH_LONG).show();
				} else {
					reportExoException(e);
				}
			}
		});
	}

	private void signUpAction() {
		// Show a progress spinner
		mLoginStatusMessageView.setText(R.string.login_progress_signing_up);

		// Store value at the time of the reset attempt.
		mEmail = mEmailView.getText().toString();
		mPassword = mPasswordView.getText().toString();

		Common.showProgressForForm(this, true);
		Portals.signUpInBackground(mEmail, mPassword, Portals.PLAN_ID, new ExoCallback<JSONObject>() {
			@Override
			public void done(JSONObject result, ExoException e) {
				mInProgress = false;
				Common.showProgressForForm(LoginActivity.this, false);
				if (result != null) {
					Toast.makeText(getApplicationContext(), String.format("Sent confirmation email to %s", mEmail), Toast.LENGTH_LONG).show();
				} else {
					reportExoException(e);
				}
			}
		});

		Common.showProgressForForm(LoginActivity.this, false);
	}

	private void skipLoginAction() {
		// Set the logged in status
		Common.setLoggedIn(this, getApplicationContext(), false);
		Intent intent = new Intent(LoginActivity.this, DeviceListActivity.class);
		startActivity(intent);
		finish();
	}

	public void reportExoException(Exception e) {
		
		String message = null;
		
		if (e instanceof PortalsResponseException) {
			PortalsResponseException pre = (PortalsResponseException) e;
			int code = pre.getResponseCode();
			if (code == 401) {
				message = getString(R.string.error_unauthorized);
			} else {
				message = String.format(Locale.ENGLISH, "Error: %s (%d)", pre.getMessage(), code);
			}
		} else {
			message = String.format(Locale.ENGLISH, "Unexpected error: %s", e.getMessage());
		}
		
		final AlertDialog dialog = new AlertDialog.Builder(this).create();
		dialog.setTitle("Communication Error");
		dialog.setMessage(message);
		dialog.setButton(DialogInterface.BUTTON_NEUTRAL, "OK", (DialogInterface.OnClickListener)null);
		dialog.show();
	}

	/**
	 * If there are form errors (invalid email, missing fields, etc.), the
	 * errors are presented and no actual login attempt is made.
	 * @return
	 */
	private boolean checkCredentials(){
		// Remove keyboard
		//getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);
		InputMethodManager mgr = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
		mgr.hideSoftInputFromWindow(getCurrentFocus().getWindowToken(), 0);

		// Reset errors.
		mEmailView.setError(null);
		mPasswordView.setError(null);

		// Store values at the time of the login attempt.
		mEmail = mEmailView.getText().toString();
		mPassword = mPasswordView.getText().toString();

		boolean cancel = false;
		View focusView = null;

		// Check for a valid password.
		if (TextUtils.isEmpty(mPassword)) {
			mPasswordView.setError(getString(R.string.error_field_required));
			focusView = mPasswordView;
			cancel = true;
		} else if (mPassword.length() < 6) {
			mPasswordView.setError(getString(R.string.error_invalid_password));
			focusView = mPasswordView;
			cancel = true;
		}

		// Check for a valid email address.
		if (TextUtils.isEmpty(mEmail)) {
			mEmailView.setError(getString(R.string.error_field_required));
			focusView = mEmailView;
			cancel = true;
		} else if (!mEmail.contains("@")) {
			mEmailView.setError(getString(R.string.error_invalid_email));
			focusView = mEmailView;
			cancel = true;
		}

		if (cancel) {
			// There was an error; don't attempt login and focus the first
			// form field with an error.
			focusView.requestFocus();
		} else {
			return true;
		}
		return false;
	}
	
	/**
	 * Usually needs to call @checkCredentials() before calling this method to make sure all information are correct.
	 * Attempts to sign in or register the account specified by the login credentials.
	 * Once signed in, the Activity will bedestroyed and DevceListActivity will be created.
	 */
	public void attemptLogin(final String email, final String password) {
		if (mInProgress) {
			return;
		}
		
		// Show a progress spinner, and kick off a background task to
		// perform the user login attempt.
		mLoginStatusMessageView.setText(R.string.login_progress_signing_in);
		Common.showProgressForForm(this, true);
		Portals.setDomain(Portals.PORTALS_DOMAIN);
		mInProgress = true;
		Portals.listPortalsInBackground(email, password, new ExoCallback<JSONArray>() {
			@Override
			public void done(JSONArray result, ExoException e) {
				mInProgress = false;
				if (result != null) {
					SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
					sharedPreferences.edit().putString(Common.PREFERENCE_EMAIL, email).commit();
					sharedPreferences.edit().putString(Common.PREFERENCE_PASSWORD, password).commit();
					sharedPreferences.edit().putString("portal_list", result.toString()).commit();
					
					// Set the logged in status
					Common.setLoggedIn(LoginActivity.this, getApplicationContext(), true);

					if(start_new_list_activity){
						Intent intent = new Intent(LoginActivity.this, DeviceListActivity.class);
						intent.putExtra(BUNDLE_LIST_FRAGMENT_REFRESH_DEVICES, true);
						startActivity(intent);
					}
					finish();
				} else {
					Common.showProgressForForm(LoginActivity.this, false);
					reportExoException(e);
				}
			}
		});
	}
}

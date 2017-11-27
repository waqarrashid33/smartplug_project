/*****************************************************************************
*
*  Copyright (C) 2012 Exosite LLC
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Exosite LLC nor the names of its contributors may
*    be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*****************************************************************************/

package com.exosite.api.portals;

import java.io.BufferedReader;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;

import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLHandshakeException;

import android.util.Base64;

import com.exosite.api.ExoCall;
import com.exosite.api.ExoException;
import com.exosite.api.ExoCallback;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Provides access to the Portals API
 */
public class Portals {

	/**
	 * Exosite Portals plan identifier for new users.
	 */
	public static final String PLAN_ID = "3676938388";

	/**
	 * Domain for interacting with Portals API
	 */
	public static final String PORTALS_DOMAIN = "ti.exosite.com";

	static String mDomain = "portals.exosite.com";
	static int mTimeoutSeconds = 15;
	
	public static final String HTTP_GET		= "GET";
	public static final String HTTP_POST	= "POST";
	public static final String HTTP_PUT		= "PUT";
	public static final String HTTP_DELETE	= "DELETE";

	/**
	 * @return Portals domain configured for communication
	 */
	public static String getDomain() {
		return mDomain;
	}

	/**
	 * @param domain
	 *            Portals domain configured for API communication
	 */
	public static void setDomain(String domain) {
		mDomain = domain;
	}

	/**
	 * @return HTTP timeout for calls to Portals API
	 */
	public static int getTimeoutSeconds() {
		return mTimeoutSeconds;
	}

	/**
	 * @param timeoutSeconds
	 *            HTTP timeout for calls to Portals API
	 */
	public static void setTimeoutSeconds(int timeoutSeconds) {
		mTimeoutSeconds = timeoutSeconds;
	}

	private Portals() {
	}

	/**
	 * List a user's portals
	 *
	 * @param email
	 *            - email address of user to authenticate
	 * @param password
	 *            - portals password of user
	 * @return JSONArray of JSONObjects containing information about user's
	 *         managed and owned portals.
	 * @throws PortalsRequestException
	 * @throws PortalsResponseException
	 */
	static public JSONArray listPortals(String email, String password) throws PortalsRequestException, PortalsResponseException {
		JSONArray response = null;
		HTTPResult r = call(HTTP_GET, mDomain, "portal/", "", email, password);
		String responseBody = r.responseBody;
		try {
			response = new JSONArray(responseBody);
		} catch (JSONException e) {
			throw new PortalsRequestException("Invalid JSON in response. Response was:" + responseBody);
		}
		return response;
	}

	/**
	 * List a user's portals
	 *
	 * @param email
	 *            - email address of user to authenticate
	 * @param password
	 *            - portals password of user
	 * @param callback
	 *            - handler for successful completion or exception condition
	 * @return JSONArray of JSONObjects containing information about user's
	 *         managed and owned portals.
	 * @throws PortalsRequestException
	 * @throws PortalsResponseException
	 */
	static public void listPortalsInBackground(final String email, final String password, final ExoCallback<JSONArray> callback) {

		new ExoCall<JSONArray>(callback) {
			@Override
			public JSONArray call() throws ExoException {
				return listPortals(email, password);
			}
		}.callInBackground();
	}
	
	/**
	 * TI addition
	 * @param path
	 *            - e.g. user/password
	 * @param body
	 *            - e.g. {"action":"reset", "email":"johndoe@gmail555.com"}
	 * @param email
	 *            - email to use for authentication, or null for no
	 *            authentication
	 * @param password
	 *            - password to use for authentication, or null for no
	 *            authentication
	 * @param callback
	 *            - handler for successful completion or exception condition
	 * @throws PortalsRequestException
	 * @throws PortalsResponseException
	 */
	static public void callInBackground(final String request, final String path, final String body, final String email, final String password, final ExoCallback<HTTPResult> callback) {
		new ExoCall<HTTPResult>(callback) {
			@Override
			public HTTPResult call() throws ExoException {				
				return Portals.call(request, mDomain, path, body, email, password);
			}
		}.callInBackground();
	}

	/**
	 * Generate an email to a Portals user to reset their password.
	 *
	 * @param email
	 *            - email address to reset
	 * @throws PortalsRequestException
	 * @throws PortalsResponseException
	 */
	static public void resetPassword(final String email) throws PortalsRequestException, PortalsResponseException {
		call(HTTP_POST, mDomain, "user/password", String.format("{\"email\":\"%s\",\"action\":\"reset\"}", email));
	}

	/**
	 * Generate an email to a Portals user to reset their password.
	 *
	 * @param email
	 *            - email address to reset
	 * @param callback
	 *            - handler for successful completion or exception condition
	 * @throws PortalsRequestException
	 * @throws PortalsResponseException
	 */
	static public void resetPasswordInBackground(final String email, final ExoCallback<Void> callback) {
		new ExoCall<Void>(callback) {
			@Override
			public Void call() throws ExoException {
				resetPassword(email);
				return null;
			}
		}.callInBackground();
	}

	/**
	 * Sign up a new Portals user on the configured domain. Since first and last
	 * name are not specified, Portals will set them to "New" and "User",
	 * respectively.
	 *
	 * @param email
	 *            - email address of user. This may not already exist as a user
	 *            in Portals.
	 * @param password
	 *            - password for user. UIs may want to confirm password.
	 * @param plan
	 *            - Portals plan identifier for new user.
	 * @throws PortalsRequestException
	 * @throws PortalsResponseException
	 */
	static public JSONObject signUp(final String email, final String password, final String plan) throws PortalsRequestException, PortalsResponseException {
		return signUp(email, password, plan, "", "");
	}
	
	/**
	 * Sign up a new Portals user on the configured domain.
	 *
	 * @param email
	 *            - email address of new user. This may not already be
	 *            registered in Portals.
	 * @param password
	 *            - password for user. UIs may want to confirm password.
	 * @param plan
	 *            - Portals plan identifier for new user.
	 * @param firstName
	 *            - first name of new user
	 * @param lastName
	 *            - last name of new user
	 * @throws PortalsRequestException
	 * @throws PortalsResponseException
	 */
	static public JSONObject signUp(String email, String password, String plan, String firstName, String lastName) throws PortalsRequestException,
			PortalsResponseException {
		HTTPResult r = call(
				HTTP_POST,
				mDomain,
				"user",
				String.format("{\"email\":\"%s\",\"password\":\"%s\",\"plan\":\"%s\",\"first_name\":\"%s\",\"last_name\":\"%s\"}",email, password, plan, firstName, lastName));
		JSONObject response;
		String responseBody = r.responseBody;
		try {
			response = new JSONObject(responseBody);
		} catch (JSONException e) {
			throw new PortalsRequestException("Invalid JSON in response. Response was: " + responseBody);
		}
		return response;
	}

	/**
	 * Sign up a new Portals user on the configured domain. Since first and last
	 * name are not specified, Portals will set them to "New" and "User",
	 * respectively.
	 *
	 * @param email
	 *            - email address of user. This may not already exist as a user
	 *            in Portals.
	 * @param password
	 *            - password for user. UIs may want to confirm password.
	 * @param plan
	 *            - Portals plan identifier for new user.
	 * @param callback
	 *            - handler for successful completion or exception condition
	 * @throws PortalsRequestException
	 * @throws PortalsResponseException
	 */
	static public void signUpInBackground(final String email, final String password, final String plan, final ExoCallback<JSONObject> callback) {
		signUpInBackground(email, password, plan, "", "", callback);
	}

	/**
	 * Sign up a new Portals user on the configured domain.
	 *
	 * @param email
	 *            - email address of new user. This may not already be
	 *            registered in Portals.
	 * @param password
	 *            - password for user. UIs may want to confirm password.
	 * @param plan
	 *            - Portals plan identifier for new user.
	 * @param firstName
	 *            - first name of new user
	 * @param lastName
	 *            - last name of new user
	 * @param callback
	 *            - handler for successful completion or exception condition
	 * @throws PortalsRequestException
	 * @throws PortalsResponseException
	 */
	static public void signUpInBackground(final String email, final String password, final String plan, final String firstName, final String lastName,
			final ExoCallback<JSONObject> callback) {

		new ExoCall<JSONObject>(callback) {
			@Override
			public JSONObject call() throws ExoException {
				return signUp(email, password, plan, firstName, lastName);
			}
		}.callInBackground();
	}

	/**
	 * Add a new device to a user's portal.
	 *
	 * @param portalRID
	 *            - identifier of portal where device should be added
	 * @param vendor
	 *            - vendor identifier
	 * @param model
	 *            - model identifier
	 * @param sn
	 *            - serial number. This must match one of the sets of valid
	 *            serial numbers configured for the model.
	 * @param name
	 *            - name of device
	 * @param email
	 *            - email address of user to authenticate
	 * @param password
	 *            - portals password of user
	 * @return JSONObject containing "rid" - RID of the new device and "cik" -
	 *         client key for new device
	 * @throws PortalsRequestException
	 * @throws PortalsResponseException
	 */
	static public JSONObject addDevice(final String portalRID,
			final String vendor,
			final String model,
			final String sn,
			final String name,
			final String email,
			final String password) throws PortalsRequestException, PortalsResponseException {
		HTTPResult r = call(HTTP_POST, mDomain, "device", String.format(
				"{\"portal_rid\":\"%s\",\"vendor\":\"%s\",\"model\":\"%s\",\"serialnumber\":\"%s\",\"name\":\"%s\"}", portalRID, vendor, model, sn, name),
				email, password);
		JSONObject response;
		String responseBody = r.responseBody;
		try {
			response = new JSONObject(responseBody);
		} catch (JSONException e) {
			throw new PortalsRequestException("Invalid JSON in response. Response was: " + responseBody);
		}
		return response;
	}

	/**
	 * Add a new device to a user's portal.
	 *
	 * @param portalRID
	 *            - identifier of portal where device should be added
	 * @param vendor
	 *            - vendor identifier
	 * @param model
	 *            - model identifier
	 * @param sn
	 *            - serial number. This must match one of the sets of valid
	 *            serial numbers configured for the model.
	 * @param name
	 *            - name of device
	 * @param email
	 *            - email address of user to authenticate
	 * @param password
	 *            - portals password of user
	 * @param callback
	 *            - handler for successful completion or exception condition
	 * @return JSONObject containing "rid" - RID of the new device and "cik" -
	 *         client key for new device
	 * @throws PortalsRequestException
	 * @throws PortalsResponseException
	 */
	static public void addDeviceInBackground(final String portalRID, final String vendor, final String model, final String sn, final String name,
			final String email, final String password, final ExoCallback<JSONObject> callback) {
		new ExoCall<JSONObject>(callback) {
			@Override
			public JSONObject call() throws ExoException {
				return addDevice(portalRID, vendor, model, sn, name, email, password);
			}
		}.callInBackground();
	}

	/**
	 * Makes a call to the the Portals API with no authentication.
	 * 
	 * @param HTTP request type
	 *            - {@link #HTTP_GET}, {@link# HTTP_POST}, or {@link #HTTP_PUT}
	 * @param domain
	 *            - e.g. portals.exosite.com
	 * @param path
	 *            - e.g. user/password
	 * @param body
	 *            - e.g. {"action":"reset", "email":"johndoe@gmail555.com"}
	 * @return result of HTTP call
	 * @throws PortalsRequestException
	 *             if an exception was thrown while making the request
	 * @throws PortalsResponseException
	 *             if the response status was >=300 or an exception was thrown
	 *             while reading the response
	 */
	public static HTTPResult call(String request, String domain, String path, String body) throws PortalsRequestException, PortalsResponseException {
		return call(request, domain, path, body, null, null);
	}

	/**
	 * Makes a call to the the Portals API.
	 * 
	 * @param HTTP request type
	 *            - {@link #HTTP_GET}, {@link# HTTP_POST}, or {@link #HTTP_PUT}
	 * @param domain
	 *            - e.g. portals.exosite.com
	 * @param path
	 *            - e.g. user/password
	 * @param body
	 *            - e.g. {"action":"reset", "email":"johndoe@gmail555.com"}
	 * @param email
	 *            - email to use for authentication, or null for no
	 *            authentication
	 * @param password
	 *            - password to use for authentication, or null for no
	 *            authentication
	 * @return result of HTTP call
	 * @throws PortalsRequestException
	 *             if an exception was thrown while making the request
	 * @throws PortalsResponseException
	 *             if the response status was >=300 or an exception was thrown
	 *             while reading the response
	 */
	public static HTTPResult call(String request, String domain, String path, String body, String email, String password) throws PortalsRequestException, PortalsResponseException {
		URL url = null;
		HttpsURLConnection conn = null;
		OutputStreamWriter writer = null;
		HTTPResult result = new HTTPResult();
		StringBuffer response = new StringBuffer();
		try {
			url = new URL("https://" + domain + "/api/portals/v1/" + path);
		} catch (MalformedURLException ex) {
			throw new PortalsRequestException("Malformed URL.");
		}
		try {
			// Construct the HttpURLConnection connection object
			try {
				conn = (HttpsURLConnection) url.openConnection();
			} catch (MalformedURLException ex) {
				throw new IOException("Cannot connect to Network");
			}
			if (email != null && password != null) {
				String encoded = Base64.encodeToString((email + ":" + password).getBytes(), Base64.NO_WRAP);
				conn.setRequestProperty("Authorization", "Basic " + encoded);
			}
			conn.setUseCaches(false);
			conn.setConnectTimeout(Portals.mTimeoutSeconds * 1000);
			conn.setRequestProperty("User-Agent", "Android demo app");
			conn.setRequestMethod(request);
			
			if (request.equals(HTTP_POST) || request.equals(HTTP_PUT)) {	//body.length() > 0
				conn.setDoOutput(true);
				conn.setRequestProperty("Content-Length", "" + body.length());
				conn.setRequestProperty("Content-Type", "application/json; charset=utf-8");
				conn.connect();

				try {
					writer = new OutputStreamWriter(conn.getOutputStream());
					writer.write(body);
					writer.flush();
				} catch (IOException e) {
					throw new PortalsRequestException("IOException writing http request.");
				} finally {
					if (null != writer)
						writer.close();
				}
			} else if(request.equals(HTTP_GET)) {
				conn.connect();
			} else if(request.equals(HTTP_DELETE)) {
				conn.connect();
			} else {
				throw new PortalsRequestException("Invalid HTTP Request.");
			}
			
			// Retrieve response if using {@link #HTTP_GET} or {@link# HTTP_POST}
			result.responseCode = conn.getResponseCode();
			if (result.responseCode == 401) {
				throw new PortalsResponseException("username or password is invalid", conn.getResponseCode(), "");
			}
			InputStreamReader isr;
			if (result.responseCode >= 300) {
				isr = new InputStreamReader(conn.getErrorStream());
			} else {
				isr = new InputStreamReader(conn.getInputStream());
			}
			BufferedReader reader = null;
			try {
				reader = new BufferedReader(isr);
				String line;
				while ((line = reader.readLine()) != null) {
					response.append(line);
				}
			} catch (IOException e) {
				throw new PortalsResponseException("Error reading response", conn.getResponseCode(), "");
			} finally {
				if (null != reader)
					reader.close();
			}

			if (result.responseCode >= 300) {
				result.responseBody = response.toString();
				try{
					JSONObject obj = new JSONObject(result.responseBody);
					result.responseBody = obj.getString("notices");
				} catch (JSONException e){
					// Nothing
				}
				
				throw new PortalsResponseException(result.responseBody, result.responseCode, response.toString());
			}

		} catch (SSLHandshakeException e) {
			throw new PortalsRequestException("SSLHandshakeException");
		} catch (EOFException e) {
			throw new PortalsRequestException("Failed to sign in. Please check your credentials or create one.");
		} catch (IOException e) {
			throw new PortalsRequestException(e.getMessage());
		} finally {
			if (conn != null)
				conn.disconnect();
		}
		result.responseBody = response.toString();
		return result;
	}
}

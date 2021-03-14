package com.torgtek.matidsms;

import android.app.IntentService;
import android.content.Intent;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;
import android.os.Debug;
import android.os.SystemClock;
import android.util.Log;
import android.widget.Toast;

import com.google.gson.Gson;
import com.torgtek.matidsms.ui.login.GPSTracker;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

/**
 * An {@link IntentService} subclass for handling asynchronous task requests in
 * a service on a separate handler thread.
 * <p>
 * TODO: Customize class - update intent actions and extra parameters.
 */
public class BackgroundService extends IntentService {
    public static String personName;
    public static String personGivenName;
    public static String personFamilyName;
    public static String personEmail;
    public static String personId;
    public static Uri personPhoto;
    public  static Boolean inapp2=false;

    // TODO: Rename actions, choose action names that describe tasks that this
    // IntentService can perform, e.g. ACTION_FETCH_NEW_ITEMS
    public static final String ACTION_FOO = "com.torgtek.matidsms.action.FOO";
    public static final String ACTION_BAZ = "com.torgtek.matidsms.action.BAZ";

    // TODO: Rename parameters
    public static final String EXTRA_PARAM1 = "com.torgtek.matidsms.extra.PARAM1";
    public static final String EXTRA_PARAM2 = "com.torgtek.matidsms.extra.PARAM2";

    public BackgroundService() {
        super("BackgroundService");
    }
    GPSTracker gps;

    @Override
    protected void onHandleIntent(Intent intent) {

        List<Map<String,Object>> jsonlist=new ArrayList<>();
        SQLiteDatabase mydatabase = null;
        Debug.waitForDebugger();
        while(true) {
            double latitude=0;
            double longitude=0;
            Log.d("BackgroundService", "ENTRY");
            try {
//                SystemClock.sleep(1000);
                mydatabase = openOrCreateDatabase("/data/data/com.torgtek.matidsms/databases/dsms.db",MODE_PRIVATE, null);

                Cursor resultSet = mydatabase.rawQuery("Select * from `LOGS` where processed =0 limit 100;", null);
                Map<String, Object> params_user = new HashMap<String, Object>();
                Map<Double, Double> coordinates = new HashMap<Double, Double>();

                params_user.put("personName",personName);
                params_user.put("personGivenName",personGivenName);
                params_user.put("personFamilyName",personFamilyName);
                params_user.put("personEmail",personEmail);
                params_user.put("personId",personId);
                Log.d("BackgroundService", "Size of list");
                Log.d("BackgroundService", String.valueOf(resultSet.getCount()));
                gps = new GPSTracker(this);

                // Check if GPS enabled
                if(gps.canGetLocation()) {

                     latitude = gps.getLatitude();
                     longitude = gps.getLongitude();

                    // \n is for new line
                    Toast.makeText(getApplicationContext(), "Your Location is - \nLat: " + latitude + "\nLong: " + longitude, Toast.LENGTH_LONG).show();
                }
                if (resultSet.moveToFirst()) {
                    do {
                        Map<String, Object> params = new HashMap<String, Object>();
                        // Passing values

                        String columnFRAME = resultSet.getString(resultSet.getColumnIndex("FRAME"));
                        try {
                            Integer temp = Integer.parseInt(columnFRAME);
                            if (temp != 0) {
                                params.put("frame", temp);
                            }
                        } catch (Exception ex) {
                            continue;
                        }
                        String columnSLEEPY = resultSet.getString(resultSet.getColumnIndex("SLEEPY"));
                        try {
                            Integer temp = Integer.parseInt(columnSLEEPY);

                            if (temp != 0) {
                                params.put("sleepy", temp);
                            }
                        } catch (Exception ex) {

                        }
                        String columnDROWSY = resultSet.getString(resultSet.getColumnIndex("DROWSY"));
                        try {
                            Integer temp = Integer.parseInt(columnDROWSY);

                            if (temp != 0) {
                                params.put("drowsy", temp);
                            }
                        } catch (Exception ex) {

                        }
                        String columnDISTRACTED = resultSet.getString(resultSet.getColumnIndex("DISTRACTED"));
                        try {
                            Integer temp = Integer.parseInt(columnDISTRACTED);

                            if (temp != 0) {
                                params.put("distracted", temp);
                            }
                        } catch (Exception ex) {

                        }
                        String columnALARMS = resultSet.getString(resultSet.getColumnIndex("ALARMS"));
                        try {
                            Integer temp = Integer.parseInt(columnALARMS);

                            if (temp != 0) {
                                params.put("alarm", temp);
                            }

                        } catch (Exception ex) {

                        }
                        String columnINTERVAL = resultSet.getString(resultSet.getColumnIndex("INTERVAL"));
                        try {
                            Float temp = Float.parseFloat(columnINTERVAL);

                            if (temp != 0) {
                                params.put("interval", temp);
                            }

                        } catch (Exception ex) {

                        }
                        String columnAVGEAR = resultSet.getString(resultSet.getColumnIndex("AVGEAR"));
                        try {
                            Float temp = Float.parseFloat(columnAVGEAR);

                            if (temp != 0) {
                                params.put("avgear", temp);
                            }

                        } catch (Exception ex) {

                        }
                        String columnAVGMAR = resultSet.getString(resultSet.getColumnIndex("AVGMAR"));
                        try {
                            Float temp = Float.parseFloat(columnAVGMAR);

                            if (temp != 0) {
                                params.put("avgmar", temp);
                            }

                        } catch (Exception ex) {

                        }
                        String columnAVGDEV = resultSet.getString(resultSet.getColumnIndex("AVGDEV"));
                        try {
                            Float temp = Float.parseFloat(columnAVGDEV);

                            if (temp != 0) {
                                params.put("avgdev", temp);
                            }

                        } catch (Exception ex) {

                        }
                        String columnSPEED = resultSet.getString(resultSet.getColumnIndex("SPEED"));
                        try {
                            Float temp = Float.parseFloat(columnSPEED);

                            if (temp != 0) {
                                params.put("speed", temp);
                            }

                        } catch (Exception ex) {

                        }
                        String columnDIST = resultSet.getString(resultSet.getColumnIndex("DIST"));
                        try {
                            Float temp = Float.parseFloat(columnDIST);

                            if (temp != 0) {
                                params.put("dist", temp);
                            }

                        } catch (Exception ex) {

                        }
                        String columnAX = resultSet.getString(resultSet.getColumnIndex("AX"));
                        try {
                            Float temp = Float.parseFloat(columnAX);

                            if (temp != 0) {
                                params.put("ax", temp);
                            }

                        } catch (Exception ex) {

                        }
                        String columnAY = resultSet.getString(resultSet.getColumnIndex("AY"));
                        try {
                            Float temp = Float.parseFloat(columnAY);

                            if (temp != 0) {
                                params.put("ay", temp);
                            }

                        } catch (Exception ex) {

                        }
                        String columnAZ = resultSet.getString(resultSet.getColumnIndex("AZ"));
                        try {
                            Float temp = Float.parseFloat(columnAZ);

                            if (temp != 0) {
                                params.put("az", temp);
                            }

                        } catch (Exception ex) {

                        }
                        String columnTS = resultSet.getString(resultSet.getColumnIndex("TS"));
                        try {
                            Integer temp = Integer.parseInt(columnTS);

                            if (temp != 0) {
                                params.put("ts", temp);
                            }

                        } catch (Exception ex) {

                        }
                        jsonlist.add(params);
                        // Do something Here with values
                    } while (resultSet.moveToNext());
                }

                Map<String,Object> finallist=new HashMap<>();
                finallist.put("user",params_user);
                finallist.put("data",jsonlist);
                finallist.put("coordinates",coordinates);

                Gson gson = new Gson();
                String kx = gson.toJson(finallist);
                OkHttpClient client = new OkHttpClient.Builder()
                        .readTimeout(100, TimeUnit.SECONDS)
                        .writeTimeout(100, TimeUnit.SECONDS)
                        .build();


                MediaType JSON
                        = MediaType.parse("application/json; charset=utf-8");
                RequestBody body = RequestBody.create(kx, JSON);
                Request request = new Request.Builder()
                        .url("http://api.torgtek.com:2095/log/")
                        .post(body)

                        .build();

                Response response = client.newCall(request).execute();
                if (response.code() == 200) {
                    String xs = response.body().string();
                    //idhar apun ko sqlite update karna hai ki data send ho gaya hai

                    for (int i = 0; i < jsonlist.size(); i++) {
                        HashMap<String, Object> dd = (HashMap<String, Object>) jsonlist.get(i);
                        try {
                            System.out.println(dd);
                            Integer frame = (Integer) dd.get("frame");
                            String sql = "DELETE FROM LOGS where FRAME=" + String.valueOf(frame);
                            mydatabase.execSQL(sql);

                        } catch (Exception ex) {
                            System.out.println(ex.getMessage());
                        }
                    }
                }

            } catch (Exception e) {
                // database doesn't exist yet.
                Object a = "";
                System.out.println(e.getMessage());

                continue;
            } finally {
                mydatabase.close();
            }
        }
    }

    /**
     * Handle action Foo in the provided background thread with the provided
     * parameters.
     */
    private void handleActionFoo(String param1, String param2) {
        // TODO: Handle action Foo
        throw new UnsupportedOperationException("Not yet implemented");
    }

    /**
     * Handle action Baz in the provided background thread with the provided
     * parameters.
     */
    private void handleActionBaz(String param1, String param2) {
        // TODO: Handle action Baz
        throw new UnsupportedOperationException("Not yet implemented");
    }
}
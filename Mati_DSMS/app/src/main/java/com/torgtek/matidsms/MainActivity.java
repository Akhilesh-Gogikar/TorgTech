package com.torgtek.matidsms;

import android.Manifest;
import android.app.AlertDialog;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.os.Build;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
//import android.support.v4.content.ContextCompat;
//import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.Html;
import android.text.format.DateUtils;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.CustomListViewAdapter;
import com.RowItem;
import com.google.android.gms.common.util.DataUtils;
import com.google.gson.Gson;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.torgtek.matidsms.R;
import com.torgtek.matidsms.ui.login.GPSTracker;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Dictionary;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import pub.devrel.easypermissions.EasyPermissions;


public class MainActivity extends AppCompatActivity  {
    public static Long current_id;
    private static final String[] LOCATION_AND_CAMERA =
            {Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.CAMERA};
    private static final String TAG = "EasyPermissions";
    private static final int RC_CAMERA_PERM = 123;
    private static final int RC_LOCATION_CONTACTS_PERM = 124;
    private static final int RC_CAMERA_AND_LOCATION =111 ;
    private static final int PERMISSION_REQUEST_CODE = 12;
    private static final int PERMISSION_REQUEST_CODE_GPS = 13;
    private static final int PERMISSION_REQUEST_CODE_STORAGE = 14;
    private static final int PERMISSION_REQUEST_CODE_INTERNET = 15;
    GPSTracker gps;
    static boolean inapp=false;
    private int requestCode;
//    public static final String[] titles = new String[] { "Strawberry",
//            "Banana", "Orange", "Mixed" };
            String[] titles ;

    public static  String[] descriptions ;

    public static  Integer[] images = { R.drawable.straw,
             };
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        final Button startbutton = findViewById(R.id.startRidebutton);
        String last=getIntent().getStringExtra("last_activity");
        SQLiteDatabase mydatabase = openOrCreateDatabase("/data/data/com.torgtek.matidsms/databases/dsms.db", MODE_PRIVATE, null);
        try {
            Cursor c = mydatabase.rawQuery("SELECT start_location,start_ts,end_location,end_ts FROM gpslogs", null);
            if (c.moveToLast()) {
//            c.getCount();
                titles = new String[c.getCount()];
                descriptions = new String[c.getCount()];
                Integer cc = 0;
                do {
                    titles[cc] = c.getString(0)+" - "+c.getString(2);
                    descriptions[cc] = "Started "+ DateUtils.getRelativeTimeSpanString(Long.parseLong(c.getString(1)) * 1000).toString()+ " @ "+DateUtils.formatDateTime(MainActivity.this,
                            Long.parseLong(c.getString(1)) * 1000,3);
                    cc++;
                } while (c.moveToPrevious());
            }
            final ListView listview = (ListView) findViewById(R.id.pastrideslist);
            ArrayList<RowItem> rowItems = new ArrayList<RowItem>();
            for (int i = 0; i < titles.length; i++) {
                RowItem item = new RowItem(titles[i], descriptions[i]);
                rowItems.add(item);
            }


            CustomListViewAdapter adapter = new CustomListViewAdapter(this,
                    R.layout.list_item, rowItems);
            listview.setAdapter(adapter);
        }
        catch (Exception px){
            px.printStackTrace();
        }

        if (checkPermission()) {



        } else {
            requestPermission();
        }



        startbutton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                if (checkPermission()) {
                    startbutton.setEnabled(false);
                    startbutton.setText("DISCLAIMER / LIABILITY");

//                    SQLiteDatabase mydatabase = openOrCreateDatabase("/data/data/com.torgtek.matidsms/databases/dsms.db", MODE_PRIVATE, null);
//                    try {
//                        mydatabase.execSQL("CREATE TABLE gpslogs ( id INTEGER PRIMARY KEY AUTOINCREMENT, name VARCHAR(20), start_lat REAL, end_lat REAL, start_long REAL, end_long REAL, start_location varchar(150), end_location varchar(150), start_ts INTeger, end_ts integer);");
//                    }
//                    catch(Exception ex){
//                     ex.printStackTrace();
//                    }
//                    mydatabase.close();

                    AlertDialog.Builder builder1 = new AlertDialog.Builder(MainActivity.this);
//                    builder1.setMessage("USE OF THE APP IS AT YOUR OWN RISK. THE APP IS PROVIDED ON AN \"AS IS\" BASIS. TO THE MAXIMUM EXTENT PERMITTED BY LAW: (A) TORG TECHNOLOGIES PVT. LTD. DISCLAIMS ALL LIABILITY WHATSOEVER, WHETHER ARISING IN CONTRACT, TORT (INCLUDING NEGLIGENCE) OR OTHERWISE IN RELATION TO THE APP; AND (B) ALL IMPLIED WARRANTIES, TERMS AND CONDITIONS RELATING TO THE APP (WHETHER IMPLIED BY STATUE, COMMON LAW OR OTHERWISE), INCLUDING (WITHOUT LIMITATION) ANY WARRANTY, TERM OR CONDITION AS TO ACCURACY, COMPLETENESS, SATISFACTORY QUALITY, PERFORMANCE, FITNESS FOR PURPOSE OR ANY SPECIAL PURPOSE, AVAILABILITY, NON INFRINGEMENT INFORMATION ACCURACY, INTEROPERABILITY, QUIET ENJOYMENT AND TITLE ARE, AS BETWEEN FUTURE AND YOU, HEREBY EXCLUDED. IN PARTICULAR, BUT WITHOUT PREJUDICE TO THE FOREGOING, WE ACCEPT NO RESPONSIBILITY FOR ANY TECHNICAL FAILURE OF THE INTERNET AND/OR THE APP; OR ANY DAMAGE OR INJURY TO USERS OR THEIR EQUIPMENT AS A RESULT OF OR RELATING TO THEIR USE OF THE APP. YOUR STATUTORY RIGHTS ARE NOT AFFECTED.");
//                    builder1.setCancelable(true);
                    builder1.setMessage(Html.fromHtml("<h1>DISCLAIMER / LIABILITY</h1><p  style=\"color:red\">MATI DSMS IS ONLY A TOOL FOR DRIVER ASSITANCE. USE OF THE APP IS AT YOUR OWN RISK. THE APP IS PROVIDED ON AN \"AS IS\" BASIS. TO THE MAXIMUM EXTENT PERMITTED BY LAW: (A) TORG TECHNOLOGIES PVT. LTD. (\"TORG TECH\") DISCLAIMS ALL LIABILITY WHATSOEVER, WHETHER ARISING IN CONTRACT, TORT (INCLUDING NEGLIGENCE) OR OTHERWISE IN RELATION TO THE APP; AND (B) ALL IMPLIED WARRANTIES, TERMS AND CONDITIONS RELATING TO THE APP (WHETHER IMPLIED BY STATUE, COMMON LAW OR OTHERWISE), INCLUDING (WITHOUT LIMITATION) ANY WARRANTY, TERM OR CONDITION AS TO ACCURACY, COMPLETENESS, SATISFACTORY QUALITY, PERFORMANCE, FITNESS FOR PURPOSE OR ANY SPECIAL PURPOSE, AVAILABILITY, NON INFRINGEMENT INFORMATION ACCURACY, INTEROPERABILITY, QUIET ENJOYMENT AND TITLE ARE, AS BETWEEN TORG TECH AND YOU, HEREBY EXCLUDED. IN PARTICULAR, BUT WITHOUT PREJUDICE TO THE FOREGOING, WE ACCEPT NO RESPONSIBILITY FOR ANY TECHNICAL FAILURE OF THE INTERNET AND/OR THE APP; OR ANY DAMAGE OR INJURY TO USERS OR THEIR EQUIPMENT AS A RESULT OF OR RELATING TO THEIR USE OF THE APP. YOUR STATUTORY RIGHTS ARE NOT AFFECTED.</p>"));
                    builder1.setPositiveButton(
                            "Accept & Continue",
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int id) {


                                    dialog.cancel();
                                    startService(new Intent(MainActivity.this,BackgroundService.class));
                                    Intent ax=new Intent(MainActivity.this,com.glesapp.glesapp.GLESAppNativeActivity.class);
                                    startActivity(ax);
                                }
                            });

//                    builder1.setNegativeButton(
//                            "No",
//                            new DialogInterface.OnClickListener() {
//                                public void onClick(DialogInterface dialog, int id) {
//                                    dialog.cancel();
//                                }
//                            });

                    AlertDialog alert11 = builder1.create();

                    alert11.show();






                } else {
                    requestPermission();
                }



            }



        });



    }

    @Override
    protected void onStart() {
        super.onStart();
        Log.d("test","ON START");
    }

    @Override
    protected void onPostResume() {
        super.onPostResume();
        Log.d("test","ON POST RESUME");
    }

    @Override
    protected void onResume() {
        super.onResume();
        gps = new GPSTracker(this);
        Log.d("test2",String.valueOf(BackgroundService.inapp2));

        // Check if GPS enabled
        if(gps.canGetLocation()) {

            final Double latitude = gps.getLatitude();
            final Double longitude = gps.getLongitude();


            Toast.makeText(getApplicationContext(), "Your Location is - \nLat: " + latitude + "\nLong: " + longitude, Toast.LENGTH_LONG).show();

            OkHttpClient client = new OkHttpClient.Builder()
                    .readTimeout(100, TimeUnit.SECONDS)
                    .writeTimeout(100, TimeUnit.SECONDS)
                    .build();


            MediaType JSON
                    = MediaType.parse("application/json; charset=utf-8");
            String url="https://nominatim.openstreetmap.org/reverse?format=jsonv2&zoom=14&addressdetails=1&lat="+latitude.toString()+"&lon="+longitude.toString();
                    Log.d("test",url);
            Request request = new Request.Builder()
                    .url(url)
                    .build();

            client.newCall(request).enqueue(new Callback() {
                @Override
                public void onFailure(Call call, IOException e) {
                    // Do something when request failed
                    e.printStackTrace();
                    Log.d(TAG, "Request Failed.");
                }

                @Override
                public void onResponse(Call call, Response response) throws IOException {
                    if(!response.isSuccessful()){
                        throw new IOException("Error : " + response);
                    }else {
                        try{
//                        Toast.makeText(getApplicationContext(),"Request Successful."+response.body().string(),Toast.LENGTH_LONG).show();
                        String resp=response.body().string();
                        try {
                          JSONObject Jobject = new JSONObject(resp);
                            JSONObject Jarray = Jobject.getJSONObject("address");
                            String finallocation="";
                            if (Jarray.has("suburb")){
                                finallocation=Jarray.getString("suburb");
                            }
                            else if (Jarray.has("town")){
                                finallocation=Jarray.getString("town");
                            }
                            else if (Jarray.has("village")){
                                finallocation=Jarray.getString("village");
                            }
                            else if (Jarray.has("city")){
                                finallocation=Jarray.getString("city");
                            }
                            SQLiteDatabase mydatabase = openOrCreateDatabase("/data/data/com.torgtek.matidsms/databases/dsms.db", MODE_PRIVATE, null);
                            try {
                                mydatabase.execSQL("CREATE TABLE gpslogs ( id INTEGER PRIMARY KEY AUTOINCREMENT, start_lat REAL NULL," +
                                        " end_lat REAL NULL," +
                                        " start_long REAL NULL," +
                                        " end_long REAL NULL," +
                                        " start_location varchar(150) NULL," +
                                        " end_location varchar(150) NULL," +
                                        " start_ts integer NULL," +
                                        " end_ts integer NULL);");
                            }
                            catch(Exception ex){
                                ex.printStackTrace();
                            }
                            String last;
                            try {
                                last=getIntent().getStringExtra("last_activity");
                            }
                            catch (Exception exs){
                                last="NotLogin";
                            }
                            Log.d("test",last);
                            Log.d("test", String.valueOf(inapp));
                            SharedPreferences prefs = getSharedPreferences("MY_PREFS_NAME", MODE_PRIVATE);
                            if (prefs.getString("name","false").equals("false")){
                            try{
                                ContentValues cv = new ContentValues();
                                cv.put("start_lat",latitude);
                                cv.put("start_long",longitude);
                                cv.put("start_location",finallocation);
                                cv.put("start_ts",System.currentTimeMillis()/1000);
                                cv.put("start_location",finallocation);
                                current_id= mydatabase.insert("gpslogs",null,cv);
                                    Log.d("TEST",current_id.toString());
                                BackgroundService.inapp2=true;
                                Log.d("test22",String.valueOf(BackgroundService.inapp2));
                                SharedPreferences.Editor editor = getSharedPreferences("MY_PREFS_NAME", MODE_PRIVATE).edit();
                                editor.putString("name", "true");
                                editor.apply();
                                mydatabase.close();

                            }
                            catch ( Exception exx){
                                exx.printStackTrace();
                            }}



                            else {
                                Cursor cursor = mydatabase.rawQuery("SELECT  id FROM gpslogs" , null);
                                String lastid="0";
                                if(cursor.moveToLast()){
                                    lastid= cursor.getString(0);
                                    //--get other cols values
                                }
                                ContentValues cv = new ContentValues();
                                cv.put("end_lat",latitude);
                                cv.put("end_long",longitude);
                                cv.put("end_location",finallocation);
                                cv.put("end_ts",System.currentTimeMillis()/1000);
                                cv.put("end_location",finallocation);
                                mydatabase.update("gpslogs",cv,"id = ?", new String[]{lastid});
                                BackgroundService.inapp2=false;
                                Log.d("test2",String.valueOf(BackgroundService.inapp2));
                                SharedPreferences.Editor editor = getSharedPreferences("MY_PREFS_NAME", MODE_PRIVATE).edit();
                                editor.putString("name", "false");
                                editor.apply();
                                mydatabase.close();

                            }








                        }
                        catch (Exception e) {
                            e.printStackTrace();
                        }

                        Log.d(TAG,"Request Successful.33");
                        }
                        catch (Exception px){
                            px.printStackTrace();
                        }

                    }

                    // Read data in the worker thread
//                    final String data = response.body().string();
                }
            });

//            try {
//                Response response = client.newCall(request).execute();
////                Toast.makeText(getApplicationContext(),response
//            } catch (IOException e) {
//                e.printStackTrace();
//            }
        }
    }


    private boolean checkPermission() {
        boolean location=true;
        boolean camera=true;
        boolean storage=true;
        boolean internet=true;
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION)
                != PackageManager.PERMISSION_GRANTED)  {
            // Permission is not granted
            location=false;
        }
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
                != PackageManager.PERMISSION_GRANTED)  {
            // Permission is not granted
            camera=false;
        }
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED)  {
            // Permission is not granted
            storage=false;
        }
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.INTERNET)
                != PackageManager.PERMISSION_GRANTED)  {
            // Permission is not granted
            internet=false;
        }
        if (camera & location &storage & internet){

            return true;

        }
        return false;
    }

    private void requestPermission() {

        ActivityCompat.requestPermissions(this,
                new String[]{Manifest.permission.CAMERA,Manifest.permission.ACCESS_FINE_LOCATION,Manifest.permission.WRITE_EXTERNAL_STORAGE,
                        Manifest.permission.INTERNET
                },
                PERMISSION_REQUEST_CODE);
    }


    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        switch (requestCode) {
            case PERMISSION_REQUEST_CODE:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Toast.makeText(getApplicationContext(), "Permission Granted", Toast.LENGTH_SHORT).show();

                    // main logic
                } else {
                    Toast.makeText(getApplicationContext(), "Permission Denied", Toast.LENGTH_SHORT).show();
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
                                != PackageManager.PERMISSION_GRANTED) {
                            showMessageOKCancel("You need to allow access permissions",
                                    new DialogInterface.OnClickListener() {
                                        @Override
                                        public void onClick(DialogInterface dialog, int which) {
                                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                                                requestPermission();
                                            }
                                        }
                                    });
                        }
                    }
                }
                break;
            case PERMISSION_REQUEST_CODE_GPS:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Toast.makeText(getApplicationContext(), "Permission Granted", Toast.LENGTH_SHORT).show();

                    // main logic
                } else {
                    Toast.makeText(getApplicationContext(), "Permission Denied", Toast.LENGTH_SHORT).show();
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION)
                                != PackageManager.PERMISSION_GRANTED) {
                            showMessageOKCancel("You need to allow access permissions",
                                    new DialogInterface.OnClickListener() {
                                        @Override
                                        public void onClick(DialogInterface dialog, int which) {
                                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                                                requestPermission();
                                            }
                                        }
                                    });
                        }
                    }
                }
                break;
            case PERMISSION_REQUEST_CODE_STORAGE:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Toast.makeText(getApplicationContext(), "Permission Granted", Toast.LENGTH_SHORT).show();

                    // main logic
                } else {
                    Toast.makeText(getApplicationContext(), "Permission Denied", Toast.LENGTH_SHORT).show();
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                                != PackageManager.PERMISSION_GRANTED) {
                            showMessageOKCancel("You need to allow access permissions",
                                    new DialogInterface.OnClickListener() {
                                        @Override
                                        public void onClick(DialogInterface dialog, int which) {
                                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                                                requestPermission();
                                            }
                                        }
                                    });
                        }
                    }
                }
                break;
            case PERMISSION_REQUEST_CODE_INTERNET:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Toast.makeText(getApplicationContext(), "Permission Granted", Toast.LENGTH_SHORT).show();

                    // main logic
                } else {
                    Toast.makeText(getApplicationContext(), "Permission Denied", Toast.LENGTH_SHORT).show();
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                        if (ContextCompat.checkSelfPermission(this, Manifest.permission.INTERNET)
                                != PackageManager.PERMISSION_GRANTED) {
                            showMessageOKCancel("You need to allow access permissions",
                                    new DialogInterface.OnClickListener() {
                                        @Override
                                        public void onClick(DialogInterface dialog, int which) {
                                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                                                requestPermission();
                                            }
                                        }
                                    });
                        }
                    }
                }
                break;
        }
    }


    private void showMessageOKCancel(String message, DialogInterface.OnClickListener okListener) {
        new AlertDialog.Builder(MainActivity.this)
                .setMessage(message)
                .setPositiveButton("OK", okListener)
                .setNegativeButton("Cancel", null)
                .create()
                .show();
    }






}


 class StableArrayAdapter extends ArrayAdapter<String> {

    HashMap<String, Integer> mIdMap = new HashMap<String, Integer>();

    public StableArrayAdapter(Context context, int textViewResourceId,
                              List<String> objects) {
        super(context, textViewResourceId, objects);
        for (int i = 0; i < objects.size(); ++i) {
            mIdMap.put(objects.get(i), i);
        }
    }

    @Override
    public long getItemId(int position) {
        String item = getItem(position);
        return mIdMap.get(item);
    }

    @Override
    public boolean hasStableIds() {
        return true;
    }

}

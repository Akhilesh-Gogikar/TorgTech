package com.torgtek.matidsms;

import android.app.IntentService;
import android.content.Intent;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;

import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;

/**
 * An {@link IntentService} subclass for handling asynchronous task requests in
 * a service on a separate handler thread.
 * <p>
 * TODO: Customize class - update intent actions and extra parameters.
 */
public class BackgroundService extends IntentService {

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

    @Override
    protected void onHandleIntent(Intent intent) {


        SQLiteDatabase mydatabase = openOrCreateDatabase("/data/data/com.torgtek.matidsms/databases/dsms.db",MODE_PRIVATE,null);
        OkHttpClient client = new OkHttpClient();
        while(true){
            try {
            Cursor resultSet = mydatabase.rawQuery("Select * from LOGS",null);
                while (resultSet.moveToNext()) {
                    Object k=resultSet.getString(0);

                    String url="";
                    Request request = new Request.Builder()
                            .url(url)
                            .build();

                    Response response = client.newCall(request).execute();
                    if(response.code()==200){

                        //idhar apun ko sqlite update karna hai ki data send ho gaya hai
                    }



                }

                Thread.sleep(1000);
            } catch (Exception e) {
                e.printStackTrace();
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
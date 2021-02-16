/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#include <cstdlib>
#include <cstring>
#include <jni.h>
#include <android_native_app_glue.h>
#include "app_engine.h"
#include "gestureDetector.h"
#include "sqlite3.h"
#include <chrono>
#include <cstdio>
#include <ctime>


static AppEngine *s_pEngineObj = nullptr;

AppEngine *
GetAppEngine(void)
{
    return s_pEngineObj;
}


static void
handle_imgui_input (AppEngine *engine, AInputEvent *event)
{
    int32_t  action = AMotionEvent_getAction (event);
    int32_t  index  = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    uint32_t flags  = (action & AMOTION_EVENT_ACTION_MASK);
    int32_t  count  = AMotionEvent_getPointerCount (event);
    int32_t released_pointer_id;

    float x = AMotionEvent_getX (event, index);
    float y = AMotionEvent_getY (event, index);

    switch (flags) {
    case AMOTION_EVENT_ACTION_DOWN:
        engine->button_cb (index, 1, x, y);
        engine->mousemove_cb (x, y);
        break;

    case AMOTION_EVENT_ACTION_POINTER_DOWN:
        break;

    case AMOTION_EVENT_ACTION_UP:
        engine->button_cb (index, 0, x, y);
        break;

    case AMOTION_EVENT_ACTION_POINTER_UP:
        released_pointer_id = AMotionEvent_getPointerId (event, index);
        x = AMotionEvent_getX (event, released_pointer_id);
        y = AMotionEvent_getY (event, released_pointer_id);
        break;

    case AMOTION_EVENT_ACTION_MOVE:
        engine->mousemove_cb (x, y);
        break;

    case AMOTION_EVENT_ACTION_CANCEL:
        break;

    default:
        break;
    }
}

static int32_t
engine_handle_input (struct android_app *app, AInputEvent *event)
{
    AppEngine* engine = reinterpret_cast<AppEngine*>(app->userData);

    if (AInputEvent_getType (event) != AINPUT_EVENT_TYPE_MOTION)
        return 0;

    ndk_helper::GESTURE_STATE tapState  = engine->tap_detector_ .Detect(event);
    ndk_helper::GESTURE_STATE dragState = engine->drag_detector_.Detect(event);

    handle_imgui_input (engine, event);

    return 0;
}

static void
ProcessAndroidCmd (struct android_app* app, int32_t cmd) 
{
    AppEngine* engine = reinterpret_cast<AppEngine*>(app->userData);

    switch (cmd) {
    // The window is being shown, get it ready.
    case APP_CMD_INIT_WINDOW: 
        if (engine->AndroidApp()->window != NULL) 
        {
            engine->OnAppInitWindow();
        }
        break;

    // The window is being hidden or closed, clean it up.
    case APP_CMD_TERM_WINDOW: 
        engine->OnAppTermWindow();
        break;
    }
}

inline bool exists_test1 (const std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    int i;
    for(i = 0; i<argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

/*--------------------------------------------------------------------------- *
 *      M A I N    F U N C T I O N
 *--------------------------------------------------------------------------- */
void android_main(struct android_app* state) 
{
    AppEngine engine(state);
    state->userData = reinterpret_cast<void*>(&engine);
    state->onAppCmd = ProcessAndroidCmd;
    state->onInputEvent = engine_handle_input;

    s_pEngineObj = &engine;

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    char sql[1024];

    rc = sqlite3_open("/data/data/com.example.tflite_iris_landmark/databases/dsms.db", &db);

    bool check = exists_test1("/data/data/com.example.tflite_iris_landmark/databases/dsms.db");

    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));

    } else {
        fprintf(stderr, "Opened database successfully\n");
    }

    std::time_t rawtime;
    std::tm* timeinfo;
    char buffer [80];

    std::time(&rawtime);
    timeinfo = std::localtime(&rawtime);

    std::strftime(buffer,80,"%Y%m%d%H%M%S",timeinfo);

    /* Create SQL statement */

    sprintf(sql ,"CREATE TABLE M%s ("  \
      "FRAME    INT    PRIMARY KEY     NOT NULL," \
      "SLEEPY           INT    NOT NULL," \
      "DROWSY            INT     NOT NULL," \
      "DISTRACTED        INT    NOT NULL," \
      "ALARMS        INT    NOT NULL," \
      "INTERVAL        REAL    NOT NULL," \
      "AVGEAR        REAL    NOT NULL," \
      "AVGMAR        REAL   NOT NULL," \
      "AVGDEV        REAL    NOT NULL," \
      "SPEED        REAL    NOT NULL," \
      "DIST        REAL    NOT NULL," \
      "AX        REAL    NOT NULL," \
      "AY        REAL    NOT NULL," \
      "AZ        REAL    NOT NULL," \
      "TS         TIMESTAMP      NOT NULL);", buffer);

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Table created successfully\n");
    }


    while (1)
    {
        int ident, events;
        struct android_poll_source* source;

        while ((ident = ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0)
        {
            if (source != NULL) {
                source->process(state, source);
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine.DeleteCamera();
                s_pEngineObj = nullptr;
                return;
            }
        }

        engine.UpdateFrame ();

        char strbuf[512];

        engine.StatusString(strbuf);

        std::time_t result = std::time(nullptr);

        /* Create SQL statement */
        sprintf(strbuf, "INSERT INTO M%s (FRAME,SLEEPY,DROWSY,DISTRACTED,ALARMS,INTERVAL,AVGEAR,AVGMAR,AVGDEV,SPEED,DIST,AX,AY,AZ,TS)"  \
         "VALUES (%s,%ld);", buffer, strbuf, result);

        /* Execute SQL statement */
        rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

        if( rc != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        } else {
            fprintf(stdout, "Records created successfully\n");
        }
    }

    sqlite3_close(db);
}

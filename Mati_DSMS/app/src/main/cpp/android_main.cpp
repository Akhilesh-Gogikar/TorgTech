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
#include <firebase/admob.h>
#include <firebase/admob/types.h>
#include <firebase/app.h>
#include <firebase/future.h>
#include <firebase/analytics.h>
#include <firebase/admob/banner_view.h>

#include <android/native_activity.h>

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

static struct android_app* g_app_state = nullptr;
static bool g_destroy_requested = false;
static bool g_started = false;
static bool g_restarted = false;

bool ProcessEvents(int msec) {
    struct android_poll_source* source = nullptr;
    int events;
    int looperId = ALooper_pollAll(msec, nullptr, &events,
                                   reinterpret_cast<void**>(&source));
    if (looperId >= 0 && source) {
        source->process(g_app_state, source);
    }
    return g_destroy_requested | g_restarted;
}

static void WaitForFutureCompletion(firebase::FutureBase future) {
    while (!ProcessEvents(1000)) {
        if (future.status() != firebase::kFutureStatusPending) {
            break;
        }
    }

    if (future.error() != firebase::admob::kAdMobErrorNone) {
        return;
    }
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
    g_app_state = state;

    s_pEngineObj = &engine;

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    char sql[1024];

    rc = sqlite3_open("/data/data/com.torgtek.matidsms/databases/dsms.db", &db);

    bool check = exists_test1("/data/data/com.torgtek.matidsms/databases/dsms.db");

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

    sprintf(sql ,"CREATE TABLE LOGS ("  \
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
      "PROCESSED        INT    NOT NULL," \
      "TS         DATETIME      NOT NULL);");

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Table created successfully\n");
    }

    {

        //App ID: ca-app-pub-1062260908990670~7999149993
        // Ad unit ID : ca-app-pub-1062260908990670/7488299511

        using namespace firebase;

        JNIEnv *env;

        state->activity->vm->AttachCurrentThread(&env, nullptr);

        auto app = App::Create(AppOptions(), env, state->activity->clazz);

        const char* kAdMobAppID = "ca-app-pub-1062260908990670~7999149993";
        analytics::Initialize(*app);

        admob::Initialize(*app, kAdMobAppID);

        const char* kBannerAdUnit = "ca-app-pub-3940256099942544/6300978111";

        admob::BannerView* banner_view;
        banner_view = new admob::BannerView();

        admob::AdSize ad_size;
        ad_size.ad_size_type = firebase::admob::kAdSizeStandard;
        ad_size.width = 320;
        ad_size.height = 50;
// my_ad_parent is a reference to an iOS UIView or an Android Activity.
// This is the parent UIView or Activity of the banner view.
        banner_view->Initialize(static_cast<firebase::admob::AdParent>(state->activity->clazz), kBannerAdUnit, ad_size);

        banner_view->InitializeLastResult();
        struct AdRequest {
            const char **test_device_ids;
            unsigned int test_device_id_count;
            const char **keywords;
            unsigned int keyword_count;
            const admob::KeyValuePair *extras;
            unsigned int extras_count;
            int birthday_day;
            int birthday_month;
            int birthday_year;
            admob::Gender gender;
            admob::ChildDirectedTreatmentState tagged_for_child_directed_treatment;
        };

        firebase::admob::AdRequest my_ad_request = {};

        // If the app is aware of the user's gender, it can be added to the
// targeting information. Otherwise, "unknown" should be used.
        my_ad_request.gender = firebase::admob::kGenderUnknown;

// The user's birthday, if known. Note that months are indexed from one.
        my_ad_request.birthday_day = 10;
        my_ad_request.birthday_month = 11;
        my_ad_request.birthday_year = 1976;

// Additional keywords to be used in targeting.
        static const char* kKeywords[] = {"AdMob", "C++", "Fun"};
        my_ad_request.keyword_count = sizeof(kKeywords) / sizeof(kKeywords[0]);
        my_ad_request.keywords = kKeywords;

// "Extra" key value pairs can be added to the request as well.
        static const firebase::admob::KeyValuePair kRequestExtras[] = {
                {"the_name_of_an_extra", "the_value_for_that_extra"}};
        my_ad_request.extras_count = sizeof(kRequestExtras) / sizeof(kRequestExtras[0]);
        my_ad_request.extras = kRequestExtras;

// Register the device IDs associated with any devices that will be used to
// test your app. Below are sample test device IDs used for making the ad request.
        static const char* kTestDeviceIDs[] =
                {"2077ef9a63d2b398840261c8221a0c9b",
                 "098fe087d987c9a878965454a65654d7"};
        my_ad_request.test_device_id_count =
                sizeof(kTestDeviceIDs) / sizeof(kTestDeviceIDs[0]);
        my_ad_request.test_device_ids = kTestDeviceIDs;

        WaitForFutureCompletion(banner_view->InitializeLastResult());

        banner_view->LoadAd(my_ad_request);

        WaitForFutureCompletion(banner_view->LoadAdLastResult());

        banner_view->Show();

        WaitForFutureCompletion(banner_view->ShowLastResult());

        banner_view->MoveTo(firebase::admob::BannerView::kPositionBottom);

        WaitForFutureCompletion(banner_view->MoveToLastResult());

        if (state->destroyRequested != 0) {
            delete banner_view;
            delete app;
            firebase::admob::Terminate();
        }

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

            DBG_LOGE("Engine state: %d", engine.state);


            if (engine.state == 1){
                state->destroyRequested = 1;
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine.DeleteCamera();
                s_pEngineObj = nullptr;
                sqlite3_close(db);
                return;
            }
        }

        engine.UpdateFrame ();

        char strbuf[512];

        engine.StatusString(strbuf);

        std::time_t result = std::time(nullptr);

        char insert_buf[512];
        char insert_buf_sql[512];

        /* Create SQL statement */
        sprintf(insert_buf_sql, "INSERT INTO `LOGS`(`FRAME`,`SLEEPY`,`DROWSY`,`DISTRACTED`,`ALARMS`,`INTERVAL`,`AVGEAR`,`AVGMAR`,`AVGDEV`,`SPEED`,`DIST`,`AX`,`AY`,`AZ`,`PROCESSED`,`TS`)"  \
         " VALUES (%s,0,%ld);", strbuf, result);

        /* Execute SQL statement */
        rc = sqlite3_exec(db, insert_buf_sql, callback, 0, &zErrMsg);

        if( rc != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        } else {
            fprintf(stdout, "Records created successfully\n");
        }

        if (engine.state == 1){
            state->destroyRequested = 1;
        }

        // Check if we are exiting.
        if (state->destroyRequested != 0) {
            engine.OnAppTermWindow();
            s_pEngineObj = nullptr;
            firebase::admob::Terminate();
            break;
        }

    }
    sqlite3_close(db);
    ANativeActivity_finish(state->activity);
    exit(0);
}
/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#ifndef __APP_ENGINE_H__
#define __APP_ENGINE_H__

#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <functional>
#include <thread>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>

#include "util_texture.h"
#include "util_render_target.h"
#include "camera_manager.h"
#include "render_imgui.h"
#include "tflite_facemesh.h"
#include "gestureDetector.h"
#include <oboe/Oboe.h>
#include <math.h>

class OboeSinePlayer: public oboe::AudioStreamCallback {
public:

    virtual ~OboeSinePlayer() = default;

    // Call this from Activity onResume()
    int32_t startAudio() {
        std::lock_guard<std::mutex> lock(mLock);
        oboe::AudioStreamBuilder builder;
        // The builder set methods can be chained for convenience.
        oboe::Result result( builder.setSharingMode(oboe::SharingMode::Exclusive)
                                     ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
                                     ->setChannelCount(kChannelCount)
                                     ->setSampleRate(kSampleRate)
                                     ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
                                     ->setFormat(oboe::AudioFormat::Float)
                                     ->setCallback(this)
                                     ->openStream(mStream));
        if (result != oboe::Result::OK) return (int32_t) result;

        // Typically, start the stream after querying some stream information, as well as some input from the user
        result = mStream->requestStart();
        return (int32_t) result;
    }

    // Call this from Activity onPause()
    void stopAudio() {
        // Stop, close and delete in case not already closed.
        std::lock_guard<std::mutex> lock(mLock);
        if (mStream) {
            mStream->stop();
            mStream->close();
            mStream.reset();
        }
    }

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override {
        float *floatData = (float *) audioData;
        for (int i = 0; i < numFrames; ++i) {
            float sampleValue = kAmplitude * sinf(mPhase);
            for (int j = 0; j < kChannelCount; j++) {
                floatData[i * kChannelCount + j] = sampleValue;
            }
            mPhase += mPhaseIncrement;
            if (mPhase >= kTwoPi) mPhase -= kTwoPi;
        }
        return oboe::DataCallbackResult::Continue;
    }

private:
    std::mutex         mLock;
    std::shared_ptr<oboe::AudioStream> mStream;

    // Stream params
    static int constexpr kChannelCount = 2;
    static int constexpr kSampleRate = 48000;
    // Wave params, these could be instance variables in order to modify at runtime
    static float constexpr kAmplitude = 2.5f;
    static float constexpr kFrequency = 200;
    static float constexpr kPI = M_PI;
    static float constexpr kTwoPi = kPI * 2;
    static double constexpr mPhaseIncrement = kFrequency * kTwoPi / (double) kSampleRate;
    // Keeps track of where the wave is
    float mPhase = 0.0;
};


typedef struct gles_ctx {
    int initdone;
    int frame_count;

    char *str_glverstion;
    char *str_glvendor;
    char *str_glrender;

    int disp_w, disp_h;

    bool tex_camera_valid;
    texture_2d_t tex_static;
    texture_2d_t app_logo;
    texture_2d_t company_logo;
    texture_2d_t safe_drive;
    texture_2d_t drowsy;
    texture_2d_t alert;
    texture_2d_t tex_camera;
    texture_2d_t tex_input;
    EGLImage egl_img;

    render_target_t rtarget_main;
    render_target_t rtarget_crop;
} gles_ctx_t;


class AppEngine {
public:
    explicit AppEngine(android_app* app);
    ~AppEngine();

    // Interfaces to android application framework
    struct android_app* AndroidApp(void) const;
    void OnAppInitWindow(void);
    void OnAppTermWindow(void);

    // Manage Camera Permission
    void RequestCameraPermission();
    void OnCameraPermission(jboolean granted);

    // Manage NDKCamera Object
    void InitCamera (void);
    void CreateCamera (int facing);
    void DeleteCamera (void);

    // OpenGLES Render
    void InitGLES (void);
    void TerminateGLES (void);
    
    void LoadInputTexture (texture_2d_t *tex, char *fname);
    void UpdateCameraTexture ();
    void CropCameraTexture ();

    void UpdateFrame (void);
    void RenderFrame (void);

    void StatusString (char* strbuf);

    void DrawTFLiteConfigInfo ();

    // IMGUI
    void setup_imgui (int win_w, int win_h, imgui_data_t *imgui_data);

    /* for touch gesture */
    ndk_helper::TapDetector        tap_detector_;
    ndk_helper::DoubletapDetector  doubletap_detector_;
    ndk_helper::DragDetector       drag_detector_;

    void mousemove_cb (int x, int y);
    void button_cb (int button, int state, int x, int y);
    void keyboard_cb (int key, int state, int x, int y);

private:

    struct android_app  *m_app;

    OboeSinePlayer  soundGenerator;

    int blinks;
    int sleep;
    int drowsy;
    int yawns;
    int distracted;

    int yawn_streak;
    int blink_streak;
    int distracted_streak;

    int alarms;
    double p_time;
    double time;

    float speed;
    float distance;

    float X;
    float Y;
    float Z;

    float v_x;
    float v_y;
    float v_z;

    float a_x;
    float a_y;
    float a_z;

    float g_x;
    float g_y;
    float g_z;

    int sound_started;
    int sound_streak;

    float avg_EAR;
    float avg_MAR;
    float avg_Dev;


    bool                m_cameraGranted;
    NDKCamera           *m_camera;
    ImageReaderHelper   m_ImgReader;

    gles_ctx_t          glctx;
    std::vector<uint8_t> m_facedet_tflite_model_buf;
    std::vector<uint8_t> m_facelandmark_tflite_model_buf;
    std::vector<uint8_t> m_irislandmark_tflite_model_buf;

    imgui_data_t        imgui_data;
    int                 m_camera_facing;

public:
};

AppEngine *GetAppEngine (void);

#endif  // __APP_ENGINE_H__

/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#include <cstdio>
#include "util_debug.h"
#include "util_asset.h"
#include "util_egl.h"
#include "util_debugstr.h"
#include "util_pmeter.h"
#include "util_texture.h"
#include "util_render2d.h"
#include "app_engine.h"
#include "render_imgui.h"
#include "assertgl.h"
#include "util_matrix.h"
#include "tflite_facemesh.h"
#include "utils.h"
#include "touch_event.h"

#define UNUSED(x) (void)(x)

#define CAMERA_RESOLUTION_W     640
#define CAMERA_RESOLUTION_H     480
#define CAMERA_CROP_WIDTH       480 /* make a src image square */
#define CAMERA_CROP_HEIGHT      480 /* make a src image square */

static imgui_data_t s_gui_prop = {0};

void
AppEngine::DrawTFLiteConfigInfo ()
{
    char strbuf[512];
    float col_pink[]  = {1.0f, 0.0f, 1.0f, 0.5f};
    float col_white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float *col_bg = col_pink;

#if defined (USE_GPU_DELEGATEV2)
    sprintf (strbuf, "GPU_DELEGATEV2: ON ");
#else
    sprintf (strbuf, "GPU_DELEGATEV2: ---");
#endif
    draw_dbgstr_ex (strbuf, glctx.disp_w - 250, glctx.disp_h - 24, 1.0f, col_white, col_bg);

}


/* Adjust the texture size to fit the window size
 *
 *                      Portrait
 *     Landscape        +------+
 *     +-+------+-+     +------+
 *     | |      | |     |      |
 *     | |      | |     |      |
 *     +-+------+-+     +------+
 *                      +------+
 */
static void
adjust_texture (int win_w, int win_h, int texw, int texh,
                int *dx, int *dy, int *dw, int *dh, int full_zoom)
{
    float win_aspect = (float)win_w / (float)win_h;
    float tex_aspect = (float)texw  / (float)texh;
    float scale;
    float scaled_w, scaled_h;
    float offset_x, offset_y;

    if (((full_zoom == 0) && (win_aspect > tex_aspect)) ||
        ((full_zoom == 1) && (win_aspect < tex_aspect)) )
    {
        scale = (float)win_h / (float)texh;
        scaled_w = scale * texw;
        scaled_h = scale * texh;
        offset_x = (win_w - scaled_w) * 0.5f;
        offset_y = 0;
    }
    else
    {
        scale = (float)win_w / (float)texw;
        scaled_w = scale * texw;
        scaled_h = scale * texh;
        offset_x = 0;
        offset_y = (win_h - scaled_h) * 0.5f;
    }

    *dx = (int)offset_x;
    *dy = (int)offset_y;
    *dw = (int)scaled_w;
    *dh = (int)scaled_h;
}


#if defined (USE_IMGUI)
void
AppEngine::mousemove_cb (int x, int y)
{
    imgui_mousemove (x, y);
}

void
AppEngine::button_cb (int button, int state, int x, int y)
{
    imgui_mousebutton (button, state, x, y);
}

void
AppEngine::keyboard_cb (int key, int state, int x, int y)
{
}
#endif

void
AppEngine::setup_imgui (int win_w, int win_h, imgui_data_t *imgui_data)
{
#if defined (USE_IMGUI)
    //egl_set_motion_func (mousemove_cb);
    //egl_set_button_func (button_cb);
    //egl_set_key_func    (keyboard_cb);

    init_touch_event (win_w, win_h);

    init_imgui (win_w, win_h);
#endif

    imgui_data->camera_facing  = m_camera_facing;
    s_gui_prop.pose_scale_x = 100.0f;
    s_gui_prop.pose_scale_y = 100.0f;
    s_gui_prop.pose_scale_z = 100.0f;
    s_gui_prop.camera_pos_z = 200.0f;
    s_gui_prop.draw_axis    = 0;
    s_gui_prop.draw_pmeter  = 1;
    *imgui_data = s_gui_prop;
}


void 
AppEngine::RenderFrame ()
{
    texture_2d_t srctex = glctx.tex_input;
    texture_2d_t srctex1 = glctx.tex_input1;
    int win_w  = glctx.disp_w;
    int win_h  = glctx.disp_h;
    static double ttime[16] = {0}, interval, invoke_ms0 = 0, invoke_ms1 = 0, invoke_ms2 = 0,
    depth_invoke_ms=0, deeplab_invoke_ms=0, detect_invoke_ms=0, laneseg_invoke_ms=0;

    int draw_x, draw_y, draw_w, draw_h;
    int texw = srctex.width;
    int texh = srctex.height;
    adjust_texture (win_w, win_h, texw, texh, &draw_x, &draw_y, &draw_w, &draw_h, 0);

    glClearColor (0.f, 0.f, 0.f, 1.0f);

    /* --------------------------------------- *
     *  Render Loop
     * --------------------------------------- */
    int count = glctx.frame_count;

    DBG_LOGE("Frame count:%d \n",
             count);
    {
        face_detect_result_t    face_detect_ret = {0};
        face_landmark_result_t  face_mesh_ret[MAX_FACE_NUM] = {0};
        irismesh_result_t       iris_mesh_ret[MAX_FACE_NUM][2] = {0};
        dense_depth_result_t dense_depth_result = {0};
        deeplab_result_t deeplab_result;
        laneseg_result_t laneseg_result;
        detect_result_t detection = {};
        char strbuf[512];

        PMETER_RESET_LAP ();
        PMETER_SET_LAP ();

        ttime[1] = pmeter_get_time_ms ();
        interval = (count > 0) ? ttime[1] - ttime[0] : 0;
        ttime[0] = ttime[1];

        glClear (GL_COLOR_BUFFER_BIT);
        glViewport (0, 0, win_w, win_h);

        soundGenerator.startAudio();

        /* --------------------------------------- *
         *  Dense Depth
         * --------------------------------------- */
        feed_dense_depth_image (&srctex1, win_w, win_h);

        ttime[2] = pmeter_get_time_ms ();
        invoke_dense_depth (&dense_depth_result);
        ttime[3] = pmeter_get_time_ms ();
        depth_invoke_ms = ttime[3] - ttime[2];

        /* --------------------------------------- *
         *  semantic segmentation
         * --------------------------------------- */
        feed_deeplab_image (&srctex1, win_w, win_h);

        ttime[4] = pmeter_get_time_ms ();
        invoke_deeplab (&deeplab_result);
        ttime[5] = pmeter_get_time_ms ();
        deeplab_invoke_ms = ttime[5] - ttime[4];

        /* --------------------------------------- *
         *  lane segmentation
         * --------------------------------------- */
        feed_laneseg_image (&srctex1, win_w, win_h);

        ttime[14] = pmeter_get_time_ms ();
        invoke_laneseg (&laneseg_result);
        ttime[15] = pmeter_get_time_ms ();
        laneseg_invoke_ms = ttime[15] - ttime[14];

        /* --------------------------------------- *
         *  object detection
         * --------------------------------------- */
        feed_detect_image (&srctex1, win_w, win_h);

        ttime[6] = pmeter_get_time_ms ();
        invoke_detect (&detection);
        ttime[7] = pmeter_get_time_ms ();
        detect_invoke_ms = ttime[7] - ttime[6];

        /* --------------------------------------- *
         *  face detection
         * --------------------------------------- */
        feed_face_detect_image (&srctex, win_w, win_h);

        ttime[8] = pmeter_get_time_ms ();
        invoke_face_detect (&face_detect_ret);
        ttime[9] = pmeter_get_time_ms ();
        invoke_ms0 = ttime[9] - ttime[8];

        /* --------------------------------------- *
         *  face landmark
         * --------------------------------------- */
        invoke_ms1 = 0;
        for (int face_id = 0; face_id < face_detect_ret.num; face_id ++)
        {
            feed_face_landmark_image (&srctex, win_w, win_h, &face_detect_ret, face_id);

            ttime[10] = pmeter_get_time_ms ();
            invoke_facemesh_landmark (&face_mesh_ret[face_id]);
            ttime[11] = pmeter_get_time_ms ();
            invoke_ms1 += ttime[11] - ttime[10];
        }

        /* --------------------------------------- *
         *  Iris landmark
         * --------------------------------------- */
        invoke_ms2 = 0;
        for (int face_id = 0; face_id < face_detect_ret.num; face_id ++)
        {
            for (int eye_id = 0; eye_id < 2; eye_id ++)
            {
                feed_iris_landmark_image (&srctex, win_w, win_h, &face_detect_ret.faces[face_id], &face_mesh_ret[face_id], eye_id);

                ttime[12] = pmeter_get_time_ms ();
                invoke_irismesh_landmark (&iris_mesh_ret[face_id][eye_id]);
                ttime[13] = pmeter_get_time_ms ();
                invoke_ms2 += ttime[13] - ttime[12];
            }

            /* need to horizontal flip for right eye */
            flip_horizontal_iris_landmark (&iris_mesh_ret[face_id][1]);
        }



        /* --------------------------------------- *
         *  render scene
         * --------------------------------------- */
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw_2d_texture_ex (&srctex1, draw_x, draw_y, draw_w, draw_h, 0);

        render_obj_detect_region (draw_x, draw_y, draw_w, draw_h, &detection);

        render_deeplab_result (draw_x, draw_y, draw_w, draw_h, &deeplab_result);

        render_depth_image_3d (&srctex1, draw_x, draw_y, draw_w, draw_h, &dense_depth_result);

        float camx = 100;
        float camy = 100;
        float camw = 300;
        float camh = 300;
        /* visualize the hand pose estimation results. */
        draw_2d_texture_ex (&srctex, camx, camy, camw, camh, 0);

        int dx = camx;
        int dy = camy + camh;
        int dw = camw;
        int dh = camh;

        /* visualize the face pose estimation results. */
        render_detect_region (dx, dy, dw, dh, &face_detect_ret);

        for (int face_id = 0; face_id < face_detect_ret.num; face_id ++)
        {
            render_iris_landmark_on_main (dx, dy, dw, dh, &face_detect_ret.faces[face_id],
                                          &face_mesh_ret[face_id], iris_mesh_ret[face_id]);

            render_iris_landmark_on_face (dx, dy, dw, dh, &face_mesh_ret[face_id], iris_mesh_ret[face_id]);
        }


        /* --------------------------------------- *
         *  post process
         * --------------------------------------- */
        glViewport (0, 0, win_w, win_h);

        DrawTFLiteConfigInfo ();

        glClear (GL_COLOR_BUFFER_BIT);

        draw_pmeter (0, 40);

        soundGenerator.stopAudio();

        sprintf (strbuf, "Interval:%5.1f [ms]\nFace :%5.1f [ms]\nMesh :%5.1f [ms]\nIris :%5.1f [ms]\nDepth  :%5.1f [ms]\nDepth  :%5.1f [ms]\nDetect  :%5.1f [ms]",
            interval, invoke_ms0, invoke_ms1, invoke_ms2, depth_invoke_ms, deeplab_invoke_ms, detect_invoke_ms);
        DBG_LOGE("Interval:%5.1f [ms]\nFace :%5.1f [ms]\nMesh :%5.1f [ms]\nIris :%5.1f [ms]\nDepth  :%5.1f [ms]\nSeg  :%5.1f [ms]\nDetect  :%5.1f [ms]\nLaneSeg  :%5.1f [ms]",
                  interval, invoke_ms0, invoke_ms1, invoke_ms2, depth_invoke_ms, deeplab_invoke_ms, detect_invoke_ms, laneseg_invoke_ms);
        //draw_dbgstr (strbuf, 10, 10);

        /* renderer info */
        int y = win_h - 22 * 3;
        draw_dbgstr (glctx.str_glverstion, 10, y); y += 22;
        draw_dbgstr (glctx.str_glvendor,   10, y); y += 22;
        draw_dbgstr (glctx.str_glrender,   10, y); y += 22;

#if defined (USE_IMGUI)
        invoke_imgui (&imgui_data);
#endif
        egl_swap();

    }
    glctx.frame_count ++;
    DBG_LOGE("Rendered Frame:%d \n",
             count);
}


AppEngine::AppEngine (android_app* app)
    : m_app(app),
      m_cameraGranted(false),
      m_camera(nullptr),
      m_camera1(nullptr),
      m_camera_facing(0)
{
    int socket;

    //socket = client.connectToServer();
    memset (&glctx, 0, sizeof (glctx));

}

AppEngine::~AppEngine()
{
    client.disconnectFromServer(socket);
    DeleteCamera();
}


/* ---------------------------------------------------------------------------- *
 *  Interfaces to android application framework
 * ---------------------------------------------------------------------------- */
void
AppEngine::OnAppInitWindow (void)
{
    InitGLES();
    InitCamera();
}

void
AppEngine::OnAppTermWindow (void)
{
    DeleteCamera();
    TerminateGLES();
}

struct android_app *
AppEngine::AndroidApp (void) const
{
    return m_app;
}


/* ---------------------------------------------------------------------------- *
 *  OpenGLES Render Functions
 * ---------------------------------------------------------------------------- */
void
AppEngine::LoadInputTexture (texture_2d_t *tex, char *fname)
{
    int32_t img_w, img_h;
    uint8_t *img_buf = asset_read_image (AndroidApp()->activity->assetManager, fname, &img_w, &img_h);

    create_2d_texture_ex (tex, img_buf, img_w, img_h, pixfmt_fourcc('R', 'G', 'B', 'A'));
    asset_free_image (img_buf);
}

void 
AppEngine::InitGLES (void)
{
    int ret;

    int ret_depth, ret_deeplab, ret_detect , ret_laneseg;

    egl_init_with_window_surface (2, m_app->window, 8, 0, 0);

    glctx.str_glverstion = (char *)glGetString (GL_VERSION);
    glctx.str_glvendor   = (char *)glGetString (GL_VENDOR);
    glctx.str_glrender   = (char *)glGetString (GL_RENDERER);

    int w, h;
    egl_get_current_surface_dimension (&w, &h);

    init_2d_renderer (w, h);
    init_pmeter (w, h, h - 100);
    init_dbgstr (w, h);

    asset_read_file (m_app->activity->assetManager,
                    (char *)FACE_DETECT_MODEL_PATH, m_facedet_tflite_model_buf);

    asset_read_file (m_app->activity->assetManager,
                    (char *)FACE_LANDMARK_MODEL_PATH, m_facelandmark_tflite_model_buf);

    asset_read_file (m_app->activity->assetManager,
                    (char *)IRIS_LANDMARK_MODEL_PATH, m_irislandmark_tflite_model_buf);

    ret = init_tflite_facemesh (
        (const char *)m_facedet_tflite_model_buf.data(), m_facedet_tflite_model_buf.size(),
        (const char *)m_facelandmark_tflite_model_buf.data(), m_facelandmark_tflite_model_buf.size(),
        (const char *)m_irislandmark_tflite_model_buf.data(), m_irislandmark_tflite_model_buf.size());

    asset_read_file (m_app->activity->assetManager,
                     (char *)DENSEDEPTH_MODEL_PATH, m_tflite_depth_model_buf);

    ret_depth = init_tflite_dense_depth (
            (const char *)m_tflite_depth_model_buf.data(), m_tflite_depth_model_buf.size());

    asset_read_file (m_app->activity->assetManager,
                     (char *)DEEPLAB_MODEL_PATH, m_tflite_deeplab_model_buf);

    ret_deeplab = init_tflite_deeplab (
            (const char *)m_tflite_deeplab_model_buf.data(), m_tflite_deeplab_model_buf.size());

    asset_read_file (m_app->activity->assetManager,
                     (char *)LANESEG_MODEL_PATH, m_tflite_laneseg_model_buf);

    ret_laneseg = init_tflite_laneseg (
            (const char *)m_tflite_laneseg_model_buf.data(), m_tflite_laneseg_model_buf.size());

    asset_read_file (m_app->activity->assetManager,
                     (char *)DETECT_MODEL_PATH, m_tflite_detect_model_buf);

    asset_read_file (m_app->activity->assetManager,
                     (char *)LABEL_MAP_PATH, m_detect_label_map_buf);

    ret_detect = init_tflite_detection (
            (const char *)m_tflite_detect_model_buf.data(), m_tflite_detect_model_buf.size(),
            (const char *)m_detect_label_map_buf.data(), m_detect_label_map_buf.size());

    setup_imgui (w, h, &imgui_data);

    glctx.disp_w = w;
    glctx.disp_h = h;
    LoadInputTexture (&glctx.tex_static, (char *)"pakutaso_sotsugyou.jpg");

    LoadInputTexture (&glctx.tex_static1, (char *)"pexels.jpg");

    /* render target for default framebuffer */
    get_render_target (&glctx.rtarget_main);

    /* render target for camera cropping */
    create_render_target (&glctx.rtarget_crop, CAMERA_CROP_WIDTH, CAMERA_CROP_HEIGHT, RTARGET_COLOR);
    glctx.tex_input.texid  = glctx.rtarget_crop.texc_id;
    glctx.tex_input.width  = glctx.rtarget_crop.width;
    glctx.tex_input.height = glctx.rtarget_crop.height;
    glctx.tex_input.format = pixfmt_fourcc('R', 'G', 'B', 'A');

    glctx.initdone = 1;

}


void
AppEngine::TerminateGLES (void)
{
    egl_terminate ();
}


void
AppEngine::UpdateFrame (void)
{
    if (glctx.initdone == 0)
        return;


    if (m_cameraGranted)
    {
        if (m_camera_facing != imgui_data.camera_facing)
        {
            m_camera_facing = imgui_data.camera_facing;
            DeleteCamera ();
            CreateCamera (m_camera_facing);
        }
        UpdateCameraTexture();
    }

    if (m_cameraGranted && glctx.tex_camera_valid == false && glctx.tex_camera1_valid == false)
        return;

    CropCameraTexture ();

    RenderFrame();
}

void
AppEngine::CropCameraTexture (void)
{
    texture_2d_t srctex = glctx.tex_camera;
    texture_2d_t srctex1 = glctx.tex_camera1;
    if (!glctx.tex_camera_valid)
        srctex = glctx.tex_static;

    if (!glctx.tex_camera1_valid)
        srctex1 = glctx.tex_static1;

    /* render to square FBO */
    render_target_t *rtarget = &glctx.rtarget_crop;
    set_render_target (rtarget);
    set_2d_projection_matrix (rtarget->width, rtarget->height);
    glClear (GL_COLOR_BUFFER_BIT);


    int draw_x, draw_y, draw_w, draw_h;
    adjust_texture (rtarget->width, rtarget->height, srctex.width, srctex.height,
                    &draw_x, &draw_y, &draw_w, &draw_h, 1);


    /* when we use inner camera, enable horizontal flip. */
    int flip = m_camera_facing ? RENDER2D_FLIP_H : 0;
    flip |= RENDER2D_FLIP_V;
    draw_2d_texture_ex (&srctex, draw_x, draw_y, draw_w, draw_h, flip);

    /* reset to the default framebuffer */
    rtarget = &glctx.rtarget_main;
    set_render_target (rtarget);
    set_2d_projection_matrix (rtarget->width, rtarget->height);

}


/* ---------------------------------------------------------------------------- *
 *  Manage NDKCamera Functions
 * ---------------------------------------------------------------------------- */
void 
AppEngine::InitCamera (void)
{
    // Not permitted to use camera yet, ask(again) and defer other events
    if (!m_cameraGranted)
    {
        RequestCameraPermission();
        return;
    }

    CreateCamera (m_camera_facing);

}


void
AppEngine::DeleteCamera(void)
{
    if (m_camera)
    {
        delete m_camera;
        m_camera = nullptr;
    }

    if (m_camera1)
    {
        delete m_camera1;
        m_camera1 = nullptr;
    }

    m_ImgReader.ReleaseImageReader ();
    m_ImgReader1.ReleaseImageReader ();
    glctx.tex_camera_valid = false;
    glctx.tex_camera1_valid = false;
}


void 
AppEngine::CreateCamera (int facing)
{
    m_camera = new NDKCamera();
    m_camera1 = new NDKCamera();
    ASSERT (m_camera, "Failed to Create CameraObject");
    ASSERT (m_camera1, "Failed to Create CameraObject");

    m_camera->SelectCameraFacing (facing);

    int facing1 = m_camera_facing ? 0 : 1;

    DBG_LOGE("Cam1:%d,Cam2:%d", facing, facing1);

    m_camera1->SelectCameraFacing (facing1);

    m_ImgReader.InitImageReader (CAMERA_RESOLUTION_W, CAMERA_RESOLUTION_H);
    ANativeWindow *nativeWindow = m_ImgReader.GetNativeWindow();

    m_ImgReader1.InitImageReader (CAMERA_RESOLUTION_W, CAMERA_RESOLUTION_H);
    ANativeWindow *nativeWindow1 = m_ImgReader1.GetNativeWindow();

    m_camera->CreateSession (nativeWindow);
    m_camera->StartPreview (true);

    m_camera1->CreateSession (nativeWindow1);
    m_camera1->StartPreview (true);
}

void
AppEngine::UpdateCameraTexture ()
{
    /* Acquire the latest AHardwareBuffer */
    AHardwareBuffer *ahw_buf = NULL;
    AHardwareBuffer *ahw_buf1 = NULL;
    int ret = m_ImgReader.GetCurrentHWBuffer (&ahw_buf);
    int ret1 = m_ImgReader1.GetCurrentHWBuffer (&ahw_buf1);
    if (ret != 0 && ret1 != 0)
        return;

    /* Get EGLClientBuffer */
    EGLClientBuffer egl_buf = eglGetNativeClientBufferANDROID (ahw_buf);
    EGLClientBuffer egl_buf1 = eglGetNativeClientBufferANDROID (ahw_buf1);
    if (!egl_buf)
    {
        DBG_LOGE("Failed to create EGLClientBuffer");
        return;
    }

    /* (Re)Create EGLImage */
    if (glctx.egl_img != EGL_NO_IMAGE_KHR)
    {
        eglDestroyImageKHR (egl_get_display(), glctx.egl_img);
        glctx.egl_img = EGL_NO_IMAGE_KHR;
    }

    /* (Re)Create EGLImage */
    if (glctx.egl_img1 != EGL_NO_IMAGE_KHR)
    {
        eglDestroyImageKHR (egl_get_display(), glctx.egl_img1);
        glctx.egl_img1 = EGL_NO_IMAGE_KHR;
    }

    EGLint attrs[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE,};
    glctx.egl_img = eglCreateImageKHR (egl_get_display(), EGL_NO_CONTEXT,
                                       EGL_NATIVE_BUFFER_ANDROID, egl_buf, attrs);

    glctx.egl_img1 = eglCreateImageKHR (egl_get_display(), EGL_NO_CONTEXT,
                                       EGL_NATIVE_BUFFER_ANDROID, egl_buf1, attrs);

    /* Bind to GL_TEXTURE_EXTERNAL_OES */
    texture_2d_t *input_tex = &glctx.tex_camera;
    texture_2d_t *input_tex1 = &glctx.tex_camera1;
    if (input_tex->texid == 0)
    {
        GLuint texid;
        glGenTextures (1, &texid);
        glBindTexture (GL_TEXTURE_EXTERNAL_OES, texid);

        glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        input_tex->texid  = texid;
        input_tex->format = pixfmt_fourcc('E', 'X', 'T', 'X');
        m_ImgReader.GetBufferDimension (&input_tex->width, &input_tex->height);
    }

    if (input_tex1->texid == 0)
    {
        GLuint texid1;
        glGenTextures (1, &texid1);
        glBindTexture (GL_TEXTURE_EXTERNAL_OES, texid1);

        glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        input_tex1->texid  = texid1;
        input_tex1->format = pixfmt_fourcc('E', 'X', 'T', 'X');
        m_ImgReader1.GetBufferDimension (&input_tex1->width, &input_tex1->height);
    }

    glBindTexture (GL_TEXTURE_EXTERNAL_OES, input_tex->texid);
    glBindTexture (GL_TEXTURE_EXTERNAL_OES, input_tex1->texid);

    glEGLImageTargetTexture2DOES (GL_TEXTURE_EXTERNAL_OES, glctx.egl_img);
    glEGLImageTargetTexture2DOES (GL_TEXTURE_EXTERNAL_OES, glctx.egl_img1);
    GLASSERT ();

    glctx.tex_camera_valid = true;
    glctx.tex_camera1_valid = true;
}


/* --------------------------------------------------------------------------- *
 * Initiate a Camera Run-time usage request to Java side implementation
 *  [The request result will be passed back in function notifyCameraPermission()]
 * --------------------------------------------------------------------------- */
void
AppEngine::RequestCameraPermission()
{
    if (!AndroidApp())
        return;

    ANativeActivity *activity = AndroidApp()->activity;

    JNIEnv *env;
    activity->vm->GetEnv ((void**)&env, JNI_VERSION_1_6);
    activity->vm->AttachCurrentThread (&env, NULL);

    jobject activityObj = env->NewGlobalRef (activity->clazz);
    jclass clz = env->GetObjectClass (activityObj);

    env->CallVoidMethod (activityObj, env->GetMethodID (clz, "RequestCamera", "()V"));
    env->DeleteGlobalRef (activityObj);

    activity->vm->DetachCurrentThread();
}

void
AppEngine::OnCameraPermission (jboolean granted)
{
    m_cameraGranted = (granted != JNI_FALSE);

    if (m_cameraGranted)
    {
        InitCamera();
    }
}


extern "C" JNIEXPORT void JNICALL
Java_com_glesapp_glesapp_GLESAppNativeActivity_notifyCameraPermission (
                            JNIEnv *env, jclass type, jboolean permission)
{
    std::thread permissionHandler (&AppEngine::OnCameraPermission, GetAppEngine(), permission);
    permissionHandler.detach();
}


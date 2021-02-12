//
// Created by tarzan on 2/8/21.
//
#include "utils.h"
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
#include "render_dense_depth.h"
#include "assertgl.h"
#include "util_matrix.h"
#include "tflite_facemesh.h"
#include "touch_event.h"

/* resize image to (300x300) for input image of MobileNet SSD */
void
feed_detect_image_uint8 (texture_2d_t *srctex, int win_w, int win_h)
{
    int x, y, w, h;
    uint8_t *buf_u8 = (uint8_t *)get_detect_input_buf (&w, &h);
    unsigned char *buf_ui8 = NULL;
    static unsigned char *pui8 = NULL;

    if (pui8 == NULL)
        pui8 = (unsigned char *)malloc(w * h * 4);

    buf_ui8 = pui8;

    draw_2d_texture_ex (srctex, 0, win_h - h, w, h, RENDER2D_FLIP_V);

    glPixelStorei (GL_PACK_ALIGNMENT, 4);
    glReadPixels (0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf_ui8);

    for (y = 0; y < h; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            int r = *buf_ui8 ++;
            int g = *buf_ui8 ++;
            int b = *buf_ui8 ++;
            buf_ui8 ++;          /* skip alpha */

            *buf_u8 ++ = r;
            *buf_u8 ++ = g;
            *buf_u8 ++ = b;
        }
    }

    return;
}

/* resize image to DNN network input size and convert to fp32. */
void
feed_detect_image_float (texture_2d_t *srctex, int win_w, int win_h)
{
    int x, y, w, h;
    float *buf_fp32 = (float *)get_detect_input_buf (&w, &h);
    unsigned char *buf_ui8 = NULL;
    static unsigned char *pui8 = NULL;

    if (pui8 == NULL)
        pui8 = (unsigned char *)malloc(w * h * 4);

    buf_ui8 = pui8;

    draw_2d_texture_ex (srctex, 0, win_h - h, w, h, RENDER2D_FLIP_V);

    glPixelStorei (GL_PACK_ALIGNMENT, 4);
    glReadPixels (0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf_ui8);

    /* convert UI8 [0, 255] ==> FP32 [-1, 1] */
    float mean = 128.0f;
    float std  = 128.0f;
    for (y = 0; y < h; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            int r = *buf_ui8 ++;
            int g = *buf_ui8 ++;
            int b = *buf_ui8 ++;
            buf_ui8 ++;          /* skip alpha */
            *buf_fp32 ++ = (float)(r - mean) / std;
            *buf_fp32 ++ = (float)(g - mean) / std;
            *buf_fp32 ++ = (float)(b - mean) / std;
        }
    }

    return;
}

void
feed_detect_image(texture_2d_t *srctex, int win_w, int win_h)
{
    int type = get_detect_input_type ();
    if (type)
        feed_detect_image_uint8 (srctex, win_w, win_h);
    else
        feed_detect_image_float (srctex, win_w, win_h);
}

/* resize image to DNN network input size and convert to fp32. */
void
feed_deeplab_image(texture_2d_t *srctex, int win_w, int win_h) {
    int x, y, w, h;
    float *buf_fp32 = (float *) get_deeplab_input_buf(&w, &h);
    unsigned char *buf_ui8 = NULL;
    static unsigned char *pui8 = NULL;

    if (pui8 == NULL)
        pui8 = (unsigned char *) malloc(w * h * 4);

    buf_ui8 = pui8;

    draw_2d_texture_ex(srctex, 0, win_h - h, w, h, RENDER2D_FLIP_V);

    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf_ui8);

    /* convert UI8 [0, 255] ==> FP32 [ 0, 1] */
    float mean = 0.0f;
    float std = 255.0f;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            int r = *buf_ui8++;
            int g = *buf_ui8++;
            int b = *buf_ui8++;

            buf_ui8++;          /* skip alpha */
            *buf_fp32++ = (float) (r - mean) / std;
            *buf_fp32++ = (float) (g - mean) / std;
            *buf_fp32++ = (float) (b - mean) / std;
            //DBG_LOGE("%d,%d : (%d, %d, %d)", x, y, r, g, b);
        }
    }



    return;

}

/* resize image to Laneseg network input size and convert to fp32. */
void
feed_laneseg_image(texture_2d_t *srctex, int win_w, int win_h) {
    int x, y, w, h;
    float *buf_fp32 = (float *) get_laneseg_input_buf(&w, &h);
    unsigned char *buf_ui8 = NULL;
    static unsigned char *pui8 = NULL;

    if (pui8 == NULL)
        pui8 = (unsigned char *) malloc(w * h * 4);

    buf_ui8 = pui8;

    draw_2d_texture_ex(srctex, 0, win_h - h, w, h, RENDER2D_FLIP_V);

    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf_ui8);

    /* convert UI8 [0, 255] ==> FP32 [ 0, 1] */
    float mean = 0.0f;
    float std = 255.0f;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            int r = *buf_ui8++;
            int g = *buf_ui8++;
            int b = *buf_ui8++;
            buf_ui8++;          /* skip alpha */
            *buf_fp32++ = (float) (r - mean) / std;
            *buf_fp32++ = (float) (g - mean) / std;
            *buf_fp32++ = (float) (b - mean) / std;
        }
    }

    return;

}

/* resize image to DNN network input size and convert to fp32. */
void
feed_dense_depth_image(texture_2d_t *srctex, int win_w, int win_h)
{
    int x, y, w, h;
    float *buf_fp32 = (float *)get_dense_depth_input_buf (&w, &h);
    unsigned char *buf_ui8 = NULL;
    static unsigned char *pui8 = NULL;

    if (pui8 == NULL)
        pui8 = (unsigned char *)malloc(w * h * 4);

    buf_ui8 = pui8;

    draw_2d_texture_ex (srctex, 0, win_h - h, w, h, RENDER2D_FLIP_V);

    glPixelStorei (GL_PACK_ALIGNMENT, 4);
    glReadPixels (0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf_ui8);

    /* convert UI8 [0, 255] ==> FP32 [-1, 1] */
    float mean = 0.0f; /*128.0f;*/
    float std  = 255.0f; /*128.0f;*/
    for (y = 0; y < h; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            int r = *buf_ui8 ++;
            int g = *buf_ui8 ++;
            int b = *buf_ui8 ++;
            buf_ui8 ++;          /* skip alpha */
            *buf_fp32 ++ = (float)(r - mean) / std;
            *buf_fp32 ++ = (float)(g - mean) / std;
            *buf_fp32 ++ = (float)(b - mean) / std;
        }
    }

    return;
}

/* resize image to DNN network input size and convert to fp32. */
void
feed_face_detect_image(texture_2d_t *srctex, int win_w, int win_h)
{
    int x, y, w, h;
    float *buf_fp32 = (float *)get_face_detect_input_buf (&w, &h);
    unsigned char *buf_ui8 = NULL;
    static unsigned char *pui8 = NULL;

    if (pui8 == NULL)
        pui8 = (unsigned char *)malloc(w * h * 4);

    buf_ui8 = pui8;

    draw_2d_texture_ex (srctex, 0, win_h - h, w, h, RENDER2D_FLIP_V);

    glPixelStorei (GL_PACK_ALIGNMENT, 4);
    glReadPixels (0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf_ui8);

    /* convert UI8 [0, 255] ==> FP32 [-1, 1] */
    float mean = 128.0f;
    float std  = 128.0f;
    for (y = 0; y < h; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            int r = *buf_ui8 ++;
            int g = *buf_ui8 ++;
            int b = *buf_ui8 ++;



            buf_ui8 ++;          /* skip alpha */
            *buf_fp32 ++ = (float)(r - mean) / std;
            *buf_fp32 ++ = (float)(g - mean) / std;
            *buf_fp32 ++ = (float)(b - mean) / std;

            //DBG_LOGE("%d,%d : (%d, %d, %d)", x, y, r, g, b);
        }
    }



    return;
}

void
feed_face_landmark_image(texture_2d_t *srctex, int win_w, int win_h, face_detect_result_t *detection, unsigned int face_id)
{
    int x, y, w, h;
    float *buf_fp32 = (float *)get_facemesh_landmark_input_buf (&w, &h);
    unsigned char *buf_ui8 = NULL;
    static unsigned char *pui8 = NULL;

    if (pui8 == NULL)
        pui8 = (unsigned char *)malloc(w * h * 4);

    buf_ui8 = pui8;

    float texcoord[] = { 0.0f, 1.0f,
                         0.0f, 0.0f,
                         1.0f, 1.0f,
                         1.0f, 0.0f };

    if (detection->num > face_id)
    {
        face_t *face = &(detection->faces[face_id]);
        float x0 = face->face_pos[0].x;
        float y0 = face->face_pos[0].y;
        float x1 = face->face_pos[1].x; //    0--------1
        float y1 = face->face_pos[1].y; //    |        |
        float x2 = face->face_pos[2].x; //    |        |
        float y2 = face->face_pos[2].y; //    3--------2
        float x3 = face->face_pos[3].x;
        float y3 = face->face_pos[3].y;
        texcoord[0] = x3;   texcoord[1] = y3;
        texcoord[2] = x0;   texcoord[3] = y0;
        texcoord[4] = x2;   texcoord[5] = y2;
        texcoord[6] = x1;   texcoord[7] = y1;
    }

    draw_2d_texture_ex_texcoord (srctex, 0, win_h - h, w, h, texcoord);

    glPixelStorei (GL_PACK_ALIGNMENT, 4);
    glReadPixels (0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf_ui8);

    /* convert UI8 [0, 255] ==> FP32 [0, 1] */
    float mean = 0.0f;
    float std  = 255.0f;
    for (y = 0; y < h; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            int r = *buf_ui8 ++;
            int g = *buf_ui8 ++;
            int b = *buf_ui8 ++;

            buf_ui8 ++;          /* skip alpha */
            *buf_fp32 ++ = (float)(r - mean) / std;
            *buf_fp32 ++ = (float)(g - mean) / std;
            *buf_fp32 ++ = (float)(b - mean) / std;
        }
    }

    return;
}


void
feed_iris_landmark_image(texture_2d_t *srctex, int win_w, int win_h,
                         face_t *face, face_landmark_result_t *facemesh, int eye_id)
{
    int x, y, w, h;
    float *buf_fp32 = (float *)get_irismesh_landmark_input_buf (&w, &h);
    unsigned char *buf_ui8 = NULL;
    static unsigned char *pui8 = NULL;

    if (pui8 == NULL)
        pui8 = (unsigned char *)malloc(w * h * 4);

    buf_ui8 = pui8;

    float texcoord[8];

    float scale_x = face->face_w;
    float scale_y = face->face_h;
    float pivot_x = face->face_cx;
    float pivot_y = face->face_cy;
    float rotation= face->rotation;

    float x0 = facemesh->eye_pos[eye_id][0].x;
    float y0 = facemesh->eye_pos[eye_id][0].y;
    float x1 = facemesh->eye_pos[eye_id][1].x; //    0--------1
    float y1 = facemesh->eye_pos[eye_id][1].y; //    |        |
    float x2 = facemesh->eye_pos[eye_id][2].x; //    |        |
    float y2 = facemesh->eye_pos[eye_id][2].y; //    3--------2
    float x3 = facemesh->eye_pos[eye_id][3].x;
    float y3 = facemesh->eye_pos[eye_id][3].y;

    float mat[16];
    float vec[4][2] = {{x0, y0}, {x1, y1}, {x2, y2}, {x3, y3}};
    matrix_identity (mat);

    matrix_translate (mat, pivot_x, pivot_y, 0);
    matrix_rotate (mat, RAD_TO_DEG(rotation), 0, 0, 1);
    matrix_scale (mat, scale_x, scale_y, 1.0f);
    matrix_translate (mat, -0.5f, -0.5f, 0);

    matrix_multvec2 (mat, vec[0], vec[0]);
    matrix_multvec2 (mat, vec[1], vec[1]);
    matrix_multvec2 (mat, vec[2], vec[2]);
    matrix_multvec2 (mat, vec[3], vec[3]);

    x0 = vec[0][0];  y0 = vec[0][1];
    x1 = vec[1][0];  y1 = vec[1][1];
    x2 = vec[2][0];  y2 = vec[2][1];
    x3 = vec[3][0];  y3 = vec[3][1];

    /* Upside down */
    if (eye_id == 0)
    {
        texcoord[0] = x3;   texcoord[1] = y3;
        texcoord[2] = x0;   texcoord[3] = y0;
        texcoord[4] = x2;   texcoord[5] = y2;
        texcoord[6] = x1;   texcoord[7] = y1;
    }
    else /* need to horizontal flip for right eye */
    {
        texcoord[0] = x2;   texcoord[1] = y2;
        texcoord[2] = x1;   texcoord[3] = y1;
        texcoord[4] = x3;   texcoord[5] = y3;
        texcoord[6] = x0;   texcoord[7] = y0;
    }

    draw_2d_texture_ex_texcoord (srctex, 0, win_h - h, w, h, texcoord);

    glPixelStorei (GL_PACK_ALIGNMENT, 4);
    glReadPixels (0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf_ui8);

    /* convert UI8 [0, 255] ==> FP32 [-1, 1] */
    float mean = 0.0f;
    float std  = 255.0f;
    for (y = 0; y < h; y ++)
    {
        for (x = 0; x < w; x ++)
        {
            int r = *buf_ui8 ++;
            int g = *buf_ui8 ++;
            int b = *buf_ui8 ++;
            buf_ui8 ++;          /* skip alpha */
            *buf_fp32 ++ = (float)(r - mean) / std;
            *buf_fp32 ++ = (float)(g - mean) / std;
            *buf_fp32 ++ = (float)(b - mean) / std;
        }
    }

    return;
}

void
render_deeplab_result (int ofstx, int ofsty, int draw_w, int draw_h,
                       deeplab_result_t *deeplab_ret)
{
    float *segmap = deeplab_ret->segmentmap;
    int segmap_w  = deeplab_ret->segmentmap_dims[0];
    int segmap_h  = deeplab_ret->segmentmap_dims[1];
    int segmap_c  = deeplab_ret->segmentmap_dims[2];
    int x, y, c;
    unsigned int imgbuf[segmap_h][segmap_w];

    /* find the most confident class for each pixel. */
    for (y = 0; y < segmap_h; y ++)
    {
        for (x = 0; x < segmap_w; x ++)
        {
            int max_id;
            float conf_max = 0.0;
            for (c = 0; c < 21; c ++)
            {
                float confidence = segmap[(y * segmap_w * segmap_c)+ (x * segmap_c) + c];
                if (c == 0 || confidence > conf_max)
                {
                    conf_max = confidence;
                    max_id = c;
                }
            }
            float *col = get_deeplab_class_color (max_id);
            unsigned char r = ((int)(col[0] * 255)) & 0xff;
            unsigned char g = ((int)(col[1] * 255)) & 0xff;
            unsigned char b = ((int)(col[2] * 255)) & 0xff;
            unsigned char a = ((int)(col[3] * 255)) & 0xff;
            imgbuf[y][x] = (a << 24) | (b << 16) | (g << 8) | (r);
        }
    }

    GLuint texid;
    glGenTextures (1, &texid );
    glBindTexture (GL_TEXTURE_2D, texid);

    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei (GL_UNPACK_ALIGNMENT, 4);

    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA,
                  segmap_w, segmap_h, 0, GL_RGBA,
                  GL_UNSIGNED_BYTE, imgbuf);

    draw_2d_texture (texid, ofstx, ofsty, draw_w, draw_h, 0);

    /* class name */
    for (c = 0; c < 21; c ++)
    {
        float col_str[] = {1.0f, 1.0f, 1.0f, 1.0f};
        float *col = get_deeplab_class_color (c);
        char *name = get_deeplab_class_name (c);
        char buf[512];
        sprintf (buf, "%2d:%s", c, name);
        draw_dbgstr_ex (buf, ofstx, ofsty + c * 22 * 0.7, 0.7f, col_str, col);
    }

    glDeleteTextures (1, &texid);
}

void
render_laneseg_result (int ofstx, int ofsty, int draw_w, int draw_h,
                       laneseg_result_t *laneseg_ret)
{
    float *segmap = laneseg_ret->lanemap;
    float *binmap = laneseg_ret->bin_lanemap;
    int segmap_w  = laneseg_ret->segmentmap_dims[0];
    int segmap_h  = laneseg_ret->segmentmap_dims[1];
    int segmap_c  = laneseg_ret->segmentmap_dims[2];
    int x, y, c;
    unsigned int imgbuf[segmap_h][segmap_w];

    /* find the most confident class for each pixel. */
    for (y = 0; y < segmap_h; y ++)
    {
        for (x = 0; x < segmap_w; x ++)
        {
            int max_id;
            for (c = 0; c < 5; c ++)
            {
                float presence = binmap[(y * segmap_w)+x];
                //DBG_LOGE("%5.1f", presence);
                float confidence = segmap[(y * segmap_w * segmap_c)+ (x * segmap_c) + c];
                if (c == 0 || presence > 0.9 && confidence > 0)
                {
                    max_id = c;
                }
            }
            float *col = get_laneseg_class_color (max_id);
            unsigned char r = ((int)(col[0] * 255)) & 0xff;
            unsigned char g = ((int)(col[1] * 255)) & 0xff;
            unsigned char b = ((int)(col[2] * 255)) & 0xff;
            unsigned char a = ((int)(col[3] * 255)) & 0xff;
            imgbuf[y][x] = (a << 24) | (b << 16) | (g << 8) | (r);
        }
    }

    GLuint texid;
    glGenTextures (1, &texid );
    glBindTexture (GL_TEXTURE_2D, texid);

    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei (GL_UNPACK_ALIGNMENT, 4);

    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA,
                  segmap_w, segmap_h, 0, GL_RGBA,
                  GL_UNSIGNED_BYTE, imgbuf);

    draw_2d_texture (texid, ofstx, ofsty, draw_w, draw_h, 0);

    /* class name */
    for (c = 0; c < 5; c ++)
    {
        float col_str[] = {1.0f, 1.0f, 1.0f, 1.0f};
        float *col = get_laneseg_class_color (c);
        char *name = get_laneseg_class_name (c);
        char buf[512];
        sprintf (buf, "%2d:%s", c, name);
        draw_dbgstr_ex (buf, ofstx, ofsty + c * 22 * 0.7, 0.7f, col_str, col);
    }

    glDeleteTextures (1, &texid);
}

void
render_obj_detect_region (int ofstx, int ofsty, int texw, int texh,
                      detect_result_t *detection)
{
    float col_white[] = {1.0f, 1.0f, 1.0f, 1.0f};

    for (int i = 0; i < detection->num; i ++)
    {
        float x1 = detection->obj[i].x1 * texw + ofstx;
        float y1 = detection->obj[i].y1 * texh + ofsty;
        float x2 = detection->obj[i].x2 * texw + ofstx;
        float y2 = detection->obj[i].y2 * texh + ofsty;
        float score   = detection->obj[i].score;
        int det_class = detection->obj[i].det_class;

        /* rectangle region */
        float *col = get_detect_class_color(det_class);
        draw_2d_rect (x1, y1, x2-x1, y2-y1, col, 2.0f);

        /* class name */
        char *name = get_detect_class_name (det_class);
        char buf[512];
        sprintf (buf, "%s(%d)", name, (int)(score * 100));
        draw_dbgstr_ex (buf, x1, y1, 1.0f, col_white, col);
    }
}

void
render_deeplab_heatmap (texture_2d_t *srctex, int ofstx, int ofsty, int draw_w, int draw_h, deeplab_result_t *deeplab_ret)
{
    float *segmap = deeplab_ret->segmentmap;
    int segmap_w  = deeplab_ret->segmentmap_dims[0];
    int segmap_h  = deeplab_ret->segmentmap_dims[1];
    int segmap_c  = deeplab_ret->segmentmap_dims[2];
    int x, y;
    unsigned char imgbuf[segmap_h][segmap_w];
    static int s_count = 0;
    int key_id = (s_count /10)% 21;
    s_count ++;
    float conf_min, conf_max;


#if 1
    conf_min =  0.0f;
    conf_max = 50.0f;
#else
    conf_min =  FLT_MAX;
    conf_max = -FLT_MAX;
    for (y = 0; y < segmap_h; y ++)
    {
        for (x = 0; x < segmap_w; x ++)
        {
            float confidence = segmap[(y * segmap_w * segmap_c)+ (x * segmap_c) + key_id];
            if (confidence < conf_min) conf_min = confidence;
            if (confidence > conf_max) conf_max = confidence;
        }
    }
#endif

    for (y = 0; y < segmap_h; y ++)
    {
        for (x = 0; x < segmap_w; x ++)
        {
            float confidence = segmap[(y * segmap_w * segmap_c)+ (x * segmap_c) + key_id];
            confidence = (confidence - conf_min) / (conf_max - conf_min);
            if (confidence < 0.0f) confidence = 0.0f;
            if (confidence > 1.0f) confidence = 1.0f;
            imgbuf[y][x] = confidence * 255;
        }
    }


    glBindTexture (GL_TEXTURE_2D, srctex->texid);

    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D (GL_TEXTURE_2D, 0, GL_LUMINANCE,
                  segmap_w, segmap_h, 0, GL_LUMINANCE,
                  GL_UNSIGNED_BYTE, imgbuf);

    draw_2d_colormap (srctex->texid, ofstx, ofsty, draw_w, draw_h, 0.8f, 0);

    {
        char strbuf[128];
        sprintf (strbuf, "%2d (%f, %f) %s\n", key_id,
                 conf_min, conf_max, get_deeplab_class_name (key_id));
        draw_dbgstr (strbuf, ofstx + 5, 5);
    }
}

void
render_detect_region (int ofstx, int ofsty, int texw, int texh,
                      face_detect_result_t *detection)
{
    float col_red[]   = {1.0f, 0.0f, 0.0f, 1.0f};

    for (int i = 0; i < detection->num; i ++)
    {
        face_t *face = &(detection->faces[i]);
        float x1 = face->topleft.x  * texw + ofstx;
        float y1 = face->topleft.y  * texh + ofsty;
        float x2 = face->btmright.x * texw + ofstx;
        float y2 = face->btmright.y * texh + ofsty;

        /* rectangle region */
        draw_2d_rect (x1, y1, x2-x1, y2-y1, col_red, 2.0f);

#if 0
        float col_white[] = {1.0f, 1.0f, 1.0f, 1.0f};
        float score = face->score;

        /* detect score */
        char buf[512];
        sprintf (buf, "%d", (int)(score * 100));
        draw_dbgstr_ex (buf, x1, y1, 1.0f, col_white, col_red);

        /* key points */
        for (int j = 0; j < kFaceKeyNum; j ++)
        {
            float x = face->keys[j].x * texw + ofstx;
            float y = face->keys[j].y * texh + ofsty;

            int r = 4;
            draw_2d_fillrect (x - (r/2), y - (r/2), r, r, col_red);
        }
#endif
    }
}



void
render_cropped_face_image (texture_2d_t *srctex, int ofstx, int ofsty, int texw, int texh,
                           face_detect_result_t *detection, unsigned int face_id)
{
    float texcoord[8];

    if (detection->num <= face_id)
        return;

    face_t *face = &(detection->faces[face_id]);
    float x0 = face->face_pos[0].x;
    float y0 = face->face_pos[0].y;
    float x1 = face->face_pos[1].x; //    0--------1
    float y1 = face->face_pos[1].y; //    |        |
    float x2 = face->face_pos[2].x; //    |        |
    float y2 = face->face_pos[2].y; //    3--------2
    float x3 = face->face_pos[3].x;
    float y3 = face->face_pos[3].y;
    texcoord[0] = x0;   texcoord[1] = y0;
    texcoord[2] = x3;   texcoord[3] = y3;
    texcoord[4] = x1;   texcoord[5] = y1;
    texcoord[6] = x2;   texcoord[7] = y2;

    draw_2d_texture_ex_texcoord (srctex, ofstx, ofsty, texw, texh, texcoord);
}

void
render_cropped_eye_image (texture_2d_t *srctex, int ofstx, int ofsty, int texw, int texh,
                          face_t *face, face_landmark_result_t *facemesh, int eye_id)
{
    float texcoord[8];

    float scale_x = face->face_w;
    float scale_y = face->face_h;
    float pivot_x = face->face_cx;
    float pivot_y = face->face_cy;
    float rotation= face->rotation;

    float x0 = facemesh->eye_pos[eye_id][0].x;
    float y0 = facemesh->eye_pos[eye_id][0].y;
    float x1 = facemesh->eye_pos[eye_id][1].x; //    0--------1
    float y1 = facemesh->eye_pos[eye_id][1].y; //    |        |
    float x2 = facemesh->eye_pos[eye_id][2].x; //    |        |
    float y2 = facemesh->eye_pos[eye_id][2].y; //    3--------2
    float x3 = facemesh->eye_pos[eye_id][3].x;
    float y3 = facemesh->eye_pos[eye_id][3].y;

    float mat[16];
    float vec[4][2] = {{x0, y0}, {x1, y1}, {x2, y2}, {x3, y3}};
    matrix_identity (mat);

    matrix_translate (mat, pivot_x, pivot_y, 0);
    matrix_rotate (mat, RAD_TO_DEG(rotation), 0, 0, 1);
    matrix_scale (mat, scale_x, scale_y, 1.0f);
    matrix_translate (mat, -0.5f, -0.5f, 0);

    matrix_multvec2 (mat, vec[0], vec[0]);
    matrix_multvec2 (mat, vec[1], vec[1]);
    matrix_multvec2 (mat, vec[2], vec[2]);
    matrix_multvec2 (mat, vec[3], vec[3]);

    x0 = vec[0][0];  y0 = vec[0][1];
    x1 = vec[1][0];  y1 = vec[1][1];
    x2 = vec[2][0];  y2 = vec[2][1];
    x3 = vec[3][0];  y3 = vec[3][1];

    texcoord[0] = x0;   texcoord[1] = y0;
    texcoord[2] = x3;   texcoord[3] = y3;
    texcoord[4] = x1;   texcoord[5] = y1;
    texcoord[6] = x2;   texcoord[7] = y2;

    draw_2d_texture_ex_texcoord (srctex, ofstx, ofsty, texw, texh, texcoord);
}


void
render_lines (int ofstx, int ofsty, int texw, int texh, float *mat, irismesh_result_t *irismesh, int *idx, int num)
{
    float col_red  [] = {1.0f, 0.0f, 0.0f, 1.0f};
    fvec3 *eye  = irismesh->eye_landmark;

    for (int i = 1; i < num; i ++)
    {
        float vec0[] = {eye[idx[i-1]].x, eye[idx[i-1]].y};
        float vec1[] = {eye[idx[i  ]].x, eye[idx[i  ]].y};
        matrix_multvec2 (mat, vec0, vec0);
        matrix_multvec2 (mat, vec1, vec1);
        float x0 = vec0[0] * texw + ofstx;   float y0 = vec0[1] * texh + ofsty;
        float x1 = vec1[0] * texw + ofstx;   float y1 = vec1[1] * texh + ofsty;

        draw_2d_line (x0, y0, x1, y1, col_red, 4.0f);
    }
}

void
render_iris_landmark (int ofstx, int ofsty, int texw, int texh, irismesh_result_t *irismesh)
{
    float col_red  [] = {1.0f, 0.0f, 0.0f, 1.0f};
    float col_green[] = {0.0f, 1.0f, 0.0f, 1.0f};
    fvec3 *eye  = irismesh->eye_landmark;
    fvec3 *iris = irismesh->iris_landmark;
    float mat[16];

    matrix_identity (mat);

    for (int i = 0; i < 71; i ++)
    {
        float x = eye[i].x * texw + ofstx;;
        float y = eye[i].y * texh + ofsty;;

        int r = 4;
        draw_2d_fillrect (x - (r/2), y - (r/2), r, r, col_red);
    }

    int eye_idx0[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    int idx_num0 = sizeof(eye_idx0) / sizeof(int);
    render_lines (ofstx, ofsty, texw, texh, mat, irismesh, eye_idx0, idx_num0);

    int eye_idx1[] = {0, 9, 10, 11, 12, 13, 14, 15, 8};
    int idx_num1 = sizeof(eye_idx1) / sizeof(int);
    render_lines (ofstx, ofsty, texw, texh, mat, irismesh, eye_idx1, idx_num1);

    for (int i = 0; i < 5; i ++)
    {
        float x = iris[i].x * texw + ofstx;;
        float y = iris[i].y * texh + ofsty;;

        int r = 4;
        draw_2d_fillrect (x - (r/2), y - (r/2), r, r, col_green);
    }

    {
        float x0 = iris[0].x * texw + ofstx; float y0 = iris[0].y * texh + ofsty;
        float x1 = iris[1].x * texw + ofstx; float y1 = iris[1].y * texh + ofsty;
        float len = sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
        draw_2d_circle (x0, y0, len, col_green, 4);
    }
}

void
render_iris_landmark_on_face (int ofstx, int ofsty, int texw, int texh,
                              face_landmark_result_t *facemesh, irismesh_result_t *irismesh)
{
    float col_green[] = {0.0f, 1.0f, 0.0f, 1.0f};

    for (int eye_id = 0; eye_id < 2; eye_id ++)
    {
        fvec3 *iris = irismesh[eye_id].iris_landmark;

        eye_region_t *eye_rgn = &facemesh->eye_rgn[eye_id];
        float scale_x = eye_rgn->size.x;
        float scale_y = eye_rgn->size.y;
        float pivot_x = eye_rgn->center.x;
        float pivot_y = eye_rgn->center.y;
        float rotation= eye_rgn->rotation;

        float mat[16];
        matrix_identity (mat);
        matrix_translate (mat, pivot_x, pivot_y, 0);
        matrix_rotate (mat, RAD_TO_DEG(rotation), 0, 0, 1);
        matrix_scale (mat, scale_x, scale_y, 1.0f);
        matrix_translate (mat, -0.5f, -0.5f, 0);

        if (0)
        {
            float col_red  [] = {1.0f, 0.0f, 0.0f, 1.0f};
            fvec3 *eye  = irismesh[eye_id].eye_landmark;
            for (int i = 0; i < 71; i ++)
            {
                float vec[2] = {eye[i].x, eye[i].y};
                matrix_multvec2 (mat, vec, vec);

                float x = vec[0] * texw + ofstx;;
                float y = vec[1] * texh + ofsty;;

                int r = 2;
                draw_2d_fillrect (x - (r/2), y - (r/2), r, r, col_red);
            }
        }

        /* iris circle */
        for (int i = 0; i < 5; i ++)
        {
            float vec[2] = {iris[i].x, iris[i].y};
            matrix_multvec2 (mat, vec, vec);

            float x = vec[0] * texw + ofstx;
            float y = vec[1] * texh + ofsty;

            int r = 4;
            draw_2d_fillrect (x - (r/2), y - (r/2), r, r, col_green);
        }

        /* eye region boundary box */
        {
            float x0 = facemesh->eye_pos[eye_id][0].x * texw + ofstx;
            float y0 = facemesh->eye_pos[eye_id][0].y * texh + ofsty;
            float x1 = facemesh->eye_pos[eye_id][1].x * texw + ofstx; //    0--------1
            float y1 = facemesh->eye_pos[eye_id][1].y * texh + ofsty; //    |        |
            float x2 = facemesh->eye_pos[eye_id][2].x * texw + ofstx; //    |        |
            float y2 = facemesh->eye_pos[eye_id][2].y * texh + ofsty; //    3--------2
            float x3 = facemesh->eye_pos[eye_id][3].x * texw + ofstx;
            float y3 = facemesh->eye_pos[eye_id][3].y * texh + ofsty;

            float col_red[] = {1.0f, 0.0f, 0.0f, 1.0f};
            draw_2d_line (x0, y0, x1, y1, col_red, 1.0f);
            draw_2d_line (x1, y1, x2, y2, col_red, 1.0f);
            draw_2d_line (x2, y2, x3, y3, col_red, 1.0f);
            draw_2d_line (x3, y3, x0, y0, col_red, 1.0f);
        }
    }
}

void
render_facemesh_keypoint (int ofstx, int ofsty, int texw, int texh, float *mat, fvec3 *joint, int idx)
{
    float col_cyan[] = {0.0f, 1.0f, 1.0f, 1.0f};

    float vec[2] = {joint[idx].x, joint[idx].y};
    matrix_multvec2 (mat, vec, vec);

    float x = vec[0] * texw + ofstx;
    float y = vec[1] * texh + ofsty;

    int r = 4;
    draw_2d_fillrect (x - (r/2), y - (r/2), r, r, col_cyan);
}

void
render_iris_landmark_on_main (int ofstx, int ofsty, int texw, int texh,
                              face_t *face, face_landmark_result_t *facemesh, irismesh_result_t *irismesh)
{
    float col_green[] = {0.0f, 1.0f, 0.0f, 1.0f};

    float mat_face[16];
    {
        float scale_x = face->face_w;
        float scale_y = face->face_h;
        float pivot_x = face->face_cx;
        float pivot_y = face->face_cy;
        float rotation= face->rotation;

        matrix_identity (mat_face);
        matrix_translate (mat_face, pivot_x, pivot_y, 0);
        matrix_rotate (mat_face, RAD_TO_DEG(rotation), 0, 0, 1);
        matrix_scale (mat_face, scale_x, scale_y, 1.0f);
        matrix_translate (mat_face, -0.5f, -0.5f, 0);
    }

    int key_idx[] = {1, 9, 10, 152, 78, 308, 234, 454};
    int key_num = sizeof(key_idx) / sizeof(int);
    for (int i = 0; i < key_num; i ++)
        render_facemesh_keypoint (ofstx, ofsty, texw, texh, mat_face, facemesh->joint, key_idx[i]);

    for (int eye_id = 0; eye_id < 2; eye_id ++)
    {
        fvec3 *iris = irismesh[eye_id].iris_landmark;

        float mat_eye[16];
        {
            eye_region_t *eye_rgn = &facemesh->eye_rgn[eye_id];
            float scale_x = eye_rgn->size.x;
            float scale_y = eye_rgn->size.y;
            float pivot_x = eye_rgn->center.x;
            float pivot_y = eye_rgn->center.y;
            float rotation= eye_rgn->rotation;

            matrix_identity (mat_eye);
            matrix_translate (mat_eye, pivot_x, pivot_y, 0);
            matrix_rotate (mat_eye, RAD_TO_DEG(rotation), 0, 0, 1);
            matrix_scale (mat_eye, scale_x, scale_y, 1.0f);
            matrix_translate (mat_eye, -0.5f, -0.5f, 0);
        }

        float mat[16];
        matrix_mult (mat, mat_face, mat_eye);

        int eye_idx0[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
        int idx_num0 = sizeof(eye_idx0) / sizeof(int);
        render_lines (ofstx, ofsty, texw, texh, mat, &irismesh[eye_id], eye_idx0, idx_num0);

        int eye_idx1[] = {0, 9, 10, 11, 12, 13, 14, 15, 8};
        int idx_num1 = sizeof(eye_idx1) / sizeof(int);
        render_lines (ofstx, ofsty, texw, texh, mat, &irismesh[eye_id], eye_idx1, idx_num1);

        if (0)
        {
            float col_red  [] = {1.0f, 0.0f, 0.0f, 1.0f};
            fvec3 *eye  = irismesh[eye_id].eye_landmark;
            for (int i = 0; i < 71; i ++)
            {
                float vec[2] = {eye[i].x, eye[i].y};
                matrix_multvec2 (mat, vec, vec);

                float x = vec[0] * texw + ofstx;;
                float y = vec[1] * texh + ofsty;;

                int r = 4;
                draw_2d_fillrect (x - (r/2), y - (r/2), r, r, col_red);
            }
        }

        /* iris circle */
        for (int i = 0; i < 5; i ++)
        {
            float vec[2] = {iris[i].x, iris[i].y};
            matrix_multvec2 (mat, vec, vec);

            float x = vec[0] * texw + ofstx;
            float y = vec[1] * texh + ofsty;

            int r = 4;
            draw_2d_fillrect (x - (r/2), y - (r/2), r, r, col_green);
        }

        {
            float vec0[2] = {iris[0].x, iris[0].y};
            float vec1[2] = {iris[1].x, iris[1].y};
            matrix_multvec2 (mat, vec0, vec0);
            matrix_multvec2 (mat, vec1, vec1);

            float x0 = vec0[0] * texw + ofstx;
            float y0 = vec0[1] * texh + ofsty;
            float x1 = vec1[0] * texw + ofstx;
            float y1 = vec1[1] * texh + ofsty;

            float len = sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
            draw_2d_circle (x0, y0, len, col_green, 4);
        }
    }
}

void
flip_horizontal_iris_landmark (irismesh_result_t *irismesh)
{
    fvec3 *eye  = irismesh->eye_landmark;
    fvec3 *iris = irismesh->iris_landmark;

    for (int i = 0; i < 71; i ++)
    {
        eye[i].x = 1.0f - eye[i].x;
    }

    for (int i = 0; i < 5; i ++)
    {
        iris[i].x = 1.0f - iris[i].x;
    }

}

void
render_depth_image_3d (texture_2d_t *dsrctex, imgui_data_t s_gui_prop, int ofstx, int ofsty, int texw, int texh,
                       dense_depth_result_t *dense_depth_ret)
{
    float mtxGlobal[16], mtxTouch[16];
    static int s_is_first_render3d = 1;
    static mesh_obj_t s_depth_mesh;

    get_touch_event_matrix (mtxTouch);
    matrix_identity (mtxGlobal);
    matrix_translate (mtxGlobal, 0, 0, -s_gui_prop.camera_pos_z);
    matrix_mult (mtxGlobal, mtxGlobal, mtxTouch);



    float *depthmap = dense_depth_ret->depthmap;
    int depthmap_w  = dense_depth_ret->depthmap_dims[0];
    int depthmap_h  = dense_depth_ret->depthmap_dims[1];

    /* create mesh object */
    if (s_is_first_render3d)
    {
        create_mesh (&s_depth_mesh, depthmap_w - 1, depthmap_h - 1);
        s_is_first_render3d = 0;
    }

    float *vtx = s_depth_mesh.vtx_array;
    float *uv  = s_depth_mesh.uv_array;

    /* create 3D vertex coordinate */
    for (int y = 0; y < depthmap_h; y ++)
    {
        for (int x = 0; x < depthmap_w; x ++)
        {
            int   idx = (y * depthmap_w + x);
            float d = depthmap[idx];

            if (1)
            {
                d -= 0;//s_gui_prop.depth_min;
                d /= 1;//s_gui_prop.depth_max;
                d = (d * 1.0 - 0.0) * s_gui_prop.pose_scale_z;
            }
            else
            {
                //d = s_gui_prop.depth_max / d;   //  inf -> 1.0
                //d = 2 - d;                      // -inf -> 1.0
                //d = d * s_gui_prop.pose_scale_z;
            }

            vtx[3 * idx + 0] =  ((x / (float)depthmap_h) * 2.0f - 1.0f) * s_gui_prop.pose_scale_x;
            vtx[3 * idx + 1] = -((y / (float)depthmap_h) * 2.0f - 1.0f) * s_gui_prop.pose_scale_y;
            vtx[3 * idx + 2] =  d;

            uv [2 * idx + 0] = x / (float)depthmap_w;
            uv [2 * idx + 1] = y / (float)depthmap_h;
        }
    }

    float colb[] = {1.0, 1.0, 1.0, 1.0};
    draw_point_arrays (mtxGlobal, vtx, uv, depthmap_h * depthmap_w, dsrctex->texid, colb);

    if (s_gui_prop.draw_axis)
    {
        /* (xyz)-AXIS */
        for (int i = -1; i <= 1; i ++)
        {
            for (int j = -1; j <= 1; j ++)
            {
                float col_base[] = {0.1, 0.5, 0.5, 0.5};
                float dx = s_gui_prop.pose_scale_x;
                float dy = s_gui_prop.pose_scale_y;
                float dz = s_gui_prop.pose_scale_z;

                {
                    float v0[3] = {-dx, i * dy, j * dz};
                    float v1[3] = { dx, i * dy, j * dz};
                    float col_red[] = {1.0, 0.0, 0.0, 1.0};
                    float *col = (i == 0 && j == 0) ? col_red : col_base;
                    draw_line (mtxGlobal, v0, v1, col);
                    DBG_LOGE("s_gui_prop.camera_pos_z: %d", s_gui_prop.camera_pos_z);
                }
                {
                    float v0[3] = {i * dx, -dy, j * dz};
                    float v1[3] = {i * dx,  dy, j * dz};
                    float col_green[] = {0.0, 1.0, 0.0, 1.0};
                    float *col = (i == 0 && j == 0) ? col_green : col_base;
                    draw_line (mtxGlobal, v0, v1, col);
                }
                {
                    float v0[3] = {i * dx, j * dy, -dz};
                    float v1[3] = {i * dx, j * dy,  dz};
                    float col_blue[] = {0.0, 0.0, 1.0, 1.0};
                    float *col = (i == 0 && j == 0) ? col_blue : col_base;
                    draw_line (mtxGlobal, v0, v1, col);
                }
            }
        }
    }
    DBG_LOGE("s_gui_prop.camera_pos_z: %d", s_gui_prop.camera_pos_z);
}

float
fclampf (float val)
{
    val = fmaxf (0.0f, val);
    val = fminf (1.0f, val);
    return val;
}

void
render_depth_image (texture_2d_t *srctex, int ofstx, int ofsty, int texw, int texh,
                    dense_depth_result_t *dense_depth_ret)
{
    float *depthmap = dense_depth_ret->depthmap;
    int depthmap_w  = dense_depth_ret->depthmap_dims[0];
    int depthmap_h  = dense_depth_ret->depthmap_dims[1];
    int x, y;
    unsigned int imgbuf[depthmap_h][depthmap_w];

    /* find the most confident class for each pixel. */
    for (y = 0; y < depthmap_h; y ++)
    {
        for (x = 0; x < depthmap_w; x ++)
        {
            float d = depthmap[y * depthmap_w + x];
            d -= 0.0;
            d /= 1.0;
            d = fclampf (d);

            unsigned char r = d * 255;
            unsigned char g = r;
            unsigned char b = r;
            unsigned char a = 255;

            imgbuf[y][x] = (a << 24) | (b << 16) | (g << 8) | (r);
        }
    }

    texture_2d_t animtex;
    create_2d_texture_ex (&animtex, imgbuf, depthmap_w, depthmap_h, pixfmt_fourcc ('R', 'G', 'B', 'A'));
    draw_2d_texture_ex (&animtex, ofstx, ofsty, texw, texh, 0);

    glDeleteTextures (1, &animtex.texid);
}

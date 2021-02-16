/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#include <cstdio>
#include <cmath>
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
#include "sense.h"

#define UNUSED(x) (void)(x)

#define CAMERA_RESOLUTION_W     640
#define CAMERA_RESOLUTION_H     480
#define CAMERA_CROP_WIDTH       480 /* make a src image square */
#define CAMERA_CROP_HEIGHT      480 /* make a src image square */



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


static void
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



static void
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

float GetDepth(float x0, float y0, float x1, float y1) {
    return std::sqrt((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1));
}

float GetLandmarkDepth(fvec3 ld0,
                       fvec3 ld1,
                       int tex_w, int tex_h) {
    return GetDepth(ld0.x * tex_w, ld0.y * tex_h,
                    ld1.x * tex_w, ld1.y * tex_h);
}

void CalculateEAR(irismesh_result_t *irismesh, int tex_w, int tex_h) {

    float length = GetLandmarkDepth(irismesh->eye_landmark[0],
                                          irismesh->eye_landmark[8], tex_w, tex_h);
    float breadth1 = GetLandmarkDepth(irismesh->eye_landmark[12],
    irismesh->eye_landmark[3], tex_w, tex_h);
    float breadth2 = GetLandmarkDepth(irismesh->eye_landmark[12],
    irismesh->eye_landmark[4], tex_w, tex_h);

    auto EAR = (breadth1 + breadth2) / (2 * length);

    irismesh->EAR = EAR;
}

void CalculateMAR(face_landmark_result_t *facemesh, int tex_w, int tex_h){

    float length = GetLandmarkDepth(facemesh->joint[78],
                                          facemesh->joint[308], tex_w, tex_h);
    float breadth1 = GetLandmarkDepth(facemesh->joint[82],
                                            facemesh->joint[87], tex_w, tex_h);
    float breadth2 = GetLandmarkDepth(facemesh->joint[312],
                                            facemesh->joint[317], tex_w, tex_h);

    auto MAR = (breadth1 + breadth2) / (2 * length);

    facemesh->MAR = MAR;

}

void CalculateDev(face_landmark_result_t *facemesh, int tex_w, int tex_h){

    float x_face_cntr = 0.5;//(facemesh->joint[234].x +
                          //                facemesh->joint[10].x + facemesh->joint[152].x +
                            //                                       facemesh->joint[454].x)/4;
    float y_face_cntr = 0.5;//(facemesh->joint[234].y +
                         //facemesh->joint[10].y + facemesh->joint[152].y +
                         //facemesh->joint[454].y)/4;
    float z_face_cntr = 0.0;//(facemesh->joint[234].z +
                         //facemesh->joint[10].z + facemesh->joint[152].z +
                         //facemesh->joint[454].z)/4;

    float xymag =  GetDepth(facemesh->joint[1].x*tex_w,facemesh->joint[1].y*tex_h, x_face_cntr*tex_w, y_face_cntr*tex_h);

    float zmag = facemesh->joint[1].z-z_face_cntr;

    DBG_LOGE("%5.1f, %5.1f", zmag, xymag);

    auto dev = abs(atan2(zmag, xymag))*180/3.1415926535897932384626433;

    facemesh->dev = dev;

}

static void
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


static void
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

static void
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

static void
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

static void
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

static void
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

static void
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
    draw_dbgstr_ex (strbuf, glctx.disp_w - 300, glctx.disp_h - 50, 1.0f, col_white, col_bg);

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

    init_imgui (win_w, win_h);
#endif

    imgui_data->camera_facing  = m_camera_facing;
}


void 
AppEngine::RenderFrame ()
{
    texture_2d_t srctex = glctx.tex_input;
    texture_2d_t app_logo = glctx.app_logo;
    texture_2d_t company_logo = glctx.company_logo;
    texture_2d_t drive = glctx.safe_drive;
    texture_2d_t drowsy = glctx.drowsy;
    texture_2d_t alert = glctx.alert;

    int win_w  = glctx.disp_w;
    int win_h  = glctx.disp_h;
    static double ttime[10] = {0}, interval, invoke_ms0 = 0, invoke_ms1 = 0, invoke_ms2 = 0;

    int draw_x, draw_y, draw_w, draw_h;
    int texw = srctex.width;
    int texh = srctex.height;
    float sensordata[8];
    adjust_texture (win_w, win_h, texw, texh, &draw_x, &draw_y, &draw_w, &draw_h, 0);

    glClearColor (0.f, 0.f, 0.f, 1.0f);

    /* --------------------------------------- *
     *  Render Loop
     * --------------------------------------- */

    int count = glctx.frame_count;
    {
        face_detect_result_t face_detect_ret = {0};
        face_landmark_result_t face_mesh_ret[MAX_FACE_NUM] = {0};
        irismesh_result_t iris_mesh_ret[MAX_FACE_NUM][2] = {0};
        char strbuf[512];

        PMETER_RESET_LAP ();
        PMETER_SET_LAP ();

        ttime[1] = pmeter_get_time_ms();
        interval = (count > 0) ? ttime[1] - ttime[0] : 0;
        ttime[0] = ttime[1];

        //glClear (GL_COLOR_BUFFER_BIT);
        //glViewport (0, 0, win_w, win_h);

        /* --------------------------------------- *
         *  face detection
         * --------------------------------------- */
        feed_face_detect_image(&srctex, win_w, win_h);

        ttime[2] = pmeter_get_time_ms();
        invoke_face_detect(&face_detect_ret);
        ttime[3] = pmeter_get_time_ms();
        invoke_ms0 = ttime[3] - ttime[2];

        /* --------------------------------------- *
         *  face landmark
         * --------------------------------------- */
        invoke_ms1 = 0;
        for (int face_id = 0; face_id < face_detect_ret.num; face_id++) {
            feed_face_landmark_image(&srctex, win_w, win_h, &face_detect_ret, face_id);

            ttime[4] = pmeter_get_time_ms();
            invoke_facemesh_landmark(&face_mesh_ret[face_id]);
            ttime[5] = pmeter_get_time_ms();
            invoke_ms1 += ttime[5] - ttime[4];
        }

        /* --------------------------------------- *
         *  Iris landmark
         * --------------------------------------- */
        invoke_ms2 = 0;
        for (int face_id = 0; face_id < face_detect_ret.num; face_id++) {
            for (int eye_id = 0; eye_id < 2; eye_id++) {
                feed_iris_landmark_image(&srctex, win_w, win_h, &face_detect_ret.faces[face_id],
                                         &face_mesh_ret[face_id], eye_id);

                ttime[6] = pmeter_get_time_ms();
                invoke_irismesh_landmark(&iris_mesh_ret[face_id][eye_id]);
                ttime[7] = pmeter_get_time_ms();
                invoke_ms2 += ttime[7] - ttime[6];
            }
            /* need to horizontal flip for right eye */
            flip_horizontal_iris_landmark(&iris_mesh_ret[face_id][1]);
        }


        /* --------------------------------------- *
         *  render scene (left half)
         * --------------------------------------- */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /* visualize the face pose estimation results. */
        draw_2d_texture_ex(&srctex, draw_x, draw_y, draw_w, draw_h, 0);
        render_detect_region(draw_x, draw_y, draw_w, draw_h, &face_detect_ret);

        //DBG_LOGE("X: %d, Y: %d, W: %d, H: %d",draw_x, draw_y, draw_w, draw_h);

        for (int face_id = 0; face_id < face_detect_ret.num; face_id++) {
            render_iris_landmark_on_main(draw_x, draw_y, draw_w, draw_h,
                                         &face_detect_ret.faces[face_id],
                                         &face_mesh_ret[face_id], iris_mesh_ret[face_id]);
        }

        /* --------------------------------------- *
         *  render scene  (right half)
         * --------------------------------------- */
        //glViewport (win_w, 0, win_w, win_h);

        /* draw cropped image of the face area */
        for (int face_id = 0; face_id < 1/*face_detect_ret.num*/; face_id++) {
            float w = 160;
            float h = 160;
            float x = 112;
            float y = 120 + h * face_id;
            float col_white[] = {1.0f, 1.0f, 1.0f, 1.0f};

            render_cropped_face_image(&srctex, x, y, w, h, &face_detect_ret, face_id);
            render_iris_landmark_on_face(x, y, w, h, &face_mesh_ret[face_id],
                                         iris_mesh_ret[face_id]);
            draw_2d_rect(x, y, w, h, col_white, 2.0f);
        }


        /* draw cropped image of the eye area */
        for (int face_id = 0; face_id < 1/*face_detect_ret.num*/; face_id++) {
            float w = 160;
            float h = 160;
            float x = 272;
            float y = 120 + h * face_id;
            float col_white[] = {1.0f, 1.0f, 1.0f, 1.0f};

            render_cropped_eye_image(&srctex, x, y, w, h, &face_detect_ret.faces[face_id],
                                     &face_mesh_ret[face_id], 0);
            render_iris_landmark(x, y, w, h, &iris_mesh_ret[face_id][0]);
            draw_2d_rect(x, y, w, h, col_white, 2.0f);

            x += w;
            render_cropped_eye_image(&srctex, x, y, w, h, &face_detect_ret.faces[face_id],
                                     &face_mesh_ret[face_id], 1);
            render_iris_landmark(x, y, w, h, &iris_mesh_ret[face_id][1]);
            draw_2d_rect(x, y, w, h, col_white, 2.0f);
        }

        time += interval;
        p_time = interval;

        sensor_ret(sensordata);

        if (g_x <= 1.0 || g_y <= 1.0 || g_z <= 1.0){
            g_x = sensordata[0];
            g_y = sensordata[1];
            g_z = sensordata[2];
        } else {
            g_x = 0.1*sensordata[0] + 0.9*g_x;
            g_y = 0.1*sensordata[1] + 0.9*g_y;
            g_z = 0.1*sensordata[2] + 0.9*g_z;
        }

        if (glctx.frame_count == 0){
            a_x=sensordata[0] - g_x;
            a_y=sensordata[1] - g_y;
            a_z=sensordata[2] - g_z;

        } else {
            a_x = 1.0*(sensordata[0] -g_x);
            a_y = 1.0*(sensordata[1] -g_y);
            a_z = 1.0*(sensordata[2] -g_z);

        }

        X = X + interval*v_x/1000 + 0.5*a_x*interval*interval/1000000;
        Y = Y + interval*v_y/1000 + 0.5*a_y*interval*interval/1000000;
        Z = Z + interval*v_z/1000 + 0.5*a_z*interval*interval/1000000;

        distance = std::sqrt(X*X + Y*Y + Z*Z)/1000;

        if (a_x > 0.05){
            v_x = v_x + interval*a_x/1000;
        } else if (abs(v_x) > 0.0){
            v_x = 0.8*v_x + interval*a_x/1000;
        }

        if (a_y > 0.05){
            v_y = v_y + interval*a_y/1000;
        } else if (abs(v_y) > 0.0){
            v_y = 0.8*v_y + interval*a_y/1000;
        }

        if (a_z > 0.05){
            v_z = v_z + interval*a_z/1000;
        } else if (abs(v_z) > 0.0){
            v_z = 0.8*v_z + interval*a_z/1000;
        }

        speed = 5*std::sqrt(v_x*v_x + v_y*v_y + v_z*v_z)/18;

        DBG_LOGE (
        "Acc_X:%5.1f\nAcc_Y :%5.1f\nAcc_Z :%5.1f\nA_X :%5.1f\nA_Y :%5.1f\nA_Z :%5.1f",
        sensordata[0], sensordata[1], sensordata[2], a_x, a_y, a_z);

        int fps = (int) round(1000 / interval);

        for (int eye_id = 0; eye_id < 2; eye_id++) {
            CalculateEAR(&iris_mesh_ret[0][eye_id], draw_w, draw_h);
        }

        float EAR = (iris_mesh_ret[0][0].EAR + iris_mesh_ret[0][1].EAR) / 2;

        CalculateMAR(&face_mesh_ret[0], draw_w, draw_h);

        CalculateDev(&face_mesh_ret[0], draw_w, draw_h);

        if (isnan(EAR) != 1) {
            avg_EAR = (avg_EAR * glctx.frame_count + EAR) / (glctx.frame_count + 1);
        }

        if (isnan(face_mesh_ret[0].MAR) != 1) {
            avg_MAR =
                    (avg_MAR * glctx.frame_count + face_mesh_ret[0].MAR) / (glctx.frame_count + 1);
        }

        if (isnan(face_mesh_ret[0].dev) != 1){
        avg_Dev = (avg_Dev * glctx.frame_count + face_mesh_ret[0].dev) / (glctx.frame_count + 1);
        }

        if (abs(EAR -avg_EAR)/avg_EAR >= 0.3 && glctx.frame_count>=60*fps){
            blink_streak += 1;
            blinks +=1;
        } else{
            blink_streak=0;
        }

        if ((face_mesh_ret[0].MAR - avg_MAR)/avg_MAR >= 0.5 && glctx.frame_count>=60*fps){
            yawn_streak += 1;
            yawns+=1;
        } else{
            yawn_streak=0;
        }

        if (abs((face_mesh_ret[0].dev - avg_Dev)/avg_Dev) >= 0.4 && glctx.frame_count>=60*fps){
            distracted_streak += 1;
            distracted += 1;

        } else{
            distracted_streak=0;
        }

        if(glctx.frame_count>=60*fps) {
            if (distracted_streak > fps || yawn_streak > fps || blink_streak > fps) {
                if (sound_started != 1) {
                    soundGenerator.startAudio();
                    sound_started = 1;
                }
            }
        }

        if (sound_started==1){
            sound_streak += 1;
        }

        if(sound_started==1 && sound_streak >= 3*fps && distracted_streak == 0 && yawn_streak==0 && blink_streak==0){
            soundGenerator.stopAudio();
            sound_started = 0;
            sound_streak = 0;
        }

        if (sound_streak >= 5*fps){
            soundGenerator.stopAudio();
            sound_started = 0;
            sound_streak = 0;
        }


        DBG_LOGE ("Sound: %d \nSleep : %d \nDrowsy: %d \nDistracted : %d",
                  sound_streak, blink_streak, yawn_streak, distracted_streak);


        /* --------------------------------------- *
         *  post process
         * --------------------------------------- */
        if (fps >= 10 || glctx.frame_count % 2 == 0) {
            DrawTFLiteConfigInfo();

            draw_pmeter(0, 40);

            float camx = 200;
            float camy = 350;
            float camw = 300;
            float camh = 300;

            draw_2d_texture_ex(&app_logo, camx, camy, camw, camh, 0);

            int dx = camx + 25;
            int dy = camy + camh + 70;
            int dw = camw - 75;
            int dh = camh - 75;


            draw_2d_texture_ex(&company_logo, dx, dy + 25, dw, dh, 0);

            if (sound_started == 1 && blink_streak < yawn_streak){

                draw_2d_texture_ex(&drowsy, glctx.disp_w + 75 - camx - camw, camy- 200, camw, camh, 0);

            } else if (sound_started == 1 && blink_streak > yawn_streak || distracted_streak > blink_streak){

                draw_2d_texture_ex(&alert, glctx.disp_w + 75 - camx - camw, camy- 200, camw, camh, 0);

            } else {

                draw_2d_texture_ex(&drive, glctx.disp_w + 75 - camx - camw, camy- 200, camw, camh, 0);

            }



            if(glctx.frame_count <= 60*fps) {
                sprintf(strbuf,
                        "CALIBRATING! \nFPS: %d\nEAR :%5.1f , Avg. EAR:%5.1f\nMAR :%5.1f, Avg. MAR:%5.1f\nPose:%5.1f , Avg. Pose:%5.1f",
                        fps, EAR, avg_EAR, face_mesh_ret[0].MAR, avg_MAR, face_mesh_ret[0].dev, avg_Dev);


            } else{
                sprintf(strbuf,
                        "FPS: %d\nEAR :%5.2f , Avg. EAR:%5.2f\nMAR :%5.2f, Avg. MAR:%5.2f\nPose:%5.2f , Avg. Pose:%5.2f",
                        fps, EAR, avg_EAR, face_mesh_ret[0].MAR, avg_MAR, face_mesh_ret[0].dev, avg_Dev);
            }

            //DBG_LOGE (
                    //"Interval:%5.1f [ms]\nTFLite0 :%5.1f [ms]\nTFLite1 :%5.1f [ms]\nTFLite2 :%5.1f [ms]",
                    //interval, invoke_ms0, invoke_ms1, invoke_ms2);
            draw_dbgstr(strbuf, 20, 10);

            /* renderer info */
            int y = win_h - 22 * 5;
            //draw_dbgstr(glctx.str_glverstion, 10, y);
            //y += 22;
            draw_dbgstr(glctx.str_glvendor, 125, y);
            y += 22;
            draw_dbgstr(glctx.str_glrender, 125, y);
            y += 22;

            char state[512];

            sprintf(state, "Speed: %3.2f kmph\nDistance: %2.2f km\nAcc_x: %8.2f\nAcc_y: %8.2f\nAcc_z: %8.2f", speed, distance, a_x, a_y, a_z);

            float col_pink[]  = {1.0f, 0.0f, 0.5f, 0.0f};
            float col_white[] = {1.0f, 1.0f, 1.0f, 1.0f};
            float *col_bg = col_pink;

            draw_dbgstr_ex(state, glctx.disp_w - 550, glctx.disp_h - 450, 2.5f, col_white, col_bg);
        }
#if defined (USE_IMGUI)
        invoke_imgui (&imgui_data);
#endif
        egl_swap();

    }
    glctx.frame_count ++;
}


AppEngine::AppEngine (android_app* app)
    : m_app(app),
      m_cameraGranted(false),
      m_camera(nullptr),
      m_camera_facing(1),
      blinks(0),
      sleep(0),
      drowsy(0),
      yawns(0),
      distracted(0),
      yawn_streak(0),
      blink_streak(0),
      avg_MAR(0.0),
      avg_Dev(0.0),
      avg_EAR(0.0),
      sound_started(0),
      sound_streak(0),
      speed(0.0),
      distance(0.0),
      a_x(0.0),
      a_y(0.0),
      a_z(0.0),
      alarms(0),
      p_time(0.0),
      time(0.0),
      g_x(0.0),
      g_y(0.0),
      g_z(0.0),
      v_x(0.0),
      v_y(0.0),
      v_z(0.0),
      X(0.0),
      Y(0.0),
      Z(0.0)

{
    memset (&glctx, 0, sizeof (glctx));
}

AppEngine::~AppEngine()
{
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

    setup_imgui (w, h, &imgui_data);

    glctx.disp_w = w;
    glctx.disp_h = h;
    LoadInputTexture (&glctx.tex_static, (char *)"pakutaso_sotsugyou.jpg");

    LoadInputTexture (&glctx.app_logo, (char *)"mati.jpg");

    LoadInputTexture (&glctx.company_logo, (char *)"Torg_Tech_logo.jpg");

    LoadInputTexture (&glctx.drowsy, (char *)"drowsy.jpg");

    LoadInputTexture (&glctx.safe_drive, (char *)"safe_Drive.jpg");

    LoadInputTexture (&glctx.alert, (char *)"alert.jpg");

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

    if (m_cameraGranted && glctx.tex_camera_valid == false)
        return;

    CropCameraTexture ();

    RenderFrame();
}

void
AppEngine::CropCameraTexture (void)
{
    texture_2d_t srctex = glctx.tex_camera;
    if (!glctx.tex_camera_valid)
        srctex = glctx.tex_static;

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

    m_ImgReader.ReleaseImageReader ();
    glctx.tex_camera_valid = false;
}


void 
AppEngine::CreateCamera (int facing)
{
    m_camera = new NDKCamera();
    ASSERT (m_camera, "Failed to Create CameraObject");

    m_camera->SelectCameraFacing (facing);

    m_ImgReader.InitImageReader (CAMERA_RESOLUTION_W, CAMERA_RESOLUTION_H);
    ANativeWindow *nativeWindow = m_ImgReader.GetNativeWindow();

    m_camera->CreateSession (nativeWindow);
    m_camera->StartPreview (true);
}

void
AppEngine::UpdateCameraTexture ()
{
    /* Acquire the latest AHardwareBuffer */
    AHardwareBuffer *ahw_buf = NULL;
    int ret = m_ImgReader.GetCurrentHWBuffer (&ahw_buf);
    if (ret != 0)
        return;

    /* Get EGLClientBuffer */
    EGLClientBuffer egl_buf = eglGetNativeClientBufferANDROID (ahw_buf);
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

    EGLint attrs[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE,};
    glctx.egl_img = eglCreateImageKHR (egl_get_display(), EGL_NO_CONTEXT,
                                       EGL_NATIVE_BUFFER_ANDROID, egl_buf, attrs);

    /* Bind to GL_TEXTURE_EXTERNAL_OES */
    texture_2d_t *input_tex = &glctx.tex_camera;
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
    glBindTexture (GL_TEXTURE_EXTERNAL_OES, input_tex->texid);

    glEGLImageTargetTexture2DOES (GL_TEXTURE_EXTERNAL_OES, glctx.egl_img);
    GLASSERT ();

    glctx.tex_camera_valid = true;
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

void
AppEngine::StatusString (char* strbuf)
{
    //char strbuf[512];

    sprintf (strbuf, "Frames:%d, Sleepy: %d, Drowsy: %d, Distracted: %d, Alarms: %d, Interval: %.2f, "
                     "Avg. EAR: %.2f, Avg. MAR: %.2f, Avg. Pose: %.2f, Speed: %0.2f, Distance: %.2f, "
                     "Acc_X: %0.2f, Acc_Y: %0.2f, Acc_Z: %0.2f\n",
             glctx.frame_count, blinks, yawns, distracted, alarms, p_time, avg_EAR, avg_MAR, avg_Dev, speed, distance, a_x, a_y, a_z);

    //DBG_LOGE("%s", strbuf);

    return;
}

extern "C" JNIEXPORT void JNICALL
Java_com_glesapp_glesapp_GLESAppNativeActivity_notifyCameraPermission (
                            JNIEnv *env, jclass type, jboolean permission)
{
    std::thread permissionHandler (&AppEngine::OnCameraPermission, GetAppEngine(), permission);
    permissionHandler.detach();
}
//
// Created by tarzan on 2/8/21.
//

#ifndef TARZAN_ADAS_UTILS_H
#define TARZAN_ADAS_UTILS_H

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
#include "tflite_dense_depth.h"
#include "tflite_detect.h"
#include "tflite_deeplab.h"
#include "tflite_laneseg.h"

#endif //TARZAN_ADAS_UTILS_H

void
feed_detect_image_uint8 (texture_2d_t *srctex, int win_w, int win_h);

void
feed_detect_image_float (texture_2d_t *srctex, int win_w, int win_h);

void
feed_detect_image(texture_2d_t *srctex, int win_w, int win_h);

void
feed_deeplab_image(texture_2d_t *srctex, int win_w, int win_h);

void
feed_laneseg_image(texture_2d_t *srctex, int win_w, int win_h);

void
feed_dense_depth_image(texture_2d_t *srctex, int win_w, int win_h);

void
feed_face_detect_image(texture_2d_t *srctex, int win_w, int win_h);

void
feed_face_landmark_image(texture_2d_t *srctex, int win_w, int win_h, face_detect_result_t *detection, unsigned int face_id);

void
feed_iris_landmark_image(texture_2d_t *srctex, int win_w, int win_h,
                         face_t *face, face_landmark_result_t *facemesh, int eye_id);

void
render_deeplab_result (int ofstx, int ofsty, int draw_w, int draw_h,
                       deeplab_result_t *deeplab_ret);

void
render_deeplab_heatmap ( int ofstx, int ofsty, int draw_w, int draw_h, deeplab_result_t *deeplab_ret);

void
render_detect_region (int ofstx, int ofsty, int texw, int texh,
                      face_detect_result_t *detection);

void
render_cropped_face_image (texture_2d_t *srctex, int ofstx, int ofsty, int texw, int texh,
                           face_detect_result_t *detection, unsigned int face_id);

void
render_cropped_eye_image (texture_2d_t *srctex, int ofstx, int ofsty, int texw, int texh,
                          face_t *face, face_landmark_result_t *facemesh, int eye_id);

void
render_lines (int ofstx, int ofsty, int texw, int texh, float *mat, irismesh_result_t *irismesh, int *idx, int num);

void
render_iris_landmark (int ofstx, int ofsty, int texw, int texh, irismesh_result_t *irismesh);

void
render_iris_landmark_on_face (int ofstx, int ofsty, int texw, int texh,
                              face_landmark_result_t *facemesh, irismesh_result_t *irismesh);

void
render_facemesh_keypoint (int ofstx, int ofsty, int texw, int texh, float *mat, fvec3 *joint, int idx);

void
flip_horizontal_iris_landmark (irismesh_result_t *irismesh);

void
render_iris_landmark_on_main (int ofstx, int ofsty, int texw, int texh,
                              face_t *face, face_landmark_result_t *facemesh, irismesh_result_t *irismesh);

void
render_depth_image_3d (texture_2d_t *srctex, imgui_data_t s_gui_prop, int ofstx, int ofsty, int texw, int texh,
                       dense_depth_result_t *dense_depth_ret);

void
render_obj_detect_region (int ofstx, int ofsty, int texw, int texh,
                          detect_result_t *detection);
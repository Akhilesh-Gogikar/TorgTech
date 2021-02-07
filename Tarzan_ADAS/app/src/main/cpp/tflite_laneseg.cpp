/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#include "util_tflite.h"
#include "tflite_laneseg.h"
#include "util_debug.h"

static tflite_interpreter_t s_laneseg_interpreter;
static tflite_tensor_t      s_tensor_laneseg_input;
static tflite_tensor_t      s_tensor_bin_laneseg;
static tflite_tensor_t      s_tensor_instance_laneseg;

static float s_class_color[MAX_LANE_CLASS + 1][4];
static char  s_class_name [MAX_LANE_CLASS + 1][64] =
{
    "background",   // 0
    "leftmost",    // 1
    "midleft",      // 2
    "midright",         // 3
    "right",         // 4
};

static int
init_class_color ()
{
    for (int i = 0; i < MAX_LANE_CLASS + 1; i ++)
    {
        float *col = s_class_color[i];
        if (i == 0)
        {
            col[0] = col[1] = col[2] = col[3] = 0.0f;
        }
        else
        {
            col[0] = (rand () % 255) / 255.0f;
            col[1] = (rand () % 255) / 255.0f;
            col[2] = (rand () % 255) / 255.0f;
            col[3] = 0.5f;
        }
    }
    return 0;
}


int
init_tflite_laneseg(const char *model_buf, size_t model_size)
{
    tflite_create_interpreter (&s_laneseg_interpreter, model_buf, model_size);

    /* get input tensor */
    tflite_get_tensor_by_name (&s_laneseg_interpreter, 0, "lanenet/input_tensor",  &s_tensor_laneseg_input);

    /* get output tensor */
    tflite_get_tensor_by_name (&s_laneseg_interpreter, 1, "lanenet/final_binary_output",  &s_tensor_bin_laneseg);

    tflite_get_tensor_by_name (&s_laneseg_interpreter, 1, "lanenet/final_pixel_embedding_output",  &s_tensor_instance_laneseg);

    init_class_color ();

    return 0;
}

int
get_laneseg_input_type ()
{
    if (s_tensor_laneseg_input.type == kTfLiteUInt8)
        return 1;
    else
        return 0;
}

void *
get_laneseg_input_buf (int *w, int *h)
{
    *w = s_tensor_laneseg_input.dims[2];
    *h = s_tensor_laneseg_input.dims[1];
    return s_tensor_laneseg_input.ptr;
}

char *
get_laneseg_class_name (int class_idx)
{
    return s_class_name[class_idx];
}

float *
get_laneseg_class_color (int class_idx)
{
    return s_class_color[class_idx];
}


int
invoke_laneseg (laneseg_result_t *laneseg_result)
{
    if (s_laneseg_interpreter.interpreter->Invoke() != kTfLiteOk)
    {
        DBG_LOGE ("ERR: %s(%d)\n", __FILE__, __LINE__);
        return -1;
    }

    laneseg_result->lanemap         = (float *)s_tensor_instance_laneseg.ptr;
    laneseg_result->bin_lanemap         = (float *)s_tensor_bin_laneseg.ptr;
    laneseg_result->segmentmap_dims[0] = s_tensor_instance_laneseg.dims[1];
    laneseg_result->segmentmap_dims[1] = s_tensor_instance_laneseg.dims[0];
    laneseg_result->segmentmap_dims[2] = s_tensor_instance_laneseg.dims[2];

    return 0;
}


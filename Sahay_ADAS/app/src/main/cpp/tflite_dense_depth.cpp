/* ------------------------------------------------ *
 * Copyright (c) 2020 gogikar.akhilesh@gmail.com
 * ------------------------------------------------ */
#include "util_tflite.h"
#include "tflite_dense_depth.h"
#include <list>


static tflite_interpreter_t s_interpreter;
static tflite_tensor_t      s_tensor_input;
static tflite_tensor_t      s_tensor_depth;


/* -------------------------------------------------- *
 *  Create TFLite Interpreter
 * -------------------------------------------------- */
/**
 * Initializes the TFLite interpreter.
 *
 * @param model_buf The model buffer.
 * @param model_size The size of the model buffer.
 *
 * @returns 0 on success, -1 on failure.
 */
int
init_tflite_dense_depth (const char *model_buf, size_t model_size)
{
    tflite_create_interpreter (&s_interpreter, model_buf, model_size);
    tflite_get_tensor_by_name (&s_interpreter, 0, "im0",  &s_tensor_input);
    tflite_get_tensor_by_name (&s_interpreter, 1, "truediv", &s_tensor_depth);

    return 0;
}


/**
 * Returns a pointer to the input buffer for the specified input tensor.
 *
 * @param input_name The name of the input tensor.
 * @param w A pointer to the width of the input tensor.
 * @param h A pointer to the height of the input tensor.
 *
 * @returns A pointer to the input buffer.
 */
void *
get_dense_depth_input_buf (int *w, int *h)
{
    /* need to retrieve the input tensor again ? (dynamic shape) */
    tflite_get_tensor_by_name (&s_interpreter, 0, "im0", &s_tensor_input);

    *w = s_tensor_input.dims[2];
    *h = s_tensor_input.dims[1];
    return s_tensor_input.ptr;
}


/* -------------------------------------------------- *
 * Invoke TensorFlow Lite
 * -------------------------------------------------- */
/**
 * Invokes the TensorFlow Lite model with the given input data.
 *
 * @param input_data The input data to the model.
 * @param input_data_size The size of input_data.
 * @param output_data The output data from the model.
 * @param output_data_size The size of output_data.
 *
 * @returns 0 if the model runs without error.
 */
int
invoke_dense_depth (dense_depth_result_t *dense_depth_result)
{
    if (s_interpreter.interpreter->Invoke() != kTfLiteOk)
    {
        fprintf (stderr, "ERR: %s(%d)\n", __FILE__, __LINE__);
        return -1;
    }

    /* need to retrieve the output tensor again ? (dynamic shape) */
    tflite_get_tensor_by_name (&s_interpreter, 1, "truediv", &s_tensor_depth);
    
    dense_depth_result->depthmap         = (float *)s_tensor_depth.ptr;
    dense_depth_result->depthmap_dims[0] = s_tensor_depth.dims[2];
    dense_depth_result->depthmap_dims[1] = s_tensor_depth.dims[1];

    return 0;
}


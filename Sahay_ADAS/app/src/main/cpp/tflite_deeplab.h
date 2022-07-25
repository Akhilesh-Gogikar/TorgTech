/* ------------------------------------------------ *
 * Copyright (c) 2020 gogikar.akhilesh@gmail.com
 * ------------------------------------------------ */
#ifndef TFLITE_DEEPLAB_H_
#define TFLITE_DEEPLAB_H_

#ifdef __cplusplus
extern "C" {
#endif

/* https://storage.googleapis.com/download.tensorflow.org/models/tflite/gpu/deeplabv3_257_mv_gpu.tflite */
#define DEEPLAB_MODEL_PATH  "deeplab_model/deeplabv3_257_mv_gpu.tflite"

#define MAX_DETECT_CLASS 20



/**
 * Runs the Deeplab model on an input image.
 *
 * @param input_image The input image to run the model on.
 * @param input_image_dims The dimensions of the input image.
 * @param input_image_type The type of the input image.
 * @param input_image_format The format of the input image.
 * @param input_image_layout The layout of the input image.
 * @param input_image_scale The scale of the input image.
 * @param input_image_zero_point The zero point of the input image.
 * @param input_image_dtype The data type of the input image.
 */
typedef struct _deeplab_result_t
{
    float *segmentmap;
    int   segmentmap_dims[3];
} deeplab_result_t;



int   init_tflite_deeplab (const char *model_buf, size_t model_size);
int   get_deeplab_input_type ();
void  *get_deeplab_input_buf (int *w, int *h);
char  *get_deeplab_class_name (int class_idx);
float *get_deeplab_class_color (int class_idx);

int   invoke_deeplab (deeplab_result_t *deeplab_result);

#ifdef __cplusplus
}
#endif

#endif /* TFLITE_DEEPLAB_H_ */

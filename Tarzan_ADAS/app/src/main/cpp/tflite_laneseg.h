/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2020 terryky1220@gmail.com
 * ------------------------------------------------ */
#ifndef TFLITE_LANESEG_H_
#define TFLITE_LANESEG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* https://storage.googleapis.com/download.tensorflow.org/models/tflite/gpu/deeplabv3_257_mv_gpu.tflite */
#define LANESEG_MODEL_PATH  "laneseg_model/lane_segmentation_model.tflite"

#define MAX_LANE_CLASS 4


typedef struct _laneseg_result_t
{
    float *lanemap;
    float *bin_lanemap;
    int   segmentmap_dims[3];
} laneseg_result_t;



int   init_tflite_laneseg (const char *model_buf, size_t model_size);
int   get_laneseg_input_type ();
void  *get_laneseg_input_buf (int *w, int *h);
char  *get_laneseg_class_name (int class_idx);
float *get_laneseg_class_color (int class_idx);

int   invoke_laneseg (laneseg_result_t *laneseg_result);

#ifdef __cplusplus
}
#endif

#endif /* TFLITE_LANESEG_H_ */

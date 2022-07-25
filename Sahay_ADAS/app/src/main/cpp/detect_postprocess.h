/* ------------------------------------------------ *
 * Copyright (c) 2020 gogikar.akhilesh@gmail.com
 * ------------------------------------------------ */
#ifndef _DETECT_POSTPROCESS_H_
#define _DETECT_POSTPROCESS_H_


/**
 * Computes the intersection-over-union overlap between two boxes.
 *
 * @param box1 The first box.
 * @param box2 The second box.
 *
 * @returns The intersection-over-union overlap between the two boxes.
 */
struct DetectionBox {
    float x1;
    float y1;
    float x2;
    float y2;
    float score;
    int   class_id;
};

/**
 * Initializes the postprocessing module.
 *
 * @param filename The path to the YOLO weights file.
 *
 * @returns 0 on success, -1 on failure.
 */
int init_detect_postprocess (std::string filename);

/**
 * Postprocesses the output of the detection model.
 *
 * @param detection_boxes The detection boxes.
 * @param boxes_ptr The pointer to the boxes.
 * @param scores_ptr The pointer to the scores.
 *
 * @returns The number of detections.
 */
int
invoke_detection_postprocess (std::vector<DetectionBox> &detection_boxes,  /* [OUT] */
                              const float *boxes_ptr,                      /* [IN ] */
                              const float *_scores_ptr);                   /* [IN ] */

#endif /* _DETECT_POSTPROCESS_H_ */

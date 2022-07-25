/* ------------------------------------------------ *
 * Copyright (c) 2020 gogikar.akhilesh@gmail.com
 * ------------------------------------------------ */
#ifndef _RENDER_HANDPOSE_H_
#define _RENDER_HANDPOSE_H_

/**
 * Creates a mesh object.
 *
 * @param num_tile_w The number of tiles in the width direction.
 * @param num_tile_h The number of tiles in the height direction.
 * @param num_idx The number of indices in the index array.
 *
 * @returns The mesh object.
 */
typedef struct _mesh_obj_t
{
    float           *vtx_array;
    float           *uv_array;
    unsigned short  *idx_array;

    GLuint vbo_vtx;
    GLuint vbo_uv;
    GLuint vbo_idx;

    int num_tile_w;
    int num_tile_h;
    int num_idx;
} mesh_obj_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the cube.
 *
 * @param aspect The aspect ratio of the cube.
 *
 * @returns The number of vertices in the cube.
 */
int init_cube (float aspect);

/**
 * Draws a cube in the global matrix.
 *
 * @param mtxGlobal The global matrix.
 * @param color The color of the cube.
 *
 * @returns The number of vertices in the cube.
 */
int draw_cube (float *mtxGlobal, float *color);

/**
 * Draws a random integer from the floor of a uniform distribution.
 *
 * @param mtxGlobal A pointer to the global random number matrix.
 * @param div_u The upper bound of the uniform distribution.
 * @param div_v The lower bound of the uniform distribution.
 *
 * @returns The random integer drawn from the floor of the uniform distribution.
 */
int draw_floor (float *mtxGlobal, float div_u, float div_v);

/**
 * Draws a line between two points.
 *
 * @param mtxGlobal The global matrix.
 * @param p0 The first point.
 * @param p1 The second point.
 * @param color The color of the line.
 *
 * @returns None
 */
int draw_line (float *mtxGlobal, float *p0, float *p1, float *color);

/**
 * Draws a triangle on the screen.
 *
 * @param mtxGlobal A pointer to the global matrix.
 * @param p0 A pointer to the first point of the triangle.
 * @param p1 A pointer to the second point of the triangle.
 * @param p2 A pointer to the third point of the triangle.
 * @param color A pointer to the color of the triangle.
 *
 * @returns None
 */
int draw_triangle (float *mtxGlobal, float *p0, float *p1, float *p2, float *color);

/**
 * Draws a point array.
 *
 * @param mtxGlobal The global matrix.
 * @param vtx The vertex array.
 * @param uv The uv array.
 * @param num The number of points.
 * @param texid The texture id.
 * @param color The color array.
 *
 * @returns None
 */
int draw_point_arrays (float *mtxGlobal, float *vtx, float *uv, int num, int texid, float *color);

/**
 * Creates a mesh object.
 *
 * @param mobj The mesh object to create.
 * @param num_tile_w The number of tiles in the width direction.
 * @param num_tile_h The number of tiles in the height direction.
 *
 * @returns 0 on success, -1 on failure.
 */
int create_mesh (mesh_obj_t *mobj, int num_tile_w, int num_tile_h);

#ifdef __cplusplus
}
#endif
#endif /* _RENDER_HANDPOSE_H_ */
 
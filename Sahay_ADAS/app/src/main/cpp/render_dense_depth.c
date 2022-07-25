/* ------------------------------------------------ *
 * Copyright (c) 2020 gogikar.akhilesh@gmail.com
 * ------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <GLES2/gl2.h>
#include "util_egl.h"
#include "assertgl.h"
#include "util_shader.h"
#include "util_matrix.h"
#include "util_debugstr.h"
#include "util_pmeter.h"
#include "util_debug.h"
#include "util_texture.h"
#include "render_dense_depth.h"

#define UNUSED(x) (void)(x)

static int          s_texid_dummy = 0;

static shader_obj_t s_sobj;
static float        s_matPrj[16];
static GLint        s_loc_mtx_mv;
static GLint        s_loc_mtx_pmv;
static GLint        s_loc_mtx_nrm;
static GLint        s_loc_color;
static GLint        s_loc_alpha;
static GLint        s_loc_lightpos;


/**
 * Computes the normals for the cube.
 *
 * @returns None
 */
static GLfloat s_nrm[] =
{
     0.0f,  0.0f,  1.0f,
     0.0f,  0.0f, -1.0f,
     1.0f,  0.0f,  0.0f,
    -1.0f,  0.0f,  0.0f,
     0.0f,  1.0f,  0.0f,
     0.0f, -1.0f,  0.0f,
};


/**
 * A simple function that returns the UV coordinates for a texture.
 *
 * @returns A pointer to the UV coordinates.
 */
static GLfloat s_uv [] =
{
#if 0
     0.0f, 1.0f,
     0.0f, 0.0f,
     1.0f, 1.0f,
     1.0f, 0.0f,
#else
     0.0f, 0.0f,
     0.0f, 1.0f,
     1.0f, 0.0f,
     1.0f, 1.0f,
#endif
};


/**
 * Computes the diffuse and specular lighting for a point.
 *
 * @param normal The normal of the point.
 * @param eyePos The position of the eye.
 *
 * @returns None
 */
static char s_strVS[] = "                                   \n\
                                                            \n\
attribute vec4  a_Vertex;                                   \n\
attribute vec3  a_Normal;                                   \n\
attribute vec2  a_TexCoord;                                 \n\
uniform   mat4  u_PMVMatrix;                                \n\
uniform   mat4  u_MVMatrix;                                 \n\
uniform   mat3  u_ModelViewIT;                              \n\
varying   vec3  v_diffuse;                                  \n\
varying   vec3  v_specular;                                 \n\
varying   vec2  v_texcoord;                                 \n\
const     float shiness = 16.0;                             \n\
uniform   vec3  u_LightPos;                                 \n\
const     vec3  LightCol = vec3(1.0, 1.0, 1.0);             \n\
                                                            \n\
void DirectionalLight (vec3 normal, vec3 eyePos)            \n\
{                                                           \n\
    vec3  lightDir = normalize (u_LightPos);                \n\
    vec3  halfV    = normalize (u_LightPos - eyePos);       \n\
    float dVP      = max(dot(normal, lightDir), 0.0);       \n\
    float dHV      = max(dot(normal, halfV   ), 0.0);       \n\
                                                            \n\
    float pf = 0.0;                                         \n\
    if(dVP > 0.0)                                           \n\
        pf = pow(dHV, shiness);                             \n\
                                                            \n\
    v_diffuse += dVP * LightCol;                            \n\
    v_specular+= pf  * LightCol * 0.5;                      \n\
}                                                           \n\
                                                            \n\
void main(void)                                             \n\
{                                                           \n\
    gl_Position = u_PMVMatrix * a_Vertex;                   \n\
    vec3 normal = normalize(u_ModelViewIT * a_Normal);      \n\
    vec3 eyePos = vec3(u_MVMatrix * a_Vertex);              \n\
                                                            \n\
    v_diffuse  = vec3(0.5);                                 \n\
    v_specular = vec3(0.0);                                 \n\
    DirectionalLight(normal, eyePos);                       \n\
                                                            \n\
    v_diffuse = clamp(v_diffuse, 0.0, 1.0);                 \n\
    v_texcoord  = a_TexCoord;                               \n\
}                                                           ";

/**
 * A simple fragment shader that renders a single pixel.
 *
 * @param u_color The color of the pixel.
 * @param u_alpha The alpha value of the pixel.
 * @param v_diffuse The diffuse color of the pixel.
 * @param v_specular The specular color of the pixel.
 * @param v_texcoord The texture coordinate of the pixel.
 *
 * @returns The color of the pixel.
 */
static char s_strFS[] = "                                   \n\
precision mediump float;                                    \n\
                                                            \n\
uniform vec3    u_color;                                    \n\
uniform float   u_alpha;                                    \n\
varying vec3    v_diffuse;                                  \n\
varying vec3    v_specular;                                 \n\
varying vec2    v_texcoord;                                 \n\
uniform sampler2D u_sampler;                                \n\
                                                            \n\
void main(void)                                             \n\
{                                                           \n\
    vec3 color;                                             \n\
    color = vec3(texture2D(u_sampler, v_texcoord));         \n\
    color *= (u_color * v_diffuse);                         \n\
    //color += v_specular;                                  \n\
    gl_FragColor = vec4(color, u_alpha);                    \n\
}                                                           ";


/**
 * Computes the inverse of a 3x3 matrix.
 *
 * @param matMVI3x3 The inverse of the matrix.
 * @param matMV The matrix to compute the inverse of.
 *
 * @returns None
 */
static void
compute_invmat3x3 (float *matMVI3x3, float *matMV)
{
    float matMVI4x4[16];

    matrix_copy (matMVI4x4, matMV);
    matrix_invert   (matMVI4x4);
    matrix_transpose(matMVI4x4);
    matMVI3x3[0] = matMVI4x4[0];
    matMVI3x3[1] = matMVI4x4[1];
    matMVI3x3[2] = matMVI4x4[2];
    matMVI3x3[3] = matMVI4x4[4];
    matMVI3x3[4] = matMVI4x4[5];
    matMVI3x3[5] = matMVI4x4[6];
    matMVI3x3[6] = matMVI4x4[8];
    matMVI3x3[7] = matMVI4x4[9];
    matMVI3x3[8] = matMVI4x4[10];
}


/**
 * Initializes the OpenGL state for the cube.
 *
 * @param aspect The aspect ratio of the viewport.
 *
 * @returns None
 */
int
init_cube (float aspect)
{
    generate_shader (&s_sobj, s_strVS, s_strFS);
    s_loc_mtx_mv  = glGetUniformLocation(s_sobj.program, "u_MVMatrix" );
    s_loc_mtx_pmv = glGetUniformLocation(s_sobj.program, "u_PMVMatrix" );
    s_loc_mtx_nrm = glGetUniformLocation(s_sobj.program, "u_ModelViewIT" );
    s_loc_color   = glGetUniformLocation(s_sobj.program, "u_color" );
    s_loc_alpha   = glGetUniformLocation(s_sobj.program, "u_alpha" );
    s_loc_lightpos= glGetUniformLocation(s_sobj.program, "u_LightPos" );

    matrix_proj_perspective (s_matPrj, 72.0f, aspect, 1.f, 10000.f);

    unsigned char imgbuf[] = {255, 255, 255, 255};
    s_texid_dummy = create_2d_texture (imgbuf, 1, 1);

    GLASSERT ();
    return 0;
}



/**
 * Draws a triangle.
 *
 * @param mtxGlobal The global matrix.
 * @param p0 The first point of the triangle.
 * @param p1 The second point of the triangle.
 * @param p2 The third point of the triangle.
 * @param color The color of the triangle.
 *
 * @returns None
 */
int
draw_triangle (float *mtxGlobal, float *p0, float *p1, float *p2, float *color)
{
    float matMV[16], matPMV[16], matMVI3x3[9];
    GLfloat floor_vtx [9];

    for (int i = 0; i < 3; i ++)
    {
        floor_vtx[0 + i] = p0[i];
        floor_vtx[3 + i] = p1[i];
        floor_vtx[6 + i] = p2[i];
    }

    glEnable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);

    glUseProgram( s_sobj.program );

    glEnableVertexAttribArray (s_sobj.loc_vtx);
    glEnableVertexAttribArray (s_sobj.loc_uv );
    glDisableVertexAttribArray(s_sobj.loc_nrm);
    glVertexAttribPointer (s_sobj.loc_vtx, 3, GL_FLOAT, GL_FALSE, 0, floor_vtx);
    glVertexAttribPointer (s_sobj.loc_uv , 2, GL_FLOAT, GL_FALSE, 0, s_uv );
    glVertexAttrib4fv (s_sobj.loc_nrm, s_nrm);

    matrix_identity (matMV);
    compute_invmat3x3 (matMVI3x3, matMV);

    matrix_mult (matMV, mtxGlobal, matMV);
    matrix_mult (matPMV, s_matPrj, matMV);

    glUniformMatrix4fv (s_loc_mtx_mv,   1, GL_FALSE, matMV );
    glUniformMatrix4fv (s_loc_mtx_pmv,  1, GL_FALSE, matPMV);
    glUniformMatrix3fv (s_loc_mtx_nrm,  1, GL_FALSE, matMVI3x3);
    glUniform3f (s_loc_lightpos, 1.0f, 1.0f, 1.0f);
    glUniform3f (s_loc_color, color[0], color[1], color[2]);
    glUniform1f (s_loc_alpha, color[3]);

    glEnable (GL_BLEND);

    glBindTexture (GL_TEXTURE_2D, s_texid_dummy);
    glDrawArrays (GL_TRIANGLES, 0, 3);

    glDisable (GL_BLEND);

    return 0;
}



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
int
draw_line (float *mtxGlobal, float *p0, float *p1, float *color)
{
    float matMV[16], matPMV[16], matMVI3x3[9];
    GLfloat floor_vtx [9];

    for (int i = 0; i < 3; i ++)
    {
        floor_vtx[0 + i] = p0[i];
        floor_vtx[3 + i] = p1[i];
    }

    glEnable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);

    glUseProgram( s_sobj.program );

    glEnableVertexAttribArray (s_sobj.loc_vtx);
    glEnableVertexAttribArray (s_sobj.loc_uv );
    glDisableVertexAttribArray(s_sobj.loc_nrm);
    glVertexAttribPointer (s_sobj.loc_vtx, 3, GL_FLOAT, GL_FALSE, 0, floor_vtx);
    glVertexAttribPointer (s_sobj.loc_uv , 2, GL_FLOAT, GL_FALSE, 0, s_uv );
    glVertexAttrib4fv (s_sobj.loc_nrm, s_nrm);

    matrix_identity (matMV);
    compute_invmat3x3 (matMVI3x3, matMV);

    matrix_mult (matMV, mtxGlobal, matMV);
    matrix_mult (matPMV, s_matPrj, matMV);

    glUniformMatrix4fv (s_loc_mtx_mv,   1, GL_FALSE, matMV );
    glUniformMatrix4fv (s_loc_mtx_pmv,  1, GL_FALSE, matPMV);
    glUniformMatrix3fv (s_loc_mtx_nrm,  1, GL_FALSE, matMVI3x3);
    glUniform3f (s_loc_lightpos, 1.0f, 1.0f, 1.0f);
    glUniform3f (s_loc_color, color[0], color[1], color[2]);
    glUniform1f (s_loc_alpha, color[3]);

    glEnable (GL_BLEND);

    glBindTexture (GL_TEXTURE_2D, s_texid_dummy);
    glDrawArrays (GL_LINES, 0, 2);

    glDisable (GL_BLEND);

    return 0;
}


/**
 * Draws a point in the scene.
 *
 * @param mtxGlobal The global model-view matrix.
 * @param p0 The position of the point.
 * @param color The color of the point.
 *
 * @returns None
 */
int
draw_point (float *mtxGlobal, float *p0, float *color)
{
    float matMV[16], matPMV[16], matMVI3x3[9];

    glEnable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);

    glUseProgram( s_sobj.program );

    glEnableVertexAttribArray (s_sobj.loc_vtx);
    glEnableVertexAttribArray (s_sobj.loc_uv );
    glDisableVertexAttribArray(s_sobj.loc_nrm);
    glVertexAttribPointer (s_sobj.loc_vtx, 3, GL_FLOAT, GL_FALSE, 0, p0);
    glVertexAttribPointer (s_sobj.loc_uv , 2, GL_FLOAT, GL_FALSE, 0, s_uv );
    glVertexAttrib4fv (s_sobj.loc_nrm, s_nrm);

    matrix_identity (matMV);
    compute_invmat3x3 (matMVI3x3, matMV);

    matrix_mult (matMV, mtxGlobal, matMV);
    matrix_mult (matPMV, s_matPrj, matMV);

    glUniformMatrix4fv (s_loc_mtx_mv,   1, GL_FALSE, matMV );
    glUniformMatrix4fv (s_loc_mtx_pmv,  1, GL_FALSE, matPMV);
    glUniformMatrix3fv (s_loc_mtx_nrm,  1, GL_FALSE, matMVI3x3);
    glUniform3f (s_loc_lightpos, 1.0f, 1.0f, 1.0f);
    glUniform3f (s_loc_color, color[0], color[1], color[2]);
    glUniform1f (s_loc_alpha, color[3]);

    glEnable (GL_BLEND);

    glBindTexture (GL_TEXTURE_2D, s_texid_dummy);
    glDrawArrays (GL_POINTS, 0, 1);

    glDisable (GL_BLEND);

    return 0;
}


/**
 * Draws a point array.
 *
 * @param mtxGlobal The global model-view matrix.
 * @param vtx The vertex array.
 * @param uv The texture coordinate array.
 * @param num The number of points to draw.
 * @param texid The texture ID.
 * @param color The color of the points.
 *
 * @returns None
 */
int
draw_point_arrays (float *mtxGlobal, float *vtx, float *uv, int num, int texid, float *color)
{
    float matMV[16], matPMV[16], matMVI3x3[9];

    glEnable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);

    glUseProgram( s_sobj.program );

    glEnableVertexAttribArray (s_sobj.loc_vtx);
    glEnableVertexAttribArray (s_sobj.loc_uv );
    glDisableVertexAttribArray(s_sobj.loc_nrm);
    glVertexAttribPointer (s_sobj.loc_vtx, 3, GL_FLOAT, GL_FALSE, 0, vtx);
    glVertexAttribPointer (s_sobj.loc_uv , 2, GL_FLOAT, GL_FALSE, 0, uv );
    glVertexAttrib4fv (s_sobj.loc_nrm, s_nrm);

    matrix_identity (matMV);
    compute_invmat3x3 (matMVI3x3, matMV);

    matrix_mult (matMV, mtxGlobal, matMV);
    matrix_mult (matPMV, s_matPrj, matMV);

    glUniformMatrix4fv (s_loc_mtx_mv,   1, GL_FALSE, matMV );
    glUniformMatrix4fv (s_loc_mtx_pmv,  1, GL_FALSE, matPMV);
    glUniformMatrix3fv (s_loc_mtx_nrm,  1, GL_FALSE, matMVI3x3);
    glUniform3f (s_loc_lightpos, 1.0f, 1.0f, 1.0f);
    glUniform3f (s_loc_color, color[0], color[1], color[2]);
    glUniform1f (s_loc_alpha, color[3]);

    glEnable (GL_BLEND);

    glBindTexture (GL_TEXTURE_2D, texid);
    glDrawArrays (GL_POINTS, 0, num);

    glDisable (GL_BLEND);

    return 0;
}


/**
 * Creates a mesh object.
 *
 * @param mesh The mesh object to create.
 * @param num_tile_w The number of tiles in the u direction.
 * @param num_tile_h The number of tiles in the v direction.
 *
 * @returns 0 on success, -1 on failure.
 */
int
create_mesh (mesh_obj_t *mesh, int num_tile_w, int num_tile_h)
{
    int num_vtx_u = num_tile_w + 1;
    int num_vtx_v = num_tile_h + 1;
    int num_vtx   = num_vtx_u * num_vtx_v;

    mesh->vtx_array = (float *)malloc (num_vtx * 3 * sizeof(float));
    mesh->uv_array  = (float *)malloc (num_vtx * 2 * sizeof(float));

    GLuint vbos[3];
    glGenBuffers (3, vbos);
    mesh->vbo_vtx = vbos[0];
    mesh->vbo_uv  = vbos[1];
    mesh->vbo_idx = vbos[2];

    int num_tri = num_tile_w * num_tile_h * 2;
    int num_idx = num_tri * 3;
    int idx_buf_size = num_idx * sizeof (unsigned short);
    unsigned short *idx_array = (unsigned short *)malloc (idx_buf_size);

    for (int tile_y = 0; tile_y < num_tile_h; tile_y ++)
    {
        for (int tile_x = 0; tile_x < num_tile_w; tile_x ++)
        {
            int idx = tile_y * num_tile_w + tile_x;

            idx_array[6 * idx + 0] = (tile_y  ) * num_vtx_u + (tile_x);    //  0 +----+ 2      + 3
            idx_array[6 * idx + 1] = (tile_y+1) * num_vtx_u + (tile_x);    //    |   /        /|
            idx_array[6 * idx + 2] = (tile_y  ) * num_vtx_u + (tile_x+1);  //    |  /        / |
            idx_array[6 * idx + 3] = (tile_y  ) * num_vtx_u + (tile_x+1);  //    | /        /  |
            idx_array[6 * idx + 4] = (tile_y+1) * num_vtx_u + (tile_x);    //    |/        /   |
            idx_array[6 * idx + 5] = (tile_y+1) * num_vtx_u + (tile_x+1);  //  1 +      4 +----+ 5
        }
    }
    mesh->idx_array = idx_array;
    mesh->num_tile_w = num_tile_w;
    mesh->num_tile_h = num_tile_h;
    mesh->num_idx    = num_idx;

    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, mesh->vbo_idx);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, idx_buf_size, idx_array, GL_STATIC_DRAW);

    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);

    return 0;
}

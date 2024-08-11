
/********************************************************************************************/
/* KallistiOS Direct PVR API:2.0  PVR TEXTURE CUBE WITH ZOOM AND ROTATION  V1 */
/********************************************************************************************/
/********************************************************************************************/
/* Name:     pngzoom .c */
/* Title:    PVR TEXTURE CUBE WITH ZOOM AND ROTATION Kos Example */
/* Author:   (c)Ian Micheal */
/* Created:   05/08/24 */
/*                                                                                          */
/* Version:  1.0 */
/* Platform: Dreamcast | KallistiOS:2.0 | KOSPVR | */
/*                                                                                          */
/* Description: */
/* The purpose of this example is to show the use of only the KOSPVR API to do
 * 3D matching  */
/* And commented so anyone that knows opengl can use DIRECT NO LAYER KOSPVR API
 */
/* History: version 1 */
/********************************************************************************************/
/********************************************************************************************/
/*        >>>  Help and code examples and advice these people where invaluable
 * <<<         */
/*     Mvp's:  dRxL with mat_perspective_fov and explaining to me the concepts
 */
/*     Mvp's:  Bruce tested and found both annoying bugs and texture distortion.
 */
/*                                                                                          */
/********************************************************************************************/

#include <arch/gdb.h>
#include <dc/fmath.h> /* Fast math library headers for optimized mathematical functions */
#include <dc/matrix.h> /* Matrix library headers for handling matrix operations */
#include <dc/matrix3d.h> /* Matrix3D library headers for handling 3D matrix operations */
#include <dc/pvr.h> /* PVR library headers for PowerVR graphics chip functions */
#include <kos.h> /* Includes necessary KallistiOS (KOS) headers for Dreamcast development */
#include <png/png.h> /* PNG library headers for handling PNG images */
#include <stdio.h> /* Standard I/O library headers for input and output functions */
#include <stdlib.h> /* Standard library headers for general-purpose functions, including abs() */

#define ABS(x) ((x) < 0 ? -(x) : (x))

typedef struct {
  pvr_ptr_t ptr;
  int w, h;
  uint32 fmt;
} kos_texture_t;

typedef void (*perspectiveFun)(float, float, float, float);

extern uint8 romdisk[];
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

static matrix_t perspective_matrix __attribute__((aligned(32)));
static const float tex_coords[4][2] = {{0, 1}, {1, 1}, {0, 0}, {1, 0}};
static const float zoom_speed = 0.30f;
static const float cubescale = 1.0f;
static kos_texture_t *texture = NULL;

/**  Cube vertices and side strips
     7*------------*5
     /|           /|
    / |          / |
  1*============*3 |
   I  |         I  |
   I 6*---------I--*4
   I /          I /
   I/           I/
  0*============*2          */
static const float cube_vertices[8][3] = {
    {-cubescale, -cubescale, +cubescale}, // 0
    {-cubescale, +cubescale, +cubescale}, // 1
    {+cubescale, -cubescale, +cubescale}, // 2
    {+cubescale, +cubescale, +cubescale}, // 3
    {+cubescale, -cubescale, -cubescale}, // 4
    {+cubescale, +cubescale, -cubescale}, // 5
    {-cubescale, -cubescale, -cubescale}, // 6
    {-cubescale, +cubescale, -cubescale}  // 7
};
static const uint8_t cube_side_strips[6][4] = {
    {0, 1, 2, 3}, // Front, 0->1->2 & 2->1->3
    {4, 5, 6, 7}, // Back, 4->5->6 & 6->5->7
    {6, 7, 0, 1}, // Left, 6->7->0 & 0->7->1
    {2, 3, 4, 5}, // Right, 2->3->4 & 4->3->5
    {1, 7, 3, 5}, // Top, 1->7->3 & 3->7->5
    {6, 0, 4, 2}  // Bottom, 6->0->4 & 4->0->2
};

static const uint32_t side_colors[6] = {
    // format:
    0xFFFF0000, // Red
    0xFF00FF00, // Green
    0xFF0000FF, // Blue
    0xFFFFFF00, // Yellow
    0xFFFF00FF, // Cyan
    0xFF00FFFF  // Magenta
};

static struct cube {
  struct {
    float x, y, z;
  } pos;
  struct {
    float x, y;
  } rot;
  struct {
    float x, y;
  } speed;
} cube_state;

// Ensure perspective_mat is aligned to a 32-byte boundary
static matrix_t perspective_mat __attribute__((aligned(32))) = {{0.0f}};
void perspectiveFovRH_NO(float rad, float aspect, float zNear, float zFar) {
  float const h = fcos(0.5 * rad) / fsin(0.5 * rad);
  float const w = h * aspect;
  perspective_mat[0][0] = w;
  perspective_mat[1][1] = h;
  perspective_mat[2][2] = -(zFar + zNear) / (zFar - zNear);
  perspective_mat[2][3] = -1.0f;
  perspective_mat[3][2] = -(2.0 * zFar * zNear) / (zFar - zNear);
  mat_apply(&perspective_mat);
}

void perspectiveFovLH_NO(float rad, float aspect, float zNear, float zFar) {
  float const h = fcos(0.5 * rad) / fsin(0.5 * rad);
  float const w = h * aspect;
  perspective_mat[0][0] = w;
  perspective_mat[1][1] = h;
  perspective_mat[2][2] = (zFar + zNear) / (zFar - zNear);
  perspective_mat[2][3] = 1.0f;
  perspective_mat[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
  mat_apply(&perspective_mat);
}

void infinitePerspectiveRH(float fovy, float aspect, float zNear, float unused) {
  float const range = ftan(fovy * 0.5f) * zNear;
  float const left = -range * aspect;
  float const right = range * aspect;
  float const bottom = -range;
  float const top = range;

  perspective_mat[0][0] = (2.0f * zNear) / (right - left);
  perspective_mat[1][1] = (2.0 * zNear) / (top - bottom);
  perspective_mat[2][2] = -1.0f;
  perspective_mat[2][3] = -1.0f;
  perspective_mat[3][2] = -2.0f * zNear;
  mat_apply(&perspective_mat);
}

void infinitePerspectiveLH(float fovy, float aspect, float zNear, float unused) {
  float const range = ftan(fovy * 0.5f) * zNear;
  float const left = -range * aspect;
  float const right = range * aspect;
  float const bottom = -range;
  float const top = range;

  perspective_mat[0][0] = (2.0f * zNear) / (right - left);
  perspective_mat[1][1] = (2.0 * zNear) / (top - bottom);
  perspective_mat[2][2] = 1.0f;
  perspective_mat[2][3] = 1.0f;
  perspective_mat[3][2] = -2.0f * zNear;
  mat_apply(&perspective_mat);
}

kos_texture_t *load_png_texture(const char *filename) {
  //   printf("Entering load_png_texture\n");
  kos_texture_t *texture;
  kos_img_t img;

  if (png_to_img(filename, PNG_FULL_ALPHA, &img) < 0) {
    //    printf("Failed to load PNG image\n");
    return NULL;
  }

  texture = (kos_texture_t *)malloc(sizeof(kos_texture_t));
  if (!texture) {
    kos_img_free(&img, 0);
    return NULL;
  }

  texture->ptr = pvr_mem_malloc(img.byte_count);
  if (!texture->ptr) {
    free(texture);
    kos_img_free(&img, 0);
    return NULL;
  }

  texture->w = img.w;
  texture->h = img.h;
  texture->fmt = PVR_TXRFMT_ARGB4444;

  pvr_txr_load_kimg(&img, texture->ptr, 0);
  kos_img_free(&img, 0);

  //  printf("Exiting load_png_texture\n");
  return texture;
}

void init_poly_context(pvr_poly_cxt_t *cxt) {
  //   printf("Entering init_poly_context\n");
  pvr_poly_cxt_txr(cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_ARGB4444, texture->w,
                   texture->h, texture->ptr, PVR_FILTER_BILINEAR);
  cxt->gen.culling = PVR_CULLING_CCW;
}

void render_cube(void) {
  pvr_poly_cxt_t cxt;
  pvr_poly_hdr_t hdr;
  pvr_vertex_t *vert;
  pvr_dr_state_t dr_state;

  init_poly_context(&cxt);
  pvr_poly_compile(&hdr, &cxt);
  pvr_prim(&hdr, sizeof(hdr));

  // Calculate cubescale based on cube_z
  // float model_scale = 50.0f;

  mat_load(&perspective_matrix); // mat_identity() not needed here, since we're
                                 // overwriting the internal matrix registers
                                 // with mat_load
  mat_translate(cube_state.pos.x, cube_state.pos.y, cube_state.pos.y);
  const float model_scale = 100.0f;
  mat_scale(cubescale * model_scale, cubescale * model_scale,
            cubescale * model_scale);
  mat_rotate_x(cube_state.rot.x);
  mat_rotate_y(cube_state.rot.z);

  pvr_dr_init(&dr_state);
  // Render all six quads
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 4; j++) {
      vec3f_t v = {.x = cube_vertices[cube_side_strips[i][j]][0],
                   .y = cube_vertices[cube_side_strips[i][j]][1],
                   .z = cube_vertices[cube_side_strips[i][j]][2] - cube_state.pos.z};
      float w = 1.0f;
      mat_trans_single4(v.x, v.y, v.z, w);
      w = 1.0f / w;

      vert = pvr_dr_target(dr_state);
      vert->flags = (j == 3) ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;

      vert->x = v.x * w + 320.0f;
      vert->y = v.y * w + 240.0f;
      vert->z = v.z * w;
      vert->u = tex_coords[j][0];
      vert->v = tex_coords[j][1];
      vert->argb = side_colors[i];
      vert->oargb = 0;
      pvr_dr_commit(vert);
    }
  }
  pvr_dr_finish();
}

void set_perspective_fun(perspectiveFun perfun) {
  mat_identity();
  perfun(90.0f * F_PI / 360.0f, 640.0f / 480.0f, 0.1f, 100.0f);
  point_t eye = {0, 0.0001f, 100.0f};
  point_t center = {0, 0, 0};
  vector_t up = {0, 0, 1};
  mat_lookat(&eye, &center, &up);
  mat_store(&perspective_matrix);
}

int main(int argc, char *argv[]) {
  gdb_init();
  pvr_init_params_t params = {{PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16,
                               PVR_BINSIZE_0, PVR_BINSIZE_0},
                              512 * 1024};

  pvr_init(&params);
  pvr_set_bg_color(0.0f, 0.0f, 0.0f);

  texture = load_png_texture("/rd/crate.png");
  if (!texture) {
    printf("Failed to load texture.\n");
    return -1;
  }

  cube_state.pos.z = 0.0f;
  cube_state.rot.x = 5.0f;
  cube_state.rot.y = 5.0f;

  set_perspective_fun(infinitePerspectiveLH);

  while (1) {
    pvr_wait_ready();
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_OP_POLY);

    render_cube();
    pvr_list_finish();
    pvr_scene_finish();

    // Apply rotation
    cube_state.rot.x += cube_state.speed.x;
    cube_state.rot.y += cube_state.speed.y;

    // Apply friction
    cube_state.speed.x *= 0.99f;
    cube_state.speed.y *= 0.99f;

    // If speed is very low, set it to zero to prevent unwanted rotation
    if (ABS(cube_state.speed.x) < 0.0001f)
      cube_state.speed.x = 0.0f;
    if (ABS(cube_state.speed.y) < 0.0001f)
      cube_state.speed.x = 0.0f;

    MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, state)
    if (state->buttons & CONT_START)
      goto out;

    // Analog stick for X and Y movement
    if (abs(state->joyx) > 16) {
      cube_state.pos.x +=
          (state->joyx / 32768.0f) * 1000.5f; // Increased sensitivity
    }
    if (abs(state->joyy) > 16) {
      cube_state.pos.x -= (state->joyy / 32768.0f) *
                          1000.5f; // Increased sensitivity and inverted Y
    }

    // Trigger handling for zooming
    if (state->ltrig > 16) { // Left trigger to zoom out
      cube_state.pos.z -= (state->ltrig / 255.0f) * zoom_speed;
      printf("cube_state.pos.z. = %f\n", cube_state.pos.z);
    }
    if (state->rtrig > 16) { // Right trigger to zoom in
      cube_state.pos.z += (state->rtrig / 255.0f) * zoom_speed;
      printf("cube_state.pos.z. = %f\n", cube_state.pos.z);
    }

    // Limit the zoom range
    if (cube_state.pos.x < -10.0f)
      cube_state.pos.x = -10.0f; // Farther away
    if (cube_state.pos.x > -0.5f)
      cube_state.pos.x = -0.5f; // Closer to the screen

    // Button controls for rotation speed
    if (state->buttons & CONT_X)
      cube_state.speed.x += 0.001f;
    if (state->buttons & CONT_Y)
      cube_state.speed.x -= 0.001f;
    if (state->buttons & CONT_A)
      cube_state.speed.y += 0.001f;
    if (state->buttons & CONT_B)
      cube_state.speed.y -= 0.001f;
    if (state->buttons & CONT_DPAD_UP) {
      printf("switching to perspectiveFovRH_NO\n");
      set_perspective_fun(perspectiveFovRH_NO);
    }
    if (state->buttons & CONT_DPAD_DOWN) {
      printf("switching to perspectiveFovLH_NO\n");
      set_perspective_fun(perspectiveFovLH_NO);
    }
    if (state->buttons & CONT_DPAD_RIGHT) {
      printf("switching to infinitePerspectiveRH\n");
      set_perspective_fun(infinitePerspectiveRH);
    }
    if (state->buttons & CONT_DPAD_LEFT) {
      printf("switching to infinitePerspectiveLH\n");
      set_perspective_fun(infinitePerspectiveLH);
    }

    MAPLE_FOREACH_END()
  }

out:
  printf("Cleaning up\n");
  pvr_shutdown(); // Clean up PVR resources
  vid_shutdown(); // This function reinitializes the video system to what dcload
                  // and friends expect it to be Run the main application here;

  printf("Exiting main\n");
  return 0;
}
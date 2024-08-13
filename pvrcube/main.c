
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

static inline void vec3f_cross(const vec3f_t *v1, const vec3f_t *v2,
                               vec3f_t *r) {
  r->x = v1->y * v2->z - v1->z * v2->y;
  r->y = v1->z * v2->x - v1->x * v2->z;
  r->z = v1->x * v2->y - v1->y * v2->x;
}

static matrix_t _print_mtrx __attribute__((aligned(32))) = {{0.0f}};
void printmatrix() {
  mat_store(&_print_mtrx);
  printf("-------------------------------------------------\n");
  for (int i = 0; i < 4; i++) {
    printf("|%9.4f |%9.4f |%9.4f |%9.4f |\n", _print_mtrx[i][0],
           _print_mtrx[i][1], _print_mtrx[i][2], _print_mtrx[i][3]);
  }
  printf("-------------------------------------------------\n");
}

static matrix_t _lookAt_mtrx __attribute__((aligned(32))) = {{1.0f}};
void lookAtLH(const vec3f_t *eye, const vec3f_t *center, const vec3f_t *up) {
  vec3f_t f = {center->x - eye->x, center->y - eye->y, center->z - eye->z};
  vec3f_normalize(f.x, f.y, f.z);

  vec3f_t s = {0.0f};
  vec3f_cross(up, &f, &s);
  vec3f_normalize(s.x, s.y, s.z);
  vec3f_t u = {0.0f};
  vec3f_cross(&f, &s, &u);

  _lookAt_mtrx[0][0] = s.x;
  _lookAt_mtrx[1][0] = s.y;
  _lookAt_mtrx[2][0] = s.z;
  _lookAt_mtrx[0][1] = u.x;
  _lookAt_mtrx[1][1] = u.y;
  _lookAt_mtrx[2][1] = u.z;
  _lookAt_mtrx[0][2] = f.x;
  _lookAt_mtrx[1][2] = f.y;
  _lookAt_mtrx[2][2] = f.z;

  vec3f_dot(s.x, s.y, s.z, eye->x, eye->y, eye->z, _lookAt_mtrx[3][0]);
  vec3f_dot(u.x, u.y, u.z, eye->x, eye->y, eye->z, _lookAt_mtrx[3][1]);
  vec3f_dot(f.x, f.y, f.z, eye->x, eye->y, eye->z, _lookAt_mtrx[3][2]);

  for (int i = 0; i < 3; i++) {
    _lookAt_mtrx[3][i] = -_lookAt_mtrx[3][i];
  }

  mat_apply(&_lookAt_mtrx);
}

void lookAtRH(const vec3f_t *eye, const vec3f_t *center, const vec3f_t *up) {
  vec3f_t f = {center->x - eye->x, center->y - eye->y, center->z - eye->z};
  vec3f_normalize(f.x, f.y, f.z);

  vec3f_t s = {0.0f};
  vec3f_cross(up, &f, &s);
  vec3f_normalize(s.x, s.y, s.z);
  vec3f_t u = {0.0f};
  vec3f_cross(&f, &s, &u);

  _lookAt_mtrx[0][0] = s.x;
  _lookAt_mtrx[1][0] = s.y;
  _lookAt_mtrx[2][0] = s.z;
  _lookAt_mtrx[0][1] = u.x;
  _lookAt_mtrx[1][1] = u.y;
  _lookAt_mtrx[2][1] = u.z;
  _lookAt_mtrx[0][2] = -f.x;
  _lookAt_mtrx[1][2] = -f.y;
  _lookAt_mtrx[2][2] = -f.z;

  vec3f_dot(s.x, s.y, s.z, eye->x, eye->y, eye->z, _lookAt_mtrx[3][0]);
  vec3f_dot(u.x, u.y, u.z, eye->x, eye->y, eye->z, _lookAt_mtrx[3][1]);
  vec3f_dot(f.x, f.y, f.z, eye->x, eye->y, eye->z, _lookAt_mtrx[3][2]);

  for (int i = 0; i < 3; i++) {
    _lookAt_mtrx[3][i] = -_lookAt_mtrx[3][i];
  }

  mat_apply(&_lookAt_mtrx);
}

static matrix_t _perspective_mtrx __attribute__((aligned(32))) = {{0.0f}};
void perspectiveFovRH_NO(float rad, float aspect, float zNear, float zFar) {
  float const h = fcos(0.5 * rad) / fsin(0.5 * rad);
  float const w = h * aspect;
  _perspective_mtrx[0][0] = w;
  _perspective_mtrx[1][1] = h;
  _perspective_mtrx[2][2] = -(zFar + zNear) / (zFar - zNear);
  _perspective_mtrx[2][3] = -1.0f;
  _perspective_mtrx[3][2] = -(2.0 * zFar * zNear) / (zFar - zNear);
  mat_apply(&_perspective_mtrx);
}

void perspectiveFovLH_NO(float rad, float aspect, float zNear, float zFar) {
  float const h = fcos(0.5 * rad) / fsin(0.5 * rad);
  float const w = h * aspect;
  _perspective_mtrx[0][0] = w;
  _perspective_mtrx[1][1] = h;
  _perspective_mtrx[2][2] = (zFar + zNear) / (zFar - zNear);
  _perspective_mtrx[2][3] = 1.0f;
  _perspective_mtrx[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
  mat_apply(&_perspective_mtrx);
}

void infinitePerspectiveRH(float fovy, float aspect, float zNear,
                           float unused) {
  float const range = ftan(fovy * 0.5f) * zNear;
  float const left = -range * aspect;
  float const right = range * aspect;
  float const bottom = -range;
  float const top = range;

  _perspective_mtrx[0][0] = (2.0f * zNear) / (right - left);
  _perspective_mtrx[1][1] = (2.0 * zNear) / (top - bottom);
  _perspective_mtrx[2][2] = -1.0f;
  _perspective_mtrx[2][3] = -1.0f;
  _perspective_mtrx[3][2] = -2.0f * zNear;
  mat_apply(&_perspective_mtrx);
}

void infinitePerspectiveLH(float fovy, float aspect, float zNear,
                           float unused) {
  float const range = ftan(fovy * 0.5f) * zNear;
  float const left = -range * aspect;
  float const right = range * aspect;
  float const bottom = -range;
  float const top = range;

  _perspective_mtrx[0][0] = (2.0f * zNear) / (right - left);
  _perspective_mtrx[1][1] = (2.0 * zNear) / (top - bottom);
  _perspective_mtrx[2][2] = 1.0f;
  _perspective_mtrx[2][3] = 1.0f;
  _perspective_mtrx[3][2] = -2.0f * zNear;
  mat_apply(&_perspective_mtrx);
}

void oglperspective(float fovy, float ar, float zNear, float zFar) {
  const float zRange = zNear - zFar;
  float const tanHalfFOV = ftan(fovy * 0.5f);

  _perspective_mtrx[0][0] = 1.0f / (tanHalfFOV * ar);
  _perspective_mtrx[1][1] = 1.0f / tanHalfFOV;
  _perspective_mtrx[2][2] = (-zNear - zFar) / zRange;
  _perspective_mtrx[2][3] = 2.0f * zFar * zNear / zRange;
  _perspective_mtrx[3][2] = 1.0f;

  mat_apply(&_perspective_mtrx);
}

void mat_perspective_wrapper(float fovy, float aspect, float zNear,
                             float zFar) {

  float cot_fovy_2 = 1.0f / ftan(fovy * 0.5f);
  mat_perspective(320.0f, 240.0f, cot_fovy_2, zNear, zFar);
}

static const float tex_coords[4][2] = {
    {0, 0}, // left bottom
    {1, 0}, // left top
    {0, 1}, // right top
    {1, 1}, // right bottom
};

static const float zoom_speed = 0.30f;
static const float cubeextnt = 1.f;
static kos_texture_t *texture;

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
static const vec3f_t cube_vertices[8] = {

    {.x = -cubeextnt, .y = -cubeextnt, .z = +cubeextnt}, // 0
    {.x = -cubeextnt, .y = +cubeextnt, .z = +cubeextnt}, // 1
    {.x = +cubeextnt, .y = -cubeextnt, .z = +cubeextnt}, // 2
    {.x = +cubeextnt, .y = +cubeextnt, .z = +cubeextnt}, // 3
    {.x = +cubeextnt, .y = -cubeextnt, .z = -cubeextnt}, // 4
    {.x = +cubeextnt, .y = +cubeextnt, .z = -cubeextnt}, // 5
    {.x = -cubeextnt, .y = -cubeextnt, .z = -cubeextnt}, // 6
    {.x = -cubeextnt, .y = +cubeextnt, .z = -cubeextnt}  // 7
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
    0x7FFF0000, // Red
    0x7F00FF00, // Green
    0x7F0000FF, // Blue
    0x7FFFFF00, // Yellow
    0x7FFF00FF, // Cyan
    0x7F00FFFF  // Magenta
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
} cube_state = {0};

static float fovy = 10.0f;

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
  pvr_poly_cxt_txr(cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444, texture->w,
                   texture->h, texture->ptr, PVR_FILTER_BILINEAR);
  cxt->gen.culling = PVR_CULLING_NONE;
  cxt->gen.clip_mode = PVR_USERCLIP_DISABLE;
}

const float model_scale = 1.f;
const char *perspectiveFovNames[] = {"oglperspective",
    "perspectiveFovRH_NO", "perspectiveFovLH_NO", "infinitePerspectiveRH",
    "infinitePerspectiveLH", "mat_perspective_wrapper"};
const perspectiveFun perspectiveFuns[] = {
    oglperspective, perspectiveFovRH_NO, perspectiveFovLH_NO, infinitePerspectiveRH,
    infinitePerspectiveLH, mat_perspective_wrapper};
int curspectivefun = 0;

void nextperspectivefun() {

  curspectivefun =
      (curspectivefun + 1) % (sizeof(perspectiveFuns) / sizeof(perspectiveFun));
  printf("switching perspective to: %s\n", perspectiveFovNames[curspectivefun]);
}

void render_cube(void) {
  // Calculate cubeextnt based on cube_z
  mat_identity();
  perspectiveFuns[curspectivefun](fovy * F_PI / 180.0f, 640.0f / 480.0f, -10.f,
                                  +10.0f);

  vec3f_t eye = {0.f, -0.00001f, 1.0f};
  vec3f_t center = {0.f, 0.f, 0.0001f};
  vec3f_t up = {0.0f, 0.0f, -1.f};
  mat_lookat(&eye, &center, &up);
  // lookAtLH(&eye, &center, &up);

  mat_translate(cube_state.pos.x, cube_state.pos.y, cube_state.pos.z);
  mat_scale(model_scale, model_scale, model_scale);

  mat_rotate_x(cube_state.rot.x);
  mat_rotate_y(cube_state.rot.y);
  // mat_rotate_z(cube_state.rot.y);

  vector_t tverts[8] = {0};
  vector_t *vp;

  for (int i = 0; i < 8; i++) {
    vp = tverts + i;
    vp->x = cube_vertices[i].x;
    vp->y = cube_vertices[i].y;
    vp->z = cube_vertices[i].z;
    vp->w = 1.0f;
    mat_trans_single4(vp->x, vp->y, vp->z, vp->w);
    float w = 1.0f / vp->w;
    // vp->x = vp->x * w + 320.0f;
    // vp->y = vp->y * w + 240.0f;
    // vp->z *= vp->w;
    // printf("vp->w: %6.3f, w:%6.3f, vp->z: %6.3f\n", vp->w, w, vp->z);
    vp->x = vp->x + 320.0f;
    vp->y = vp->y + 240.0f;

  }

  pvr_poly_cxt_t cxt;
  pvr_dr_state_t dr_state;
  pvr_poly_hdr_t hdr;
  pvr_vertex_t *vert;
  init_poly_context(&cxt);
  pvr_poly_compile(&hdr, &cxt);
  pvr_prim(&hdr, sizeof(hdr));
  pvr_dr_init(&dr_state);
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 4; j++) {
      vp = tverts + cube_side_strips[i][j];
      vert = pvr_dr_target(dr_state);
      vert->flags = (j == 3) ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
      vert->x = vp->x;
      vert->y = vp->y;
      vert->z = vp->z;
      vert->u = tex_coords[j][0];
      vert->v = tex_coords[j][1];
      vert->argb = side_colors[i];
      vert->oargb = 0;
      pvr_dr_commit(vert);
    }
  }
  pvr_dr_finish();
}

int main(int argc, char *argv[]) {
  gdb_init();
  pvr_init_params_t params = {{PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16,
                               PVR_BINSIZE_0, PVR_BINSIZE_0},
                              512 * 1024};

  pvr_init(&params);
  pvr_set_bg_color(0, 0, 0);

  texture = load_png_texture("/rd/dc.png");
  if (!texture) {
    printf("Failed to load texture.\n");
    return -1;
  }

  cube_state.pos.z = 1.0f;
  cube_state.rot.x = 0;
  cube_state.rot.y = 5;

  printmatrix(); // call once to keep it from being optimized out, it is nice to
                 // have as watch expression in gdb when stepping matrix
                 // construction

  int right_down = 0;

  while (1) {
    vid_border_color(0, 255, 0);
    pvr_wait_ready();
    vid_border_color(0, 0, 255);

    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_TR_POLY);

    render_cube();
    pvr_list_finish();
    pvr_scene_finish();
    vid_border_color(255, 0, 0);
    // Apply rotation
    cube_state.rot.x += cube_state.speed.x;
    cube_state.rot.y += cube_state.speed.y;

    // Apply friction
    cube_state.speed.x *= 0.99f;
    cube_state.speed.y *= 0.99f;

    // If speed is very low, set it to zero to prevent unwanted rotation
    if (ABS(cube_state.speed.x) < 0.0001f)
      cube_state.speed.x = 0;
    if (ABS(cube_state.speed.y) < 0.0001f)
      cube_state.speed.x = 0;

    MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, state)
    if (state->buttons & CONT_START)
      goto out;

    // Analog stick for X and Y movement
    if (abs(state->joyx) > 16) {
      cube_state.pos.x -=
          (state->joyx / 32768.0f) * 10.5f; // Increased sensitivity
      printf("cube_state.pos.x = %f\n", cube_state.pos.x);
    }
    if (abs(state->joyy) > 16) {
      cube_state.pos.y -= (state->joyy / 32768.0f) *
                          10.5f; // Increased sensitivity and inverted Y
      printf("cube_state.pos.y = %f\n", cube_state.pos.y);
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
    if (cube_state.pos.z < -10.0f)
      cube_state.pos.z = -10.0f; // Farther away
    if (cube_state.pos.z > 10.0f)
      cube_state.pos.z = 10.0f; // Closer to the screen

    // Button controls for rotation speed
    if (state->buttons & CONT_X)
      cube_state.speed.x += 0.001f;
    if (state->buttons & CONT_Y)
      cube_state.speed.x -= 0.001f;
    if (state->buttons & CONT_A)
      cube_state.speed.y += 0.001f;
    if (state->buttons & CONT_B)
      cube_state.speed.y -= 0.001f;

    if (state->buttons & CONT_DPAD_LEFT) {
      cube_state = (struct cube){0};
      cube_state.pos.z = 1.0f;
      cube_state.rot.y = 5.0f;
      fovy = 10.0f;
      printf("current perspective: %s\n", perspectiveFovNames[curspectivefun]);
    }
    if (state->buttons & CONT_DPAD_DOWN) {
      fovy -= 1.0f;
      printf("fovy = %f\n", fovy);
    }
    if (state->buttons & CONT_DPAD_UP) {
      fovy += 1.0f;
      printf("fovy = %f\n", fovy);
    }

    if (state->buttons & CONT_DPAD_RIGHT) {
      right_down = 1;
    } else if (right_down) {
      nextperspectivefun();
      right_down = 0;
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

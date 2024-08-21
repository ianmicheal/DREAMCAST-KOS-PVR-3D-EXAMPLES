
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

// #include <arch/gdb.h>
#include <dc/fmath.h> /* Fast math library headers for optimized mathematical functions */
#include <dc/matrix.h> /* Matrix library headers for handling matrix operations */
#include <dc/matrix3d.h> /* Matrix3D library headers for handling 3D matrix operations */
#include <dc/pvr.h> /* PVR library headers for PowerVR graphics chip functions */
#include <kos.h> /* Includes necessary KallistiOS (KOS) headers for Dreamcast development */
#include <png/png.h> /* PNG library headers for handling PNG images */
#include <stdio.h> /* Standard I/O library headers for input and output functions */
#include <stdlib.h> /* Standard library headers for general-purpose functions, including abs() */

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define SUPERSAMPLING 1
#if SUPERSAMPLING == 1
#define XSCALE 2.0f
#else
#define XSCALE 1.0f
#endif

#define DEBUG

#include "../cube.h"        /* Cube vertices and side strips layout */
#include "../perspective.h" /* Perspective projection matrix functions */
#include "../pvrtex.h"      /* texture management, single header code */

#define DEFAULT_FOV 45.0f
#define ZOOM_SPEED 0.3f
#define MODEL_SCALE 3.0f
#define MIN_ZOOM -10.0f
#define MAX_ZOOM 15.0f

#define WIREFRAME_MIN_GRID_SIZE 0
#define WIREFRAME_MAX_GRID_SIZE 12
#define WIREFRAME_GRID_SIZE_STEP 4

typedef enum {
  TEXTURED_TR,
  CUBES_CUBE,
  CUBES_CUBE_MAX,
  SPARSE_WIREFRAME,
  DENSE_WIREFRAME,
  MAX_RENDERMODE
} render_mode_e;

static render_mode_e render_mode = TEXTURED_TR;

typedef struct {
  pvr_ptr_t ptr;
  int w, h;
  uint32 fmt;
} kos_texture_t;

static struct {
  uint32_t A : 1;
  uint32_t B : 1;
  uint32_t X : 1;
  uint32_t Y : 1;
  uint32_t START : 1;
  uint32_t DPAD_UP : 1;
  uint32_t DPAD_DOWN : 1;
  uint32_t DPAD_LEFT : 1;
  uint32_t DPAD_RIGHT : 1;
} buttons;

extern uint8 romdisk[];
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

static float fovy = DEFAULT_FOV;

static const float tex_coords[4][2] = {
    {0, 0}, // left bottom
    {0, 1}, // left top
    {1, 0}, // right bottom
    {1, 1}, // right top
};

static dttex_info_t texture256;
static dttex_info_t texture128;
static dttex_info_t texture64;

#define LINE_WIDTH 1.0f
static inline void draw_line(vec3f_t *from, vec3f_t *to, float centerz,
                             pvr_dr_state_t *dr_state) {
  pvr_sprite_col_t *quad = (pvr_sprite_col_t *)pvr_dr_target(*dr_state);
  quad->flags = PVR_CMD_VERTEX_EOL;

  if (from->x > to->x) {
    vec3f_t *tmp = from;
    from = to;
    to = tmp;
  }
  vec3f_t direction = {to->x - from->x, to->y - from->y, to->z - from->z};

  vec3f_normalize(direction.x, direction.y, direction.z);

  quad->ax = from->x;
  quad->ay = from->y;
  quad->az = from->z + centerz * 0.1;
  quad->bx = to->x;
  quad->by = to->y;
  quad->bz = to->z + centerz * 0.1;
  quad->cx = to->x + LINE_WIDTH * XSCALE * direction.y;

  pvr_dr_commit(quad);
  quad = (pvr_sprite_col_t *)pvr_dr_target(*dr_state);

  // make a pointer with 32 bytes negative offset to allow correct access
  // to the second half of the quad
  pvr_sprite_col_t *quad2ndhalf = (pvr_sprite_col_t *)((int)quad - 32);
  quad2ndhalf->cy = to->y - LINE_WIDTH * direction.x;
  quad2ndhalf->cz = to->z + centerz * 0.1;
  quad2ndhalf->dx = from->x + LINE_WIDTH * XSCALE * direction.y;
  quad2ndhalf->dy = from->y - LINE_WIDTH * direction.x;

  pvr_dr_commit(quad);
}

void render_wire_grid(vec3f_t *min, vec3f_t *max, vec3f_t *dir1, vec3f_t *dir2,
                      int num_lines, uint32_t color, pvr_dr_state_t *dr_state) {
  vec3f_t step = {(max->x - min->x) / (num_lines + 1),
                  (max->y - min->y) / (num_lines + 1),
                  (max->z - min->z) / (num_lines + 1)};

  if (color != 0) {
    pvr_sprite_cxt_t cxt;
    pvr_sprite_cxt_col(&cxt, PVR_LIST_OP_POLY);
    cxt.gen.culling = PVR_CULLING_NONE; // disable culling for polygons
                                        // facing away from the camera
    pvr_sprite_hdr_t *hdrpntr = (pvr_sprite_hdr_t *)pvr_dr_target(*dr_state);
    pvr_sprite_compile(hdrpntr, &cxt);
    hdrpntr->argb = color;
    pvr_dr_commit(hdrpntr);
  }
  vec3f_t twolines[4] = {0};
  vec3f_t *from_v = twolines + 0;
  vec3f_t *to_v = twolines + 1;
  vec3f_t *from_h = twolines + 2;
  vec3f_t *to_h = twolines + 3;

  for (int i = 1; i <= num_lines; i++) {
    from_v->x = min->x + i * step.x * dir1->x;
    from_v->y = min->y + i * step.y * dir1->y;
    from_v->z = min->z + i * step.y * dir1->z;

    to_v->x = dir1->x == 0.0f ? max->x : min->x + i * step.x * dir1->x;
    to_v->y = dir1->y == 0.0f ? max->y : min->y + i * step.y * dir1->y;
    to_v->z = dir1->z == 0.0f ? max->z : min->z + i * step.z * dir1->z;

    from_h->x = min->x + i * step.x * dir2->x;
    from_h->y = min->y + i * step.y * dir2->y;
    from_h->z = min->z + i * step.z * dir2->z;
    to_h->x = dir2->x == 0.0f ? max->x : min->x + i * step.x * dir2->x;
    to_h->y = dir2->y == 0.0f ? max->y : min->y + i * step.y * dir2->y;
    to_h->z = dir2->z == 0.0f ? max->z : min->z + i * step.z * dir2->z;

    mat_transform((vector_t *)twolines, (vector_t *)twolines, 4,
                  sizeof(vec3f_t));
    draw_line(from_v, to_v, 0, dr_state);
    draw_line(from_h, to_h, 0, dr_state);
  }
  draw_line(min, max, 0, dr_state);
}

void render_wire_cube(void) {
  mat_load(&stored_projection_view);
  mat_translate(cube_state.pos.x, cube_state.pos.y, cube_state.pos.z);
  mat_scale(MODEL_SCALE * XSCALE, MODEL_SCALE, MODEL_SCALE);
  mat_rotate_x(cube_state.rot.x);
  mat_rotate_y(cube_state.rot.y);

  vec3f_t tverts[8] __attribute__((aligned(32))) = {0};
  mat_transform((vector_t *)&cube_vertices, (vector_t *)&tverts, 8,
                sizeof(vec3f_t));

  pvr_dr_state_t dr_state;

  pvr_sprite_cxt_t cxt;
  pvr_sprite_cxt_col(&cxt, PVR_LIST_OP_POLY);

  // pvr_sprite_col_t cxt;
  // pvr_sprite_cxt_col(&cxt, PVR_LIST_OP_POLY);
  cxt.gen.culling = PVR_CULLING_NONE; // disable culling for polygons
                                      // facing away from the camera

  pvr_dr_init(&dr_state);
  pvr_sprite_hdr_t hdr;
  pvr_sprite_compile(&hdr, &cxt);

  for (int i = 0; i < 6; i++) {
    pvr_sprite_hdr_t *hdrpntr = (pvr_sprite_hdr_t *)pvr_dr_target(dr_state);
    hdr.argb = cube_side_colors[i];
    *hdrpntr = hdr;
    pvr_dr_commit(hdrpntr);
    vec3f_t *ac = tverts + cube_side_strips[i][0];
    vec3f_t *bc = tverts + cube_side_strips[i][2];
    vec3f_t *cc = tverts + cube_side_strips[i][3];
    vec3f_t *dc = tverts + cube_side_strips[i][1];

    float centerz = (ac->z + bc->z + cc->z + dc->z) / 4.0f;

    draw_line(ac, dc, centerz, &dr_state);
    draw_line(bc, cc, centerz, &dr_state);
    draw_line(dc, cc, centerz, &dr_state);
    draw_line(ac, bc, centerz, &dr_state);
  }

  vec3f_t wiredir1 = (vec3f_t){1, 0, 0};
  vec3f_t wiredir2 = (vec3f_t){0, 1, 0};

  render_wire_grid(cube_vertices + 0, cube_vertices + 3, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[0], &dr_state);

  if (render_mode == DENSE_WIREFRAME) {
    for (int i = 1; i < cube_state.grid_size + 1; i++) {
      vec3f_t inner_from = *(cube_vertices + 0);
      vec3f_t inner_to = *(cube_vertices + 3);
      float z_offset =
          i * ((inner_from.x - inner_to.x) / (cube_state.grid_size + 1));
      inner_from.z += z_offset;
      inner_to.z += z_offset;
      render_wire_grid(&inner_from, &inner_to, &wiredir1, &wiredir2,
                       cube_state.grid_size, 0x55FFFFFF, &dr_state);
    }
  }

  render_wire_grid(cube_vertices + 4, cube_vertices + 7, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[1], &dr_state);

  wiredir2.y = 0;
  wiredir2.z = 1;
  render_wire_grid(cube_vertices + 0, cube_vertices + 4, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[5], &dr_state);

  if (render_mode == DENSE_WIREFRAME) {
    for (int i = 1; i < cube_state.grid_size + 1; i++) {
      vec3f_t inner_from = *(cube_vertices + 0);
      vec3f_t inner_to = *(cube_vertices + 4);
      float y_offset =
          i * ((inner_to.x - inner_from.x) / (cube_state.grid_size + 1));
      inner_from.y += y_offset;
      inner_to.y += y_offset;
      render_wire_grid(&inner_from, &inner_to, &wiredir1, &wiredir2,
                       cube_state.grid_size, 0x55FFFFFF, &dr_state);
    }
  }

  render_wire_grid(cube_vertices + 1, cube_vertices + 5, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[4], &dr_state);

  wiredir1.x = 0;
  wiredir1.z = 1;
  wiredir2.z = 0;
  wiredir2.y = 1;
  render_wire_grid(cube_vertices + 4, cube_vertices + 3, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[3], &dr_state);

  render_wire_grid(cube_vertices + 6, cube_vertices + 1, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[2], &dr_state);
  // render_wire_grid(cube_vertices + 6, cube_vertices + 1, &wiredir1,
  // &wiredir2, 10,inner_from.x
  //                  cube_side_colors[0], &dr_state);

  pvr_dr_finish();
}

void render_txr_cube(void) {
  mat_load(&stored_projection_view);
  mat_translate(cube_state.pos.x, cube_state.pos.y, cube_state.pos.z);
  mat_scale(MODEL_SCALE * XSCALE, MODEL_SCALE, MODEL_SCALE);
  mat_rotate_x(cube_state.rot.x);
  mat_rotate_y(cube_state.rot.y);

  vec3f_t tverts[8] __attribute__((aligned(32))) = {0};
  mat_transform((vector_t *)&cube_vertices, (vector_t *)&tverts, 8,
                sizeof(vec3f_t));

  pvr_dr_state_t dr_state;

  pvr_sprite_cxt_t cxt;
  pvr_sprite_cxt_txr(&cxt, PVR_LIST_TR_POLY, texture256.pvrformat,
                     texture256.width, texture256.height, texture256.ptr,
                     PVR_FILTER_BILINEAR);

  // pvr_sprite_col_t cxt;
  // pvr_sprite_cxt_col(&cxt, PVR_LIST_TR_POLY);
  cxt.gen.culling = PVR_CULLING_NONE; // disable culling for polygons facing
                                      // away from the camera
  cxt.gen.specular = PVR_SPECULAR_ENABLE;

  pvr_dr_init(&dr_state);
  pvr_sprite_hdr_t hdr;
  pvr_sprite_compile(&hdr, &cxt);
  pvr_sprite_hdr_t *hdrpntr = (pvr_sprite_hdr_t *)pvr_dr_target(dr_state);
  hdr.argb = 0x7FFFFFFF;

  for (int i = 0; i < 6; i++) {
    *hdrpntr = hdr;
    hdrpntr->oargb = cube_side_colors[i];
    pvr_dr_commit(hdrpntr);
    vec3f_t *ac = tverts + cube_side_strips[i][0];
    vec3f_t *bc = tverts + cube_side_strips[i][2];
    vec3f_t *cc = tverts + cube_side_strips[i][3];
    vec3f_t *dc = tverts + cube_side_strips[i][1];

    pvr_sprite_txr_t *quad = (pvr_sprite_txr_t *)pvr_dr_target(dr_state);
    quad->flags = PVR_CMD_VERTEX_EOL;
    quad->ax = ac->x;
    quad->ay = ac->y;
    quad->az = ac->z;
    quad->bx = bc->x;
    quad->by = bc->y;
    quad->bz = bc->z;
    quad->cx = cc->x;
    pvr_dr_commit(quad);
    quad = (pvr_sprite_txr_t *)pvr_dr_target(dr_state);

    // make a pointer with 32 bytes negative offset to allow correct access
    // to the second half of the quad
    pvr_sprite_txr_t *quad2ndhalf = (pvr_sprite_txr_t *)((int)quad - 32);
    quad2ndhalf->cy = cc->y;
    quad2ndhalf->cz = cc->z;
    quad2ndhalf->dx = dc->x;
    quad2ndhalf->dy = dc->y;
    // note that dc->z is infered from
    quad2ndhalf->auv = PVR_PACK_16BIT_UV(tex_coords[0][0], tex_coords[0][1]);
    quad2ndhalf->buv = PVR_PACK_16BIT_UV(tex_coords[2][0], tex_coords[2][1]);
    quad2ndhalf->cuv = PVR_PACK_16BIT_UV(tex_coords[3][0], tex_coords[3][1]);

    pvr_dr_commit(quad);
  }
  pvr_dr_finish();
}

void render_cubes_cube() {
  mat_load(&stored_projection_view);
  mat_translate(cube_state.pos.x, cube_state.pos.y, cube_state.pos.z);
  mat_scale(MODEL_SCALE * XSCALE, MODEL_SCALE, MODEL_SCALE);
  mat_rotate_x(cube_state.rot.x);
  mat_rotate_y(cube_state.rot.y);

  pvr_dr_state_t dr_state;

  pvr_sprite_cxt_t cxt;

  uint32_t sqrt_cubes = 8;

  if (render_mode == CUBES_CUBE_MAX) {
    sqrt_cubes = 12;
    pvr_sprite_cxt_txr(
        &cxt, PVR_LIST_OP_POLY, texture64.pvrformat | PVR_TXRFMT_4BPP_PAL(0),
        texture64.width, texture64.height, texture64.ptr, PVR_FILTER_BILINEAR);

  } else {
    pvr_sprite_cxt_txr(
        &cxt, PVR_LIST_OP_POLY, texture128.pvrformat | PVR_TXRFMT_8BPP_PAL(1),
        texture128.width, texture128.height, texture128.ptr, PVR_FILTER_BILINEAR);
  }

  // cxt.gen.culling = PVR_CULLING_CCW;
  cxt.gen.specular = PVR_SPECULAR_ENABLE;

  pvr_dr_init(&dr_state);
  pvr_sprite_hdr_t hdr;
  pvr_sprite_compile(&hdr, &cxt);
  pvr_sprite_hdr_t *hdrpntr = (pvr_sprite_hdr_t *)pvr_dr_target(dr_state);
  hdr.argb = 0x7FFFFFFF;

  vec3f_t cube_min = cube_vertices[6];
  vec3f_t cube_max = cube_vertices[3];

  vec3f_t cube_step = {
      (cube_max.x - cube_min.x) / sqrt_cubes,
      (cube_max.y - cube_min.y) / sqrt_cubes,
      (cube_max.z - cube_min.z) / sqrt_cubes,
  };

  vec3f_t cube_size = {
      cube_step.x * 0.75f,
      cube_step.y * 0.75f,
      cube_step.z * 0.75f,
  };

  for (int cx = 0; cx < sqrt_cubes; cx++) {
    for (int cy = 0; cy < sqrt_cubes; cy++) {
      for (int cz = 0; cz < sqrt_cubes; cz++) {
        vec3f_t cube_pos = {cube_min.x + cube_step.x * (float)cx,
                            cube_min.y + cube_step.y * (float)cy,
                            cube_min.z + cube_step.z * (float)cz};

        vec3f_t tverts[8] __attribute__((aligned(32))) = {
            {.x = cube_pos.x, .y = cube_pos.y, .z = cube_pos.z + cube_size.z},
            {.x = cube_pos.x,
             .y = cube_pos.y + cube_size.y,
             .z = cube_pos.z + cube_size.z},
            {.x = cube_pos.x + cube_size.x,
             .y = cube_pos.y,
             .z = cube_pos.z + cube_size.z},
            {.x = cube_pos.x + cube_size.x,
             .y = cube_pos.y + cube_size.y,
             .z = cube_pos.z + cube_size.z},
            {.x = cube_pos.x + cube_size.x, .y = cube_pos.y, .z = cube_pos.z},
            {.x = cube_pos.x + cube_size.x,
             .y = cube_pos.y + cube_size.y,
             .z = cube_pos.z},
            {.x = cube_pos.x, .y = cube_pos.y, .z = cube_pos.z},
            {.x = cube_pos.x, .y = cube_pos.y + cube_size.y, .z = cube_pos.z}};

        mat_transform((vector_t *)&tverts, (vector_t *)&tverts, 8,
                      sizeof(vec3f_t));

        for (int i = 0; i < 6; i++) {
          *hdrpntr = hdr;
          hdrpntr->oargb = cube_side_colors[i];
          pvr_dr_commit(hdrpntr);
          vec3f_t *ac = tverts + cube_side_strips[i][0];
          vec3f_t *bc = tverts + cube_side_strips[i][2];
          vec3f_t *cc = tverts + cube_side_strips[i][3];
          vec3f_t *dc = tverts + cube_side_strips[i][1];

          pvr_sprite_txr_t *quad = (pvr_sprite_txr_t *)pvr_dr_target(dr_state);
          quad->flags = PVR_CMD_VERTEX_EOL;
          quad->ax = ac->x;
          quad->ay = ac->y;
          quad->az = ac->z;
          quad->bx = bc->x;
          quad->by = bc->y;
          quad->bz = bc->z;
          quad->cx = cc->x;
          pvr_dr_commit(quad);
          quad = (pvr_sprite_txr_t *)pvr_dr_target(dr_state);

          // make a pointer with 32 bytes negative offset to allow correct
          // access to the second half of the quad
          pvr_sprite_txr_t *quad2ndhalf = (pvr_sprite_txr_t *)((int)quad - 32);
          quad2ndhalf->cy = cc->y;
          quad2ndhalf->cz = cc->z;
          quad2ndhalf->dx = dc->x;
          quad2ndhalf->dy = dc->y;
          // note that dc->z is infered from
          quad2ndhalf->auv =
              PVR_PACK_16BIT_UV(tex_coords[0][0], tex_coords[0][1]);
          quad2ndhalf->buv =
              PVR_PACK_16BIT_UV(tex_coords[2][0], tex_coords[2][1]);
          quad2ndhalf->cuv =
              PVR_PACK_16BIT_UV(tex_coords[3][0], tex_coords[3][1]);

          pvr_dr_commit(quad);
        }
      };
    }
  }
  pvr_dr_finish();
}

static inline void cube_reset_state() {
  uint32_t grid_size = cube_state.grid_size;
  cube_state = (struct cube){0};
  cube_state.grid_size = grid_size;
  fovy = DEFAULT_FOV;
  cube_state.pos.z = 7.0f;
  cube_state.rot.x = F_PI / 4.0f;
  cube_state.rot.y = F_PI / 4.0f;
  update_projection_view(fovy);
}

int update_state() {
  int keep_running = 1;
  MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, state)
  if (state->buttons & CONT_START) {
    keep_running = 0;
  }

  // Analog stick for X and Y movement
  if (abs(state->joyx) > 16) {
    cube_state.pos.x +=
        (state->joyx / 32768.0f) * 20.5f; // Increased sensitivity
  }
  if (abs(state->joyy) > 16) {
    cube_state.pos.y += (state->joyy / 32768.0f) *
                        20.5f; // Increased sensitivity and inverted Y
  }

  // Trigger handling for zooming
  if (state->ltrig > 16) { // Left trigger to zoom out
    cube_state.pos.z -= (state->ltrig / 255.0f) * ZOOM_SPEED;
  }
  if (state->rtrig > 16) { // Right trigger to zoom in
    cube_state.pos.z += (state->rtrig / 255.0f) * ZOOM_SPEED;
  }

  // Limit the zoom range
  if (cube_state.pos.z < MIN_ZOOM)
    cube_state.pos.z = MIN_ZOOM; // Farther away
  if (cube_state.pos.z > MAX_ZOOM)
    cube_state.pos.z = MAX_ZOOM; // Closer to the screen

  // Button controls for rotation speed
  if (state->buttons & CONT_X)
    cube_state.speed.y += 0.001f;
  if (state->buttons & CONT_B)
    cube_state.speed.y -= 0.001f;
  if (state->buttons & CONT_A)
    cube_state.speed.x += 0.001f;
  if (state->buttons & CONT_Y)
    cube_state.speed.x -= 0.001f;

  if (state->buttons & CONT_DPAD_LEFT) {
    // cube_state = (struct cube){0};
    fovy = DEFAULT_FOV;
    cube_reset_state();
  }
  if (state->buttons & CONT_DPAD_RIGHT) {
    if (buttons.DPAD_RIGHT == 0) {
      buttons.DPAD_RIGHT = 1;
      switch (render_mode) {
      case TEXTURED_TR:
      case CUBES_CUBE:
      case CUBES_CUBE_MAX:
        render_mode++;
        break;
      default:
        cube_state.grid_size += WIREFRAME_GRID_SIZE_STEP;
        if (cube_state.grid_size > WIREFRAME_MAX_GRID_SIZE) {
          cube_state.grid_size = WIREFRAME_MIN_GRID_SIZE;
          render_mode++;
          if (render_mode >= MAX_RENDERMODE) {
            render_mode = TEXTURED_TR;
          }
        }
      }
    }
  } else {
    buttons.DPAD_RIGHT = 0;
  }

  if (state->buttons & CONT_DPAD_DOWN) {
    fovy -= 1.0f;
    update_projection_view(fovy);
  }
  if (state->buttons & CONT_DPAD_UP) {
    fovy += 1.0f;
    update_projection_view(fovy);
  }
  MAPLE_FOREACH_END()

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

  return keep_running;
}

int main(int argc, char *argv[]) {

#ifdef DEBUG
  gdb_init();
#endif

  pvr_set_bg_color(0.0, 0.0, 24.0f / 255.0f);
  pvr_init_params_t params = {
      {PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0,
       PVR_BINSIZE_0},
      1536 * 1024,

      0,             // No DMA
      SUPERSAMPLING, //  Set horisontal FSAA
      0,             // Translucent Autosort enabled.
      2              // Extra OPBs
  };
  pvr_init(&params);
  pvr_set_bg_color(0, 0, 0);
  if (!pvrtex_load("/rd/texture/rgb565_vq_tw/dc.dt", &texture256)) {
    printf("Failed to load texture256.\n");
    return -1;
  }
  if (!pvrtex_load("/rd/texture/pal8/dc_128sq_256colors.dt", &texture128)) {
    printf("Failed to load texture128.\n");
    return -1;
  }
  if (!pvrtex_load_palette("/rd/texture/pal8/dc_128sq_256colors.dt.pal",
                    PVR_PAL_ARGB8888, 256)) {
    printf("Failed to load palette.\n");
    return -1;
  }
  if (!pvrtex_load("/rd/texture/pal4/dc_64sq_16colors.dt", &texture64)) {
    printf("Failed to load texture64.\n");
    return -1;
  }
  if (!pvrtex_load_palette("/rd/texture/pal4/dc_64sq_16colors.dt.pal",
                    PVR_PAL_ARGB8888, 0)) {
    printf("Failed to load palette.\n");
    return -1;
  }

  cube_reset_state();
  int keep_running = 1;

  while (keep_running) {
    keep_running = update_state();

#ifdef DEBUG
    vid_border_color(255, 0, 0);
#endif
    pvr_wait_ready();

#ifdef DEBUG
    vid_border_color(0, 255, 0);
#endif
    pvr_scene_begin();

    switch (render_mode) {
    case TEXTURED_TR:
      pvr_list_begin(PVR_LIST_TR_POLY);
      render_txr_cube();
      pvr_list_finish();
      break;
    case DENSE_WIREFRAME:
    case SPARSE_WIREFRAME:
      pvr_list_begin(PVR_LIST_OP_POLY);
      render_wire_cube();
      pvr_list_finish();
      break;
    case CUBES_CUBE_MAX:
    case CUBES_CUBE:
      pvr_list_begin(PVR_LIST_OP_POLY);
      render_cubes_cube();
      pvr_list_finish();
      break;
    default:
      break;
    }
#ifdef DEBUG
    vid_border_color(0, 0, 255);
#endif
    // if (!keep_running) {
    //   pvr_stats_t stats;
    //   pvr_get_stats(&stats);
    //   printf("Last frame stats:\n"
    //          "frame_count: %lu\n"
    //          "rnd_last_time: %lu\n"
    //          "vtx_buffer_used: %lu\n"
    //          "vtx_buffer_used_max: %lu\n"
    //          "frame_rate %f\n",
    //          stats.frame_count, stats.rnd_last_time, stats.vtx_buffer_used,
    //          stats.vtx_buffer_used_max, stats.frame_rate);
    // }
    pvr_scene_finish();
  }

  printf("Cleaning up\n");
  pvrtex_unload(&texture256);
  pvr_shutdown(); // Clean up PVR resources
  vid_shutdown(); // This function reinitializes the video system to what dcload
                  // and friends expect it to be Run the main application here;

  printf("Exiting main\n");
  return 0;
}

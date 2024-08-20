
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

#include "../cube.h" /* Cube vertices and side strips layout */
#include "../pvrtex.h" /* texture management, single header code */
#include "../perspective.h" /* Perspective projection matrix functions */

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define DEFAULT_FOV 45.0f
#define ZOOM_SPEED 0.3f
#define MODEL_SCALE 3.0f
#define MIN_ZOOM -10.0f
#define MAX_ZOOM 15.0f
#define LINE_WIDTH 1.f

extern uint8 romdisk[];
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

static float fovy = DEFAULT_FOV;
static dttex_info_t texture256;

static inline void draw_line(vec3f_t *ac, vec3f_t *dc, float centerz,
                             pvr_dr_state_t *dr_state) {
  pvr_sprite_col_t *quad = (pvr_sprite_col_t *)pvr_dr_target(*dr_state);
  quad->flags = PVR_CMD_VERTEX_EOL;

  vec3f_t *from = ac;
  vec3f_t *to = dc;

  if (from->x > to->x) {
    from = dc;
    to = ac;
  }
  vec3f_t direction = {to->x - from->x, to->y - from->y, to->z - from->z};

  vec3f_normalize(direction.x, direction.y, direction.z);

  quad->ax = from->x;
  quad->ay = from->y;
  quad->az = from->z + centerz * 0.1;
  quad->bx = to->x;
  quad->by = to->y;
  quad->bz = to->z + centerz * 0.1;
  quad->cx = to->x + LINE_WIDTH * direction.y;

  pvr_dr_commit(quad);
  quad = (pvr_sprite_col_t *)pvr_dr_target(*dr_state);

  // make a pointer with 32 bytes negative offset to allow correct access
  // to the second half of the quad
  pvr_sprite_txr_t *quad2ndhalf = (pvr_sprite_txr_t *)((int)quad - 32);
  quad2ndhalf->cy = to->y - LINE_WIDTH * direction.x;
  quad2ndhalf->cz = to->z + centerz * 0.1;
  quad2ndhalf->dx = from->x + LINE_WIDTH * direction.y;
  quad2ndhalf->dy = from->y - LINE_WIDTH * direction.x;

  pvr_dr_commit(quad);
}

void render_wire_cube(void) {

  pvr_wait_ready();
  pvr_scene_begin();
  pvr_list_begin(PVR_LIST_OP_POLY);

  mat_load(&_projection_view);
  mat_translate(cube_state.pos.x, cube_state.pos.y, cube_state.pos.z);
  mat_scale(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE);
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
  cxt.gen.culling = PVR_CULLING_NONE; // disable culling for polygons facing
                                      // away from the camera

  pvr_dr_init(&dr_state);
  pvr_sprite_hdr_t hdr;
  pvr_sprite_compile(&hdr, &cxt);
  pvr_sprite_hdr_t *hdrpntr = (pvr_sprite_hdr_t *)pvr_dr_target(dr_state);

  for (int i = 0; i < 6; i++) {
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
  pvr_dr_finish();

  pvr_list_finish();
  pvr_scene_finish();
}

void render_txr_cube(void) {

  pvr_wait_ready();
  pvr_scene_begin();
  pvr_list_begin(PVR_LIST_TR_POLY);

  mat_load(&_projection_view);
  mat_translate(cube_state.pos.x, cube_state.pos.y, cube_state.pos.z);
  mat_scale(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE);
  mat_rotate_x(cube_state.rot.x);
  mat_rotate_y(cube_state.rot.y);

  vec3f_t tverts[8] __attribute__((aligned(32))) = {0};
  mat_transform((vector_t *)&cube_vertices, (vector_t *)&tverts, 8,
                sizeof(vec3f_t));

  pvr_dr_state_t dr_state;

  pvr_sprite_cxt_t cxt;
  pvr_sprite_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444, texture256->w,
                     texture256->h, texture256->ptr, PVR_FILTER_BILINEAR);

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
    // note that dc->z is infered from cc->z
    quad2ndhalf->auv = PVR_PACK_16BIT_UV(tex_coords[0][0], tex_coords[0][1]);
    quad2ndhalf->buv = PVR_PACK_16BIT_UV(tex_coords[2][0], tex_coords[2][1]);
    quad2ndhalf->cuv = PVR_PACK_16BIT_UV(tex_coords[3][0], tex_coords[3][1]);

    pvr_dr_commit(quad);
  }
  pvr_dr_finish();

  pvr_list_finish();
  pvr_scene_finish();
}

static inline void cube_startpos() {
  cube_state = (struct cube){0};
  fovy = DEFAULT_FOV;
  cube_state.pos.z = (MAX_ZOOM + MIN_ZOOM) / 2.0f;
  cube_state.rot.x = 0.5;
  cube_state.rot.y = 0.5;
  update_projection_view();
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
    cube_state.speed.x += 0.001f;
  if (state->buttons & CONT_Y)
    cube_state.speed.x -= 0.001f;
  if (state->buttons & CONT_A)
    cube_state.speed.y += 0.001f;
  if (state->buttons & CONT_B)
    cube_state.speed.y -= 0.001f;

  if (state->buttons & CONT_DPAD_LEFT) {
    cube_state = (struct cube){0};
    fovy = DEFAULT_FOV;
    cube_startpos();
  }
  if (state->buttons & CONT_DPAD_DOWN) {
    fovy -= 1.0f;
    update_projection_view();
  }
  if (state->buttons & CONT_DPAD_UP) {
    fovy += 1.0f;
    update_projection_view();
  }

  if (state->buttons & CONT_DPAD_RIGHT) {
    printf("fovy = %f\n"
           "cube_state.pos.x = %f\n"
           "cube_state.pos.y = %f\n"
           "cube_state.pos.z = %f\n"
           "cube_state.rot.x = %f\n"
           "cube_state.rot.y = %f\n"
           "cube_state.speed.x = %f\n"
           "cube_state.speed.y = %f\n",
           fovy, cube_state.pos.x, cube_state.pos.y, cube_state.pos.z,
           cube_state.rot.x, cube_state.rot.y, cube_state.speed.x,
           cube_state.speed.y);
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
  // gdb_init();
  pvr_init_params_t params = {
      {PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0,
       PVR_BINSIZE_0},
      512 * 1024,

      0, // No DMA
      1, //  Enable horisontal FSAA
      0  // Translucent Autosort enabled.
  };
  pvr_init(&params);
  pvr_set_bg_color(0, 0, 0);

  texture256 = load_png_texture("/rd/dc.png");
  if (!texture256) {
    printf("Failed to load texture256.\n");
    return -1;
  }

  cube_startpos();
  int keep_running = 1;
  while (keep_running) {
    keep_running = update_state();
    render_txr_cube();
  }

  printf("Cleaning up\n");
  pvr_shutdown(); // Clean up PVR resources
  vid_shutdown(); // This function reinitializes the video system to what dcload
                  // and friends expect it to be Run the main application here;

  printf("Exiting main\n");
  return 0;
}
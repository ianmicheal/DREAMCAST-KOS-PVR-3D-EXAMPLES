
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

#define DEFAULT_FOV 45.0f
#define ZOOM_SPEED 0.3f
#define CUBEEXTENT 1.0f
#define MODEL_SCALE 3.0f
#define MIN_ZOOM -10.0f
#define MAX_ZOOM 15.0f

#define SUPERSAMPLING 1
#if SUPERSAMPLING == 1
#define XSCALE 2.0f
#else
#define XSCALE 1.0f
#endif

#define DEBUG

typedef enum { WIREFRAME, TEXTURED } render_mode_e;

static render_mode_e render_mode = TEXTURED;

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

static kos_texture_t *texture256;

/**  Cube vertices and side strips layout:
     7*-----------*5
     /|          /|
    / |         / |
  1*===========*3 |
   I  |        I  |
   I 6*--------I--*4
   I /         I /
   I/          I/
  0*===========*2          */
static const vec3f_t cube_vertices[8] __attribute__((aligned(32))) = {
    {.x = -CUBEEXTENT, .y = -CUBEEXTENT, .z = +CUBEEXTENT}, // 0
    {.x = -CUBEEXTENT, .y = +CUBEEXTENT, .z = +CUBEEXTENT}, // 1
    {.x = +CUBEEXTENT, .y = -CUBEEXTENT, .z = +CUBEEXTENT}, // 2
    {.x = +CUBEEXTENT, .y = +CUBEEXTENT, .z = +CUBEEXTENT}, // 3
    {.x = +CUBEEXTENT, .y = -CUBEEXTENT, .z = -CUBEEXTENT}, // 4
    {.x = +CUBEEXTENT, .y = +CUBEEXTENT, .z = -CUBEEXTENT}, // 5
    {.x = -CUBEEXTENT, .y = -CUBEEXTENT, .z = -CUBEEXTENT}, // 6
    {.x = -CUBEEXTENT, .y = +CUBEEXTENT, .z = -CUBEEXTENT}  // 7
};
static const uint8_t cube_side_strips[6][4] __attribute__((aligned(32))) = {
    {0, 1, 2, 3}, // Front, 0->1->2 & 2->1->3
    {4, 5, 6, 7}, // Back, 4->5->6 & 6->5->7
    {6, 7, 0, 1}, // Left, 6->7->0 & 0->7->1
    {2, 3, 4, 5}, // Right, 2->3->4 & 4->3->5
    {1, 7, 3, 5}, // Top, 1->7->3 & 3->7->5
    {6, 0, 4, 2}  // Bottom, 6->0->4 & 4->0->2
};

static const uint32_t specular_side_colors[6] __attribute__((aligned(32))) = {
    // format: 0xAARRGGBB
    0x7FF00000, // Red
    0x7F007F00, // Green
    0x7F00007F, // Blue
    0x7F7F7F00, // Yellow
    0x7F7F007F, // Cyan
    0x7F007F7F  // Magenta
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
  float _filler; // Pad to 32 bytes
} cube_state __attribute__((aligned(32))) = {0};

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

static matrix_t _projection_view __attribute__((aligned(32))) = {0};
void update_projection_view() {
  mat_identity();
  float radians = fovy * F_PI / 180.0f;
  float cot_fovy_2 = 1.0f / ftan(radians * 0.5f);
  mat_perspective(XSCALE * 320.0f, 240.0f, cot_fovy_2, -10.f, +10.0f);

  point_t eye = {0.f, -0.00001f, 20.0f};
  point_t center = {0.f, 0.f, 0.f};
  vector_t up = {0.f, 0.f, 1.f};
  mat_lookat(&eye, &center, &up);
  mat_store(&_projection_view);
}

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

void render_wire_grid(vec3f_t *min, vec3f_t *max, vec3f_t *dir1,
                      vec3f_t *dir2, int num_lines, uint32_t color,
                      pvr_dr_state_t *dr_state) {
  vec3f_t step = {(max->x - min->x) / (num_lines + 1),
                  (max->y - min->y) / (num_lines + 1),
                  (max->z - min->z) / (num_lines + 1)};

  if (color != 0) {
    pvr_sprite_cxt_t cxt;
    pvr_sprite_cxt_col(&cxt, PVR_LIST_OP_POLY);
    cxt.gen.culling = PVR_CULLING_NONE | PVR_CULLING_SMALL; // disable culling for polygons facing
                                        // away from the camera
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

    to_v->x =
        dir1->x == 0.0f ? max->x : min->x + i * step.x * dir1->x;
    to_v->y =
        dir1->y == 0.0f ? max->y : min->y + i * step.y * dir1->y;
    to_v->z =
        dir1->z == 0.0f ? max->z : min->z + i * step.z * dir1->z;

    from_h->x = min->x + i * step.x * dir2->x;
    from_h->y = min->y + i * step.y * dir2->y;
    from_h->z = min->z + i * step.z * dir2->z;
    to_h->x =
        dir2->x == 0.0f ? max->x : min->x + i * step.x * dir2->x;
    to_h->y =
        dir2->y == 0.0f ? max->y : min->y + i * step.y * dir2->y;
    to_h->z =
        dir2->z == 0.0f ? max->z : min->z + i * step.z * dir2->z;

    mat_transform(twolines, twolines, 4, sizeof(vec3f_t));
    draw_line(from_v, to_v, 0, dr_state);
    draw_line(from_h, to_h, 0, dr_state);
  }
  draw_line(min, max, 0, dr_state);
}

void render_wire_cube(void) {
  mat_load(&_projection_view);
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
  cxt.gen.culling =
      PVR_CULLING_NONE | PVR_CULLING_SMALL; // disable culling for polygons
                                            // facing away from the camera

  pvr_dr_init(&dr_state);
  pvr_sprite_hdr_t hdr;
  pvr_sprite_compile(&hdr, &cxt);

  for (int i = 0; i < 6; i++) {
    pvr_sprite_hdr_t *hdrpntr = (pvr_sprite_hdr_t *)pvr_dr_target(dr_state);
    hdr.argb = specular_side_colors[i];
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

#define WIREFRAME_GRID_SIZE 16

  render_wire_grid(cube_vertices + 0, cube_vertices + 3, &wiredir1, &wiredir2,
                   WIREFRAME_GRID_SIZE, specular_side_colors[0], &dr_state);

  render_wire_grid(cube_vertices + 4, cube_vertices + 7, &wiredir1, &wiredir2,
                   WIREFRAME_GRID_SIZE, specular_side_colors[1], &dr_state);

  // wiredir1.x = 0;
  // wiredir2.y = 0;
  wiredir2.y = 0;
  wiredir2.z = 1;
  render_wire_grid(cube_vertices + 0, cube_vertices + 4, &wiredir1, &wiredir2,
                   WIREFRAME_GRID_SIZE, specular_side_colors[5], &dr_state);
  render_wire_grid(cube_vertices + 1, cube_vertices + 5, &wiredir1, &wiredir2,
                   WIREFRAME_GRID_SIZE, specular_side_colors[4], &dr_state);

  wiredir1.x = 0;
  wiredir1.z = 1;
  wiredir2.z = 0;
  wiredir2.y = 1;
  render_wire_grid(cube_vertices + 4, cube_vertices + 3, &wiredir1, &wiredir2,
                   WIREFRAME_GRID_SIZE, specular_side_colors[3], &dr_state);

  render_wire_grid(cube_vertices + 6, cube_vertices + 1, &wiredir1, &wiredir2,
                   WIREFRAME_GRID_SIZE, specular_side_colors[2], &dr_state);
  // render_wire_grid(cube_vertices + 6, cube_vertices + 1, &wiredir1,
  // &wiredir2, 10,
  //                  specular_side_colors[0], &dr_state);

  pvr_dr_finish();
}

void render_txr_cube(void) {
  mat_load(&_projection_view);
  mat_translate(cube_state.pos.x, cube_state.pos.y, cube_state.pos.z);
  mat_scale(MODEL_SCALE * XSCALE, MODEL_SCALE, MODEL_SCALE);
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
    hdrpntr->oargb = specular_side_colors[i];
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
    cube_state.speed.y += 0.001f;
  if (state->buttons & CONT_B)
    cube_state.speed.y -= 0.001f;
  if (state->buttons & CONT_A)
    cube_state.speed.x += 0.001f;
  if (state->buttons & CONT_Y)
    cube_state.speed.x -= 0.001f;

  if (state->buttons & CONT_DPAD_LEFT) {
    cube_state = (struct cube){0};
    fovy = DEFAULT_FOV;
    cube_startpos();
  }
  if (state->buttons & CONT_DPAD_RIGHT) {
    if (buttons.DPAD_RIGHT == 0) {
      buttons.DPAD_RIGHT = 1;
      if (render_mode == TEXTURED)
        render_mode = WIREFRAME;
      else
        render_mode = TEXTURED;
    }
  } else {
    buttons.DPAD_RIGHT = 0;
  }

  if (state->buttons & CONT_DPAD_DOWN) {
    fovy -= 1.0f;
    update_projection_view();
  }
  if (state->buttons & CONT_DPAD_UP) {
    fovy += 1.0f;
    update_projection_view();
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
      512 * 1024,

      0,             // No DMA
      SUPERSAMPLING, //  Set horisontal FSAA
      0,             // Translucent Autosort enabled.
      1              // Extra OPBs
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

#ifdef DEBUG
    vid_border_color(255, 0, 0);
#endif
    pvr_wait_ready();

#ifdef DEBUG
    vid_border_color(0, 255, 0);
#endif
    pvr_scene_begin();

    switch (render_mode) {
    case TEXTURED:
      pvr_list_begin(PVR_LIST_TR_POLY);
      render_txr_cube();
      pvr_list_finish();
      break;
    case WIREFRAME:
      pvr_list_begin(PVR_LIST_OP_POLY);
      render_wire_cube();
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
  pvr_shutdown(); // Clean up PVR resources
  vid_shutdown(); // This function reinitializes the video system to what dcload
                  // and friends expect it to be Run the main application here;

  printf("Exiting main\n");
  return 0;
}
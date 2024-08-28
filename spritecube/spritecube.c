/** Various examples of sprites based rendering of cubes and wireframes on the 
 * Dreamcast using KallistiOS. By Daniel Fairchild, aka dRxL, @dfchil, daniel@fairchild.dk */

#include <dc/fmath.h> /* Fast math library headers for optimized mathematical functions */
#include <dc/matrix.h> /* Matrix library headers for handling matrix operations */
#include <dc/matrix3d.h> /* Matrix3D library headers for handling 3D matrix operations */
#include <dc/pvr.h> /* PVR library headers for PowerVR graphics chip functions */
#include <kos.h> /* Includes necessary KallistiOS (KOS) headers for Dreamcast development */
#include <png/png.h> /* PNG library headers for handling PNG images */
#include <stdio.h> /* Standard I/O library headers for input and output functions */
#include <stdlib.h> /* Standard library headers for general-purpose functions, including abs() */

#define DEBUG
#ifdef DEBUG
#include <arch/gdb.h>
#endif

#define SUPERSAMPLING 1 // Set to 1 to enable horizontal FSAA, 0 to disable
#if SUPERSAMPLING == 1
#define XSCALE 2.0f
#else
#define XSCALE 1.0f
#endif
#define FRAMETIMES
#include "../cube.h"        /* Cube vertices and side strips layout */
#include "../perspective.h" /* Perspective projection matrix functions */
#include "../pvrtex.h"      /* texture management, single header code */
#define DEFAULT_FOV 75.0f // Field of view, adjust with dpad up/down
#define ZOOM_SPEED 0.3f
#define MODEL_SCALE 3.0f
#define MIN_ZOOM -10.0f
#define MAX_ZOOM 15.0f
#define LINE_WIDTH 1.0f
#define WIREFRAME_MIN_GRID_LINES 0 
#define WIREFRAME_MAX_GRID_LINES 10
#define WIREFRAME_GRID_LINES_STEP 5
typedef enum {
  TEXTURED_TR,      // Textured transparent cube
  CUBES_CUBE_MIN,   // Cube of cubes, 7x7x7 cubes, in 6 different color variations
  CUBES_CUBE_MAX,   // Cube of cubes, 15x15x15 cubes, no color variations
  WIREFRAME_EMPTY,  // Wireframe cube, colored wires on the sides only
  WIREFRAME_FILLED, // Wireframe cube, as above, but with a white grid inside
  MAX_RENDERMODE    // Not a render mode, sentinel value for end of enum
} render_mode_e;

static render_mode_e render_mode = TEXTURED_TR;
static float fovy = DEFAULT_FOV;
static dttex_info_t texture256;
static dttex_info_t texture64;
static dttex_info_t texture32;

static inline void set_cube_transform(){
  mat_load(&stored_projection_view);
  mat_translate(cube_state.pos.x, cube_state.pos.y, cube_state.pos.z);
  mat_scale(MODEL_SCALE * XSCALE, MODEL_SCALE, MODEL_SCALE);
  mat_rotate_x(cube_state.rot.x);
  mat_rotate_y(cube_state.rot.y);
}

static inline void draw_textured_sprite(vec3f_t *tverts, uint32_t side, pvr_dr_state_t *dr_state){
    vec3f_t *ac = tverts + cube_side_strips[side][0];
    vec3f_t *bc = tverts + cube_side_strips[side][2];
    vec3f_t *cc = tverts + cube_side_strips[side][3];
    vec3f_t *dc = tverts + cube_side_strips[side][1];
    pvr_sprite_txr_t *quad = (pvr_sprite_txr_t *)pvr_dr_target(*dr_state);
    quad->flags = PVR_CMD_VERTEX_EOL;
    quad->ax = ac->x;
    quad->ay = ac->y;
    quad->az = ac->z;
    quad->bx = bc->x;
    quad->by = bc->y;
    quad->bz = bc->z;
    quad->cx = cc->x;
    pvr_dr_commit(quad);
    quad = (pvr_sprite_txr_t *)pvr_dr_target(*dr_state);
/* make a pointer with 32 bytes negative offset to allow field access to the second half of the quad */
    pvr_sprite_txr_t *quad2ndhalf = (pvr_sprite_txr_t *)((int)quad - 32);
    quad2ndhalf->cy = cc->y;
    quad2ndhalf->cz = cc->z;
    quad2ndhalf->dx = dc->x;
    quad2ndhalf->dy = dc->y;
    quad2ndhalf->auv = PVR_PACK_16BIT_UV(cube_tex_coords[0][0], cube_tex_coords[0][1]);
    quad2ndhalf->cuv = PVR_PACK_16BIT_UV(cube_tex_coords[3][0], cube_tex_coords[3][1]);
    quad2ndhalf->buv = PVR_PACK_16BIT_UV(cube_tex_coords[2][0], cube_tex_coords[2][1]);
    pvr_dr_commit(quad);
}

void render_txr_tr_cube(void) {
  set_cube_transform();
  vec3f_t tverts[8] __attribute__((aligned(32))) = {0};
  mat_transform((vector_t *)&cube_vertices, (vector_t *)&tverts, 8,
                sizeof(vec3f_t));
  pvr_dr_state_t dr_state;
  pvr_sprite_cxt_t cxt;
  pvr_sprite_cxt_txr(&cxt, PVR_LIST_TR_POLY, texture256.pvrformat,
                     texture256.width, texture256.height, texture256.ptr,
                     PVR_FILTER_BILINEAR);
  cxt.gen.specular = PVR_SPECULAR_ENABLE;
  cxt.gen.culling = PVR_CULLING_NONE;
  pvr_dr_init(&dr_state);
  pvr_sprite_hdr_t hdr;
  pvr_sprite_compile(&hdr, &cxt);
  hdr.argb = 0x7FFFFFFF;
  for (int i = 0; i < 6; i++) {
    pvr_sprite_hdr_t *hdrpntr = (pvr_sprite_hdr_t *)pvr_dr_target(dr_state);
    *hdrpntr = hdr;
    hdrpntr->oargb = cube_side_colors[i];
    pvr_dr_commit(hdrpntr);
    draw_textured_sprite(tverts, i, &dr_state);
  }
  pvr_dr_finish();
}

void render_cubes_cube() {
  set_cube_transform();
  pvr_sprite_cxt_t cxt;
  pvr_sprite_cxt_col(&cxt, PVR_LIST_OP_POLY);
  uint32_t cuberoot_cubes = 8;
  if (render_mode == CUBES_CUBE_MAX) {
    cuberoot_cubes = 17 - SUPERSAMPLING *2.0f;
    // 15x15x15 cubes, 6 faces per cube, 2 triangles per face @60 fps == 2430000 triangles pr. second
    // 17*17*16 cubes, or 3329280 triangles pr. second, works with FSAA disabled, set #define SUPERSAMPLING 0
    pvr_sprite_cxt_txr(
        &cxt, PVR_LIST_OP_POLY, texture32.pvrformat | PVR_TXRFMT_4BPP_PAL(16),
        texture32.width, texture32.height, texture32.ptr, PVR_FILTER_BILINEAR);
  } else {
    pvr_sprite_cxt_txr(&cxt, PVR_LIST_OP_POLY,texture64.pvrformat | PVR_TXRFMT_8BPP_PAL(0),
                       texture64.width, texture64.height, texture64.ptr,
                       PVR_FILTER_BILINEAR);
    cxt.gen.specular = PVR_SPECULAR_ENABLE;
  }
  pvr_dr_state_t dr_state;
  pvr_dr_init(&dr_state);
  pvr_sprite_hdr_t hdr;
  pvr_sprite_compile(&hdr, &cxt);
  hdr.argb = 0xFFFFFFFF;
  if (render_mode == CUBES_CUBE_MAX) { // use single shared header for MAX mode without specular
    pvr_sprite_hdr_t *hdrptr = (pvr_sprite_hdr_t *)pvr_dr_target(dr_state);
    *hdrptr = hdr;
    pvr_dr_commit(hdrptr);
  }
  vec3f_t cube_min = cube_vertices[6];
  vec3f_t cube_max = cube_vertices[3];
  vec3f_t cube_step = {
      (cube_max.x - cube_min.x) / cuberoot_cubes,
      (cube_max.y - cube_min.y) / cuberoot_cubes,
      (cube_max.z - cube_min.z) / cuberoot_cubes,
  };
  vec3f_t cube_size = {
      cube_step.x * 0.75f,
      cube_step.y * 0.75f,
      cube_step.z * 0.75f,
  };
  int xiterations = cuberoot_cubes - (SUPERSAMPLING == 0 && render_mode == CUBES_CUBE_MAX ? 1:0);
  for (int cx = 0; cx < xiterations; cx++) {
    for (int cy = 0; cy < cuberoot_cubes; cy++) {
      for (int cz = 0; cz < cuberoot_cubes; cz++) {
        if (render_mode == CUBES_CUBE_MIN) {
          pvr_sprite_hdr_t *hdrpntr = (pvr_sprite_hdr_t *)pvr_dr_target(dr_state);
          *hdrpntr = hdr;
          hdrpntr->oargb = cube_side_colors[(cx + cy + cz) % 6];
          pvr_dr_commit(hdrpntr);
        }
        vec3f_t cube_pos = {cube_min.x + cube_step.x * (float)cx,
                            cube_min.y + cube_step.y * (float)cy,
                            cube_min.z + cube_step.z * (float)cz};
        vec3f_t tverts[8] __attribute__((aligned(32))) = {
            {.x = cube_pos.x, .y = cube_pos.y, .z = cube_pos.z + cube_size.z},
            {.x = cube_pos.x, .y = cube_pos.y + cube_size.y, .z = cube_pos.z + cube_size.z},
            {.x = cube_pos.x + cube_size.x, .y = cube_pos.y, .z = cube_pos.z + cube_size.z},
            {.x = cube_pos.x + cube_size.x, .y = cube_pos.y + cube_size.y, .z = cube_pos.z + cube_size.z},
            {.x = cube_pos.x + cube_size.x, .y = cube_pos.y, .z = cube_pos.z},
            {.x = cube_pos.x + cube_size.x, .y = cube_pos.y + cube_size.y, .z = cube_pos.z},
            {.x = cube_pos.x, .y = cube_pos.y, .z = cube_pos.z},
            {.x = cube_pos.x, .y = cube_pos.y + cube_size.y, .z = cube_pos.z}};
        mat_transform((vector_t *)&tverts, (vector_t *)&tverts, 8,
                      sizeof(vec3f_t));
        for (int i = 0; i < 6; i++) {
          draw_textured_sprite(tverts, i, &dr_state);
        }
      };
    }
  }
  pvr_dr_finish();
}

static inline void draw_sprite_line(vec3f_t *from, vec3f_t *to, float centerz, pvr_dr_state_t *dr_state) {
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
    cxt.gen.culling = PVR_CULLING_NONE;
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
    mat_transform((vector_t *)twolines, (vector_t *)twolines, 4, sizeof(vec3f_t));
    draw_sprite_line(from_v, to_v, 0, dr_state);
    draw_sprite_line(from_h, to_h, 0, dr_state);
  }
  draw_sprite_line(min, max, 0, dr_state);
}

void render_wire_cube(void) {
  set_cube_transform();
  vec3f_t tverts[8] __attribute__((aligned(32))) = {0};
  mat_transform((vector_t *)&cube_vertices, (vector_t *)&tverts, 8, sizeof(vec3f_t));
  pvr_dr_state_t dr_state;
  pvr_sprite_cxt_t cxt;
  pvr_sprite_cxt_col(&cxt, PVR_LIST_OP_POLY);
  cxt.gen.culling = PVR_CULLING_NONE;
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
    draw_sprite_line(ac, dc, centerz, &dr_state);
    draw_sprite_line(bc, cc, centerz, &dr_state);
    draw_sprite_line(dc, cc, centerz, &dr_state);
    draw_sprite_line(ac, bc, centerz, &dr_state);
  }
  vec3f_t wiredir1 = (vec3f_t){1, 0, 0};
  vec3f_t wiredir2 = (vec3f_t){0, 1, 0};
  render_wire_grid(cube_vertices + 0, cube_vertices + 3, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[0], &dr_state);
  if (render_mode == WIREFRAME_FILLED) {
    for (int i = 1; i < cube_state.grid_size + 1; i++) {
      vec3f_t inner_from = *(cube_vertices + 0);
      vec3f_t inner_to = *(cube_vertices + 3);
      float z_offset = i * ((inner_from.x - inner_to.x) / (cube_state.grid_size + 1));
      inner_from.z += z_offset;
      inner_to.z += z_offset;
      render_wire_grid(&inner_from, &inner_to, &wiredir1, &wiredir2,
                       cube_state.grid_size, 0x55FFFFFF, &dr_state);
    }
  }
  render_wire_grid(cube_vertices + 4, cube_vertices + 7, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[1], &dr_state);
  wiredir2.y = 0; wiredir2.z = 1;
  render_wire_grid(cube_vertices + 0, cube_vertices + 4, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[5], &dr_state);
  if (render_mode == WIREFRAME_FILLED) {
    for (int i = 1; i < cube_state.grid_size + 1; i++) {
      vec3f_t inner_from = *(cube_vertices + 0);
      vec3f_t inner_to = *(cube_vertices + 4);
      float y_offset = i * ((inner_to.x - inner_from.x) / (cube_state.grid_size + 1));
      inner_from.y += y_offset;
      inner_to.y += y_offset;
      render_wire_grid(&inner_from, &inner_to, &wiredir1, &wiredir2,
                       cube_state.grid_size, 0x55FFFFFF, &dr_state);
    }
  }
  render_wire_grid(cube_vertices + 1, cube_vertices + 5, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[4], &dr_state);
  wiredir1.x = 0; wiredir1.z = 1;
  wiredir2.z = 0; wiredir2.y = 1;
  render_wire_grid(cube_vertices + 4, cube_vertices + 3, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[3], &dr_state);
  render_wire_grid(cube_vertices + 6, cube_vertices + 1, &wiredir1, &wiredir2,
                   cube_state.grid_size, cube_side_colors[2], &dr_state);
  pvr_dr_finish();
}

static inline void cube_reset_state() {
  uint32_t grid_size = cube_state.grid_size;
  cube_state = (struct cube){0};
  cube_state.grid_size = grid_size;
  fovy = DEFAULT_FOV;
  cube_state.pos.z = 12.0f;
  cube_state.rot.x = 1.25f * F_PI;
  cube_state.rot.y = 1.75f *F_PI;
  update_projection_view(fovy);
}

static uint32_t dpad_right_down = 0;
static inline int update_state() {
  MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, state)
  if (state->buttons & CONT_START) {
    return 0;
  }
  if (state->buttons & CONT_DPAD_RIGHT) {
    if (dpad_right_down == 0) {
      dpad_right_down = 1;
      switch (render_mode) {
      case TEXTURED_TR:
      case CUBES_CUBE_MIN:
      case CUBES_CUBE_MAX:
        render_mode++;
        break;
      default:
        cube_state.grid_size += WIREFRAME_GRID_LINES_STEP;
        if (cube_state.grid_size > WIREFRAME_MAX_GRID_LINES) {
          cube_state.grid_size = WIREFRAME_MIN_GRID_LINES;
          render_mode++;
          if (render_mode >= MAX_RENDERMODE) {
            render_mode = TEXTURED_TR;
          }
        }
      }
    }
  } else {
    dpad_right_down = 0;
  }
  if (abs(state->joyx) > 16)
    cube_state.pos.x += (state->joyx / 32768.0f) * 20.5f; // Increased sensitivity
  if (abs(state->joyy) > 16)
    cube_state.pos.y += (state->joyy / 32768.0f) * 20.5f; // Increased sensitivity and inverted Y
  if (state->ltrig > 16) // Left trigger to zoom out
    cube_state.pos.z -= (state->ltrig / 255.0f) * ZOOM_SPEED;
  if (state->rtrig > 16) // Right trigger to zoom in
    cube_state.pos.z += (state->rtrig / 255.0f) * ZOOM_SPEED;
  if (cube_state.pos.z < MIN_ZOOM)
    cube_state.pos.z = MIN_ZOOM; // Farther away
  if (cube_state.pos.z > MAX_ZOOM)
    cube_state.pos.z = MAX_ZOOM; // Closer to the screen
  if (state->buttons & CONT_X)
    cube_state.speed.y += 0.001f;
  if (state->buttons & CONT_B)
    cube_state.speed.y -= 0.001f;
  if (state->buttons & CONT_A)
    cube_state.speed.x += 0.001f;
  if (state->buttons & CONT_Y)
    cube_state.speed.x -= 0.001f;
  if (state->buttons & CONT_DPAD_LEFT) {
    fovy = DEFAULT_FOV;
    cube_reset_state();
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
  cube_state.rot.x += cube_state.speed.x;
  cube_state.rot.y += cube_state.speed.y;
  cube_state.speed.x *= 0.99f;
  cube_state.speed.y *= 0.99f;
  return 1;
}
extern uint8 romdisk[];
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);
int main(int argc, char *argv[]) {
#ifdef DEBUG
  gdb_init();
#endif
  pvr_set_bg_color(0.0, 0.0, 24.0f / 255.0f);
  pvr_init_params_t params = {
      {PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0,
       PVR_BINSIZE_0},
      3 << 20,       // Vertex buffer size, 3MB
      0,             // No DMA15
      SUPERSAMPLING, // Set horisontal FSAA
      0,             // Translucent Autosort enabled.
      3              // Extra OPBs
  };
  pvr_init(&params);
  pvr_set_bg_color(0, 0, 0);
  if (!pvrtex_load("/rd/texture/rgb565_vq_tw/dc.dt", &texture256))
    return -1;
  if (!pvrtex_load("/rd/texture/pal8/dc_64sq_256colors.dt", &texture64))
    return -1;
  if (!pvrtex_load_palette("/rd/texture/pal8/dc_64sq_256colors.dt.pal", PVR_PAL_RGB565, 0))
    return -1;
  if (!pvrtex_load("/rd/texture/pal4/dc_32sq_16colors.dt", &texture32))
    return -1;
  if (!pvrtex_load_palette("/rd/texture/pal4/dc_32sq_16colors.dt.pal", PVR_PAL_RGB565, 256))
    return -1;
  cube_reset_state();
  while (update_state()) {
#ifdef FRAMETIMES
    vid_border_color(255, 0, 0);
#endif
    pvr_wait_ready();
#ifdef FRAMETIMES
    vid_border_color(0, 255, 0);
#endif
    pvr_scene_begin();
    switch (render_mode) {
    case TEXTURED_TR:
      pvr_list_begin(PVR_LIST_TR_POLY);
      render_txr_tr_cube();
      pvr_list_finish();
      break;
    case WIREFRAME_FILLED:
    case WIREFRAME_EMPTY:
      pvr_list_begin(PVR_LIST_OP_POLY);
      render_wire_cube();
      pvr_list_finish();
      break;
    case CUBES_CUBE_MAX:
    case CUBES_CUBE_MIN:
      pvr_list_begin(PVR_LIST_OP_POLY);
      render_cubes_cube();
      pvr_list_finish();
      break;
    default:
      break;
    }
#ifdef FRAMETIMES
    vid_border_color(0, 0, 255);
#endif
    pvr_scene_finish();
  }
  printf("Cleaning up\n");
  pvrtex_unload(&texture256);
  pvrtex_unload(&texture64);
  pvrtex_unload(&texture32);
  pvr_shutdown(); // Clean up PVR resources
  vid_shutdown(); // This function reinitializes the video system to what dcload
                  // and friends expect it to be Run the main application here;
  printf("Exiting main\n");
  return 0;
}
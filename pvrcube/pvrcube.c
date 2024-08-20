
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

#include <dc/fmath.h> /* Fast math library headers for optimized mathematical functions */
#include <dc/pvr.h> /* PVR library headers for PowerVR graphics chip functions */
#include <kos.h> /* Includes necessary KallistiOS (KOS) headers for Dreamcast development */
#include <png/png.h> /* PNG library headers for handling PNG images */
#include <stdio.h> /* Standard I/O library headers for input and output functions */
#include <stdlib.h> /* Standard library headers for general-purpose functions, including abs() */

#include <dc/matrix.h> /* Matrix library headers for handling matrix operations */
#include <dc/matrix3d.h> /* Matrix3D library headers for handling 3D matrix operations */

#include "../cube.h" /* Cube vertices and side strips layout */
#include "../pvrtex.h" /* texture management, single header code */
#include "../perspective.h" /* Perspective projection matrix functions */

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define MODEL_SCALE 3.0f
#define DEFAULT_FOV 45.0f
#define MIN_ZOOM -10.0f
#define MAX_ZOOM 15.0f
#define ZOOM_SPEED 0.3f

extern uint8 romdisk[];
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

static float fovy = DEFAULT_FOV;

static dttex_info_t texture;

static inline void init_poly_context(pvr_poly_cxt_t *cxt) {
  pvr_poly_cxt_txr(cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444, texture.bytewidth,
                   texture.byteheight, texture.ptr, PVR_FILTER_BILINEAR);
  cxt->gen.culling = PVR_CULLING_NONE; // disable culling for polygons facing
                                       // away from the camera
  cxt->gen.specular = PVR_SPECULAR_ENABLE;
}

void render_cube(void) {
  mat_load(&_projection_view);
  mat_translate(cube_state.pos.x, cube_state.pos.y, cube_state.pos.z);
  mat_scale(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE);
  mat_rotate_x(cube_state.rot.x);
  mat_rotate_y(cube_state.rot.y);

  vec3f_t tverts[8] __attribute__((aligned(32))) = {0};
  mat_transform((vector_t*)&cube_vertices, (vector_t*)&tverts, 8, sizeof(vec3f_t));

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
      vec3f_t *vp = tverts + cube_side_strips[i][j];
      vert = pvr_dr_target(dr_state);
      vert->flags = (j == 3) ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
      vert->x = vp->x;
      vert->y = vp->y;
      vert->z = vp->z;
      vert->u = cube_tex_coords[j][0];
      vert->v = cube_tex_coords[j][1];
      vert->argb = 0xCFFFFFFF;
      // The oargb specular color does the following:
      // resulting color = texsample * argb + oargb.
      // So white texture samples will remain white and black texture samples
      // will be the same as oargb
      vert->oargb = cube_side_colors[i];
      pvr_dr_commit(vert);
    }
  }
  pvr_dr_finish();
}

static inline void cube_startpos() {
  cube_state = (struct cube){0};
  fovy = DEFAULT_FOV;
  cube_state.pos.z = (MAX_ZOOM + MIN_ZOOM) / 2.0f;
  cube_state.rot.x = 0.5;
  cube_state.rot.y = 0.5;
  update_projection_view(fovy);
}


int update_state() {
  int keep_running = 1;
  MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, state)
  if (state->buttons & CONT_START){
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
    update_projection_view(fovy);
  }
  if (state->buttons & CONT_DPAD_UP) {
    fovy += 1.0f;
    update_projection_view(fovy);
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
  pvr_init_params_t params = {
      {PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0,
       PVR_BINSIZE_0},
      512 * 1024, // Vertex buffer size
      0, // No DMA
      0, //  No FSAA
      0  // Translucent Autosort enabled.
  };

  pvr_init(&params);
  pvr_set_bg_color(0, 0, 0);

  if (!load_texture("/rd/texture/rgb565_vq_tw/dc.dt", &texture)) {
    printf("Failed to load texture.\n");
    return -1;
  }
  cube_startpos();

  while (1) {
    if (!update_state())
      break;

    pvr_wait_ready();
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_TR_POLY);

    render_cube();

    pvr_list_finish();
    pvr_scene_finish();
  }

  printf("Cleaning up\n");
  unload_texture(&texture);
  pvr_shutdown(); // Clean up PVR resources
  vid_shutdown(); // This function reinitializes the video system to what dcload
                  // and friends expect it to be Run the main application here;

  printf("Exiting main\n");
  return 0;
}
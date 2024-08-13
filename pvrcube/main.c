
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

#define DEFAULT_FOV 45.0f
#define ZOOM_SPEED 0.3f
#define CUBEEXTENT 1.0f
#define MODEL_SCALE 3.0f
#define MIN_ZOOM -10.0f
#define MAX_ZOOM 15.0f

typedef struct {
  pvr_ptr_t ptr;
  int w, h;
  uint32 fmt;
} kos_texture_t;

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

static kos_texture_t *texture;

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

static inline void init_poly_context(pvr_poly_cxt_t *cxt) {
  //   printf("Entering init_poly_context\n");
  pvr_poly_cxt_txr(cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444, texture->w,
                   texture->h, texture->ptr, PVR_FILTER_BILINEAR);
  cxt->gen.culling = PVR_CULLING_NONE; // disable culling for polygons facing
                                       // away from the camera
  cxt->gen.specular = PVR_SPECULAR_ENABLE;
}

static matrix_t _projection_view __attribute__((aligned(32))) = {0};
void update_projection_view() {
  mat_identity();
  float radians = fovy * F_PI / 180.0f;
  float cot_fovy_2 = 1.0f / ftan(radians * 0.5f);
  mat_perspective(320.0f, 240.0f, cot_fovy_2, -10.f, +10.0f);

  vec3f_t eye = {0.f, -0.00001f, 20.0f};
  vec3f_t center = {0.f, 0.f, 0.f};
  vec3f_t up = {0.f, 0.f, 1.f};
  mat_lookat(&eye, &center, &up);
  mat_store(&_projection_view);
}

void render_cube(void) {
  mat_load(&_projection_view);
  mat_translate(cube_state.pos.x, cube_state.pos.y, cube_state.pos.z);
  mat_scale(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE);
  mat_rotate_x(cube_state.rot.x);
  mat_rotate_y(cube_state.rot.y);

  vec3f_t tverts[8] __attribute__((aligned(32))) = {0};
  mat_transform(&cube_vertices, &tverts, 8, sizeof(vec3f_t));

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
      vert->u = tex_coords[j][0];
      vert->v = tex_coords[j][1];
      vert->argb =
          0xCFFFFFFF; // slightly transparent and preserve texture color, if any
      // specular color does the following: resulting color = texsample * argb +
      // oargb So white texture samples will remain white and black texture
      // samples will be the same as oargb
      vert->oargb = specular_side_colors[i];
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
  update_projection_view();
}

int update_state() {
  int no_exit = 1;
  MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, state)
  if (state->buttons & CONT_START)
    no_exit = 0;

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

  return no_exit;
}

int main(int argc, char *argv[]) {
  gdb_init();
  pvr_init_params_t params = {
      {PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0,
       PVR_BINSIZE_0},
      512 * 1024,

      0, // No DMA
      0, //  No FSAA
      0  // Translucent Autosort enabled.
  };

  pvr_init(&params);
  pvr_set_bg_color(0, 0, 0);

  texture = load_png_texture("/rd/dc.png");
  if (!texture) {
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
  pvr_shutdown(); // Clean up PVR resources
  vid_shutdown(); // This function reinitializes the video system to what dcload
                  // and friends expect it to be Run the main application here;

  printf("Exiting main\n");
  return 0;
}
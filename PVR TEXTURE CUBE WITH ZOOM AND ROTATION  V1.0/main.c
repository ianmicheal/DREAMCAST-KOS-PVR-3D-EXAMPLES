
/********************************************************************************************/
/* KallistiOS Direct PVR API:2.0  PVR TEXTURE CUBE WITH ZOOM AND ROTATION  V1               */
/********************************************************************************************/
/********************************************************************************************/
/* Name:     pngzoom .c                                                                     */
/* Title:    PVR TEXTURE CUBE WITH ZOOM AND ROTATION Kos Example                            */
/* Author:   (c)Ian Micheal                                                                 */
/* Created:   05/08/24                                                                      */
/*                                                                                          */
/* Version:  1.0                                                                            */
/* Platform: Dreamcast | KallistiOS:2.0 | KOSPVR |                                          */
/*                                                                                          */
/* Description:                                                                             */
/* The purpose of this example is to show the use of only the KOSPVR API to do 3D matching  */
/* And commented so anyone that knows opengl can use DIRECT NO LAYER KOSPVR API             */
/* History: version 1                                                                       */
/********************************************************************************************/
/********************************************************************************************/
/*        >>>  Help and code examples and advice these people where invaluable  <<<         */
/*     Mvp's:  dRxL with my_perspective_mat_lh and explaining to me the concepts            */
/*     Mvp's:  Bruce tested and found both annoying bugs and texture distortion.            */
/*                                                                                          */
/********************************************************************************************/ 



#include <kos.h>        /* Includes necessary KallistiOS (KOS) headers for Dreamcast development */
#include <dc/pvr.h>     /* PVR library headers for PowerVR graphics chip functions */
#include <math.h>       /* Standard slower math library headers for mathematical functions */
#include <dc/fmath.h>   /* Fast math library headers for optimized mathematical functions */
#include <dc/matrix.h>  /* Matrix library headers for handling matrix operations */
#include <png/png.h>    /* PNG library headers for handling PNG images */
#include <stdio.h>      /* Standard I/O library headers for input and output functions */
#include <stdlib.h>     /* Standard library headers for general-purpose functions, including abs() */  



extern uint8 romdisk[];
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

typedef struct {
    pvr_ptr_t ptr;
    int w, h;
    uint32 fmt;
} kos_texture_t;

kos_texture_t* texture = NULL;

float cube_x = 0.0f, cube_y = 0.0f, cube_z = -0.0f;
float xrot = 0.0f, yrot = 0.0f, xspeed = 0.0f, yspeed = 0.0f;

// Ensure perspective_mat is aligned to a 32-byte boundary
static matrix_t perspective_mat __attribute__((aligned(32)));

void my_perspective_mat_lh(float fov, float aspect, float zNear, float zFar) {
 //   printf("Entering my_perspective_mat_lh\n");
    float f = 1.0f / ftan(fov * 0.5f * F_PI / 180.0f);
  
    // Use a local array to build the matrix, ensuring all accesses are aligned
    float local_mat[4][4] __attribute__((aligned(32))) = {
        {f / aspect, 0.0f, 0.0f, 0.0f},
        {0.0f, f, 0.0f, 0.0f},
        {0.0f, 0.0f, (zFar + zNear) / (zNear - zFar), (2.0f * zFar * zNear) / (zNear - zFar)},
        {0.0f, 0.0f, -1.0f, 0.0f}
    };

    // Copy the local matrix to the global perspective_mat
    memcpy(perspective_mat, local_mat, sizeof(matrix_t));

    mat_load(&perspective_mat);
  //  printf("Exiting my_perspective_mat_lh\n");
}

static inline float min_float(float a, float b) {
    return a < b ? a : b;
}

static inline float max_float(float a, float b) {
    return a > b ? a : b;
}


kos_texture_t* load_png_texture(const char *filename) {
 //   printf("Entering load_png_texture\n");
    kos_texture_t* texture;
    kos_img_t img;

    if (png_to_img(filename, PNG_FULL_ALPHA, &img) < 0) {
    //    printf("Failed to load PNG image\n");
        return NULL;
    }

    texture = (kos_texture_t*)malloc(sizeof(kos_texture_t));
    if (!texture) {
     //   printf("Failed to allocate texture structure\n");
        kos_img_free(&img, 0);
        return NULL;
    }

    texture->ptr = pvr_mem_malloc(img.byte_count);
    if (!texture->ptr) {
     //   printf("Failed to allocate PVR memory for texture\n");
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
    pvr_poly_cxt_txr(cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_ARGB4444, texture->w, texture->h, texture->ptr, PVR_FILTER_BILINEAR);
    cxt->gen.culling = PVR_CULLING_CCW;
 //   printf("Exiting init_poly_context\n");
}

void render_cube(void) {
 //   printf("Entering render_cube\n");
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t *vert;
    pvr_dr_state_t dr_state;
    float scale = 1.0f;
    matrix_t model_view_matrix __attribute__((aligned(32)));

  //  printf("Initializing poly context\n");
    init_poly_context(&cxt);
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));

  //  printf("Setting up projection matrix\n");
    mat_identity();
    my_perspective_mat_lh(45.0f, 640.0f / 480.0f, 0.1f, 100.0f);

  //  printf("Setting up view matrix\n");
    point_t eye = {0, 0, 0};
    point_t center = {0, 0, -1};
    vector_t up = {0, 1, 0};
    mat_lookat(&eye, &center, &up);

    // Calculate scale based on cube_z
    float zoom_scale = 100.0f / (-cube_z);

  //  printf("Setting up model view matrix\n");
    mat_identity();
    mat_translate(cube_x, cube_y, cube_z);
    mat_rotate_x(xrot);
    mat_rotate_y(yrot);
    mat_scale(scale * zoom_scale, scale * zoom_scale, scale * zoom_scale);
    mat_store(&model_view_matrix);

    float vertices[24][3] = {
        {-scale, -scale, scale},   // Bottom-left
        {scale, -scale, scale},    // Bottom-right
        {-scale, scale, scale},    // Top-left
        {scale, scale, scale},     // Top-right

        {scale, -scale, scale},    // Bottom-front
        {scale, -scale, -scale},   // Bottom-back
        {scale, scale, scale},     // Top-front
        {scale, scale, -scale},    // Top-back

        {scale, -scale, -scale},   // Bottom-right
        {-scale, -scale, -scale},  // Bottom-left
        {scale, scale, -scale},    // Top-right
        {-scale, scale, -scale},   // Top-left

        {-scale, -scale, -scale},  // Bottom-back
        {-scale, -scale, scale},   // Bottom-front
        {-scale, scale, -scale},   // Top-back
        {-scale, scale, scale},    // Top-front

        {-scale, scale, scale},    // Front-left
        {scale, scale, scale},     // Front-right
        {-scale, scale, -scale},   // Back-left
        {scale, scale, -scale},    // Back-right

        {-scale, -scale, -scale},  // Back-left
        {scale, -scale, -scale},   // Back-right
        {-scale, -scale, scale},   // Front-left
        {scale, -scale, scale}     // Front-right
    };

    float tex_coords[4][2] = {{0, 1}, {1, 1}, {0, 0}, {1, 0}};

    uint32 colors[6] = {
        PVR_PACK_COLOR(1.0f, 1.0f, 0.0f, 0.0f),  // Red
        PVR_PACK_COLOR(1.0f, 0.0f, 1.0f, 0.0f),  // Green
        PVR_PACK_COLOR(1.0f, 0.0f, 0.0f, 1.0f),  // Blue
        PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 0.0f),  // Yellow
        PVR_PACK_COLOR(1.0f, 1.0f, 0.0f, 1.0f),  // Cyan
        PVR_PACK_COLOR(1.0f, 0.0f, 1.0f, 1.0f)   // Magenta
    };

 //   printf("Initializing DR state\n");
    pvr_dr_init(dr_state);

  //  printf("Rendering cube faces\n");
    // Render all six quads
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 4; j++) {
            int idx = i * 4 + j;
            float x = vertices[idx][0];
            float y = vertices[idx][1];
            float z = vertices[idx][2];

            vec3f_t v = {x, y, z};
            mat_trans_single(v.x, v.y, v.z);

            vert = pvr_dr_target(dr_state);
            vert->flags = (j == 3) ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
            vert->x = v.x + 320.0f;
            vert->y = v.y + 240.0f;
            vert->z = min_float(65535.0f, max_float(0.0f, (v.z + 10.0f) / 20.0f * 65535.0f));
            vert->u = tex_coords[j][0];
            vert->v = tex_coords[j][1];
            vert->argb = colors[i];
            vert->oargb = 0;
            pvr_dr_commit(vert);
        }
    }

  //  printf("Restoring model view matrix\n");
    mat_load(&model_view_matrix);
 //   printf("Exiting render_cube\n");
}

int main(int argc, char *argv[]) {
  //  printf("Entering main\n");
    pvr_init_params_t params = {
        { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 },
        512 * 1024
    };

  //  printf("Initializing PVR\n");
    pvr_init(&params);
    pvr_set_bg_color(0.0f, 0.0f, 0.0f);

  //  printf("Loading texture\n");
    texture = load_png_texture("/rd/crate.png");
    if (!texture) {
        printf("Failed to load texture.\n");
        return -1;
    }

  //  printf("Entering main loop\n");
    while(1) {
        pvr_wait_ready();
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_OP_POLY);
        
        render_cube();
        
        pvr_list_finish();
        pvr_scene_finish();

        // Apply rotation
        xrot += xspeed;
        yrot += yspeed;

        // Apply friction
        xspeed *= 0.99f;
        yspeed *= 0.99f;

        // If speed is very low, set it to zero to prevent unwanted rotation
        if (fabs(xspeed) < 0.0001f) xspeed = 0;
        if (fabs(yspeed) < 0.0001f) yspeed = 0;

        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, state)
            if (state->buttons & CONT_START)
                goto out;
            
       
          // Analog stick for X and Y movement
    if (abs(state->joyx) > 16) {
        cube_x += (state->joyx / 32768.0f) * 1000.5f;  // Increased sensitivity
    }
    if (abs(state->joyy) > 16) {
        cube_y -= (state->joyy / 32768.0f) * 1000.5f;  // Increased sensitivity and inverted Y
    }
    
            // Trigger handling for zooming
            float zoom_speed = 0.30f;
            if (state->ltrig > 16) {  // Left trigger to zoom out
                cube_z -= (state->ltrig / 255.0f) * zoom_speed;
            }
            if (state->rtrig > 16) {  // Right trigger to zoom in
                cube_z += (state->rtrig / 255.0f) * zoom_speed;
            }
            
            // Limit the zoom range
            if (cube_z < -10.0f) cube_z = -10.0f;  // Farther away
            if (cube_z > -0.5f) cube_z = -0.5f;    // Closer to the screen
            
            // Button controls for rotation speed
            if (state->buttons & CONT_X)
                xspeed += 0.001f;
            if (state->buttons & CONT_Y)
                xspeed -= 0.001f;
            if (state->buttons & CONT_A)
                yspeed += 0.001f;
            if (state->buttons & CONT_B)
                yspeed -= 0.001f;

            // Print debug information
         //    printf("Cube position: X=%.2f, Y=%.2f, Z=%.2f\n", (double)cube_x, (double)cube_y, (double)cube_z);
         //    printf("Analog: X=%d, Y=%d\n", state->joyx, state->joyy);
        MAPLE_FOREACH_END()
    }

out:
    printf("Cleaning up\n");
    pvr_shutdown(); // Clean up PVR resources
    vid_shutdown(); // This function reinitializes the video system to what dcload and friends expect it to be
      // Run the main application here;

    printf("Exiting main\n");
    return 0;
}

/********************************************************************************************/
/* KallistiOS Direct PVR API:2.0  PVR TEXTURE CUBE WITH 6 TEXTURES: ZOOM AND ROTATION, V2.0 */
/********************************************************************************************/
/********************************************************************************************/
/* Name:     cube6.c                                                                        */
/* Title:    PVR TEXTURE CUBE WITH 6 TEXTURES: ZOOM AND ROTATION, V2.0 Kos Example          */
/* Author:   (c)Ian Micheal                                                                 */
/* Created:  08/05/24                                                                       */
/*                                                                                          */
/* Version:  2.0                                                                            */
/* Platform: Dreamcast | KallistiOS:2.0 | KOSPVR |                                          */
/*                                                                                          */
/* Description: 6 Textures one each cube face                                               */
/* The purpose of this example is to show the use of only the KOSPVR API to do 3D           */
/* And commented so anyone who knows OpenGL can use the DIRECT NO LAYER KOSPVR API.         */
/* History: version 2                                                                       */
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

#define NUM_TEXTURES 6

typedef struct {
    pvr_ptr_t ptr;
    int w, h;
    uint32 fmt;
} kos_texture_t;

kos_texture_t* textures[NUM_TEXTURES] = {NULL};

float cube_x = 0.0f, cube_y = 0.0f, cube_z = -0.0f;
float xrot = 0.0f, yrot = 0.0f, xspeed = 0.0f, yspeed = 0.0f;
/* Author: Ian Michael
 * This needed alignment as it worked on DC emulators. They all allow non-aligned
 * memory reads, which is an instant crash on the SH4 CPU and the Dreamcast, as
 * they do not allow this. I fixed this with major printf and add2line debugging.
 */
static matrix_t perspective_mat __attribute__((aligned(32)));

void my_perspective_mat_lh(float fov, float aspect, float zNear, float zFar) {
    float f = 1.0f / ftan(fov * 0.5f * F_PI / 180.0f);
    float local_mat[4][4] __attribute__((aligned(32))) = {
        {f / aspect, 0.0f, 0.0f, 0.0f},
        {0.0f, f, 0.0f, 0.0f},
        {0.0f, 0.0f, (zFar + zNear) / (zNear - zFar), (2.0f * zFar * zNear) / (zNear - zFar)},
        {0.0f, 0.0f, -1.0f, 0.0f}
    };
    memcpy(perspective_mat, local_mat, sizeof(matrix_t));
    mat_load(&perspective_mat);
}

static inline float min_float(float a, float b) {
    return a < b ? a : b;
}

static inline float max_float(float a, float b) {
    return a > b ? a : b;
}
/*
 * Author: Ian Michael
 *
 * This method goes beyond the usual static texture loading. Typically, setting
 * a 512x512 PNG, even if it's just 10 KB, forces it to occupy a static 512 KB
 * in VRAM. Here, however, we dynamically read the image size, ensuring it only
 * uses the actual size in KB. This approach supports KMG format too, providing
 * additional savings.
 *
 * While this might seem straightforward, existing examples mostly stick to
 * static file sizes, like 512x512, leading to inefficient use of VRAM. Nearly
 * all examples, including homebrew projects, follow this inefficient practice.
 */
kos_texture_t* load_png_texture(const char *filename) {
    kos_texture_t* texture;
    kos_img_t img;

    if (png_to_img(filename, PNG_FULL_ALPHA, &img) < 0) {
        return NULL;
    }

    texture = (kos_texture_t*)malloc(sizeof(kos_texture_t));
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

    return texture;
}

void load_cube_textures() {
    const char* texture_files[NUM_TEXTURES] = {
        "/rd/face1.png",
        "/rd/face2.png",
        "/rd/face3.png",
        "/rd/face4.png",
        "/rd/face5.png",
        "/rd/face6.png"
    };

    for (int i = 0; i < NUM_TEXTURES; i++) {
        textures[i] = load_png_texture(texture_files[i]);
        if (!textures[i]) {
            printf("Failed to load texture %s.\n", texture_files[i]);
        }
    }
}

void init_poly_context(pvr_poly_cxt_t *cxt, int texture_index) {
    pvr_poly_cxt_txr(cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_ARGB4444, 
                     textures[texture_index]->w, textures[texture_index]->h, 
                     textures[texture_index]->ptr, PVR_FILTER_BILINEAR);
    cxt->gen.culling = PVR_CULLING_CCW;
}

void render_cube(void) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t *vert;
    pvr_dr_state_t dr_state;
    float scale = 1.0f;
    matrix_t model_view_matrix __attribute__((aligned(32)));

    mat_identity();
    my_perspective_mat_lh(45.0f, 640.0f / 480.0f, 0.1f, 100.0f);

    point_t eye = {0, 0, 0};
    point_t center = {0, 0, -1};
    vector_t up = {0, 1, 0};
    mat_lookat(&eye, &center, &up);

    float zoom_scale = 100.0f / (-cube_z);

    mat_identity();
    mat_translate(cube_x, cube_y, cube_z);
    mat_rotate_x(xrot);
    mat_rotate_y(yrot);
    mat_scale(scale * zoom_scale, scale * zoom_scale, scale * zoom_scale);
    mat_store(&model_view_matrix);

    float vertices[24][3] = {
        {-scale, -scale, scale},   {scale, -scale, scale},    {-scale, scale, scale},    {scale, scale, scale},
        {scale, -scale, scale},    {scale, -scale, -scale},   {scale, scale, scale},     {scale, scale, -scale},
        {scale, -scale, -scale},   {-scale, -scale, -scale},  {scale, scale, -scale},    {-scale, scale, -scale},
        {-scale, -scale, -scale},  {-scale, -scale, scale},   {-scale, scale, -scale},   {-scale, scale, scale},
        {-scale, scale, scale},    {scale, scale, scale},     {-scale, scale, -scale},   {scale, scale, -scale},
        {-scale, -scale, -scale},  {scale, -scale, -scale},   {-scale, -scale, scale},   {scale, -scale, scale}
    };

    float tex_coords[4][2] = {{0, 1}, {1, 1}, {0, 0}, {1, 0}};

    pvr_dr_init(dr_state);

    for (int i = 0; i < 6; i++) {
        init_poly_context(&cxt, i);
        pvr_poly_compile(&hdr, &cxt);
        pvr_prim(&hdr, sizeof(hdr));

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
            vert->argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
            vert->oargb = 0;
            pvr_dr_commit(vert);
        }
    }

    mat_load(&model_view_matrix);
}

void cleanup() {
    for (int i = 0; i < NUM_TEXTURES; i++) {
        if (textures[i]) {
            pvr_mem_free(textures[i]->ptr);
            free(textures[i]);
        }
    }
    pvr_shutdown();
    vid_shutdown();
}

int main(int argc, char *argv[]) {
    pvr_init_params_t params = {
        { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 },
        512 * 1024
    };

    pvr_init(&params);
    pvr_set_bg_color(0.0f, 0.0f, 0.0f);

    load_cube_textures();

    while(1) {
        pvr_wait_ready();
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_OP_POLY);
        
        render_cube();
        
        pvr_list_finish();
        pvr_scene_finish();

        xrot += xspeed;
        yrot += yspeed;

        xspeed *= 0.99f;
        yspeed *= 0.99f;

        if (fabs(xspeed) < 0.0001f) xspeed = 0;
        if (fabs(yspeed) < 0.0001f) yspeed = 0;

        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, state)
            if (state->buttons & CONT_START)
                goto out;
            
            if (abs(state->joyx) > 16) {
                cube_x += (state->joyx / 32768.0f) * 1000.5f;
            }
            if (abs(state->joyy) > 16) {
                cube_y -= (state->joyy / 32768.0f) * 1000.5f;
            }
            
            float zoom_speed = 0.30f;
            if (state->ltrig > 16) {
                cube_z -= (state->ltrig / 255.0f) * zoom_speed;
            }
            if (state->rtrig > 16) {
                cube_z += (state->rtrig / 255.0f) * zoom_speed;
            }
            
            if (cube_z < -10.0f) cube_z = -10.0f;
            if (cube_z > -0.5f) cube_z = -0.5f;
            
            if (state->buttons & CONT_X)
                xspeed += 0.001f;
            if (state->buttons & CONT_Y)
                xspeed -= 0.001f;
            if (state->buttons & CONT_A)
                yspeed += 0.001f;
            if (state->buttons & CONT_B)
                yspeed -= 0.001f;
        MAPLE_FOREACH_END()
    }

out:
    cleanup();
    return 0;
}
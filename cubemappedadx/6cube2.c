/********************************************************************************************/
/* KallistiOS Direct PVR API: 2.0 - PVR TEXTURE CUBE With Perlin Textures V2.0              */
/********************************************************************************************/
/********************************************************************************************/
/* Name:     cube6.c                                                                         */
/* Title:    PVR TEXTURE CUBE WITH 6 TEXTURES: ZOOM AND ROTATION, V2.0 KOS Example          */
/* Author:   (c) Ian Micheal                                                                 */
/* Created:  08/05/24                                                                        */
/*                                                                                          */
/* Version:  2.0                                                                             */
/* Platform: Dreamcast | KallistiOS: 2.0 | KOSPVR                                           */
/*                                                                                          */
/* Description:                                                                              */
/*   - 6 Textures, one on each cube face.                                                    */
/*   - Demonstrates the use of the KOSPVR API to perform 3D rendering without layers.        */
/*   - Commented to assist OpenGL users in understanding the DIRECT NO LAYER KOSPVR API.     */
/*                                                                                          */
/* History:                                                                                 */
/*   - Version 2                                                                             */
/********************************************************************************************/
/********************************************************************************************/
/* >>>  Special Thanks and Code Examples  <<<                                                */
/* Mvp's:                                                                                   */
/*   - dRxL: mat_perspective_fov and explanations of related concepts.                      */
/*   - Bruce: Testing, bug reporting, and texture distortion fixes.                         */
/*   - falcogirgis: OpenGL order rendering advice and code contributions.                   */
/********************************************************************************************/

#include <dc/fmath.h>  /* Fast math library for optimized mathematical functions             */
#include <dc/matrix.h> /* Matrix library for handling matrix operations                      */
#include <dc/pvr.h>    /* PVR library for PowerVR graphics chip functions                   */
#include <kos.h>       /* KallistiOS (KOS) headers for Dreamcast development                */
#include <math.h>      /* Standard math library for general mathematical functions          */
#include <png/png.h>   /* PNG library for handling PNG images                               */
#include <stdio.h>     /* Standard I/O library for input and output functions               */
#include <stdlib.h>    /* Standard library for general-purpose functions, including abs()   */
#include "perlin.h"    /* Perlin noise header                                               */

/********************************************************************************************/
/* LibADX (c) 2012 Josh PH3NOM Pearson                                                      */
/* Decoder algorithm based on adx2wav (c) BERO 2001                                         */
/*                                                                                          */
/* LibADX is a library for decoding ADX audio files using the                                */
/* KallistiOS development environment, intended for use only                                */
/* on the Sega Dreamcast game console.                                                      */
/*                                                                                          */
/* Features:                                                                                 */
/*   - Full implementation of the ADX looping function.                                     */
/*   - Functions include play, pause, stop, restart.                                        */
/*                                                                                          */
/* This library is completely free to modify and/or redistribute.                           */
/********************************************************************************************/

#include "LibADX/libadx.h" /* ADX Decoder Library */
#include "LibADX/snddrv.h" /* Direct Access to Sound Driver */

#define NUM_TEXTURES 6
#define PERLIN_TEXTURE_SIZE 16

extern uint8 romdisk[];
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

// Perlin noise parameters
typedef struct {
    float scale;
    float persistence;
    float lacunarity;
    int octaves;
    float offset_x;
    float offset_y;
    int color_mode;
    float metallic_hue;
} PerlinParams;

PerlinParams perlin_params = {
    .scale = 32.0f,
    .persistence = 0.5f,
    .lacunarity = 2.0f,
    .octaves = 4,
    .offset_x = 0.0f,
    .offset_y = 0.0f,
    .color_mode = 0,
    .metallic_hue = 0.0f
};

typedef struct {
    pvr_ptr_t ptr;
    int w, h;
    uint32 fmt;
} kos_texture_t;

kos_texture_t* textures[NUM_TEXTURES] = {NULL};
pvr_ptr_t perlin_texture = NULL;

float cube_x = 0.0f, cube_y = 0.0f, cube_z = -5.0f;
float xrot = 0.0f, yrot = 0.0f;

static matrix_t _perspective_mtrx __attribute__((aligned(32)));

void mat_perspective_fov(float fov, float aspect, float zNear, float zFar) {
    float f = 1.0f / ftan(fov * 0.5f * F_PI / 180.0f);
    float local_mat[4][4] __attribute__((aligned(32))) = {
        {f / aspect, 0.0f, 0.0f, 0.0f},
        {0.0f, f, 0.0f, 0.0f},
        {0.0f, 0.0f, (zFar + zNear) / (zNear - zFar), (2.0f * zFar * zNear) / (zNear - zFar)},
        {0.0f, 0.0f, -1.0f, 0.0f}
    };
    memcpy(_perspective_mtrx, local_mat, sizeof(matrix_t));
    mat_load(&_perspective_mtrx);
}

static inline float min_float(float a, float b) { return a < b ? a : b; }
static inline float max_float(float a, float b) { return a > b ? a : b; }

kos_texture_t* load_png_texture(const char *filename) {
    kos_texture_t* texture;
    kos_img_t img;

    if (png_to_img(filename, PNG_FULL_ALPHA, &img) < 0) {
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

    return texture;
}

void load_cube_textures() {
    const char* texture_files[NUM_TEXTURES] = {
        "/rd/face1.png", "/rd/face2.png", "/rd/face3.png",
        "/rd/face4.png", "/rd/face5.png", "/rd/face6.png"
    };
    for (int i = 0; i < NUM_TEXTURES; i++) {
        textures[i] = load_png_texture(texture_files[i]);
        if (!textures[i]) {
            printf("Failed to load texture %s.\n", texture_files[i]);
        } else {
            printf("Loaded texture %s: %dx%d, format: %d\n", 
                   texture_files[i], textures[i]->w, textures[i]->h, textures[i]->fmt);
        }
    }
}

uint16 hsv_to_rgb565(float h, float s, float v) {
    float r = 0, g = 0, b = 0;
    int i = (int)(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }

    uint16 r16 = (uint16)(r * 31);
    uint16 g16 = (uint16)(g * 63);
    uint16 b16 = (uint16)(b * 31);

    return (r16 << 11) | (g16 << 5) | b16;
}
void create_perlin_texture() {
    if (perlin_texture != NULL) {
        pvr_mem_free(perlin_texture);
    }
    
    uint16 *texture_data = (uint16 *)memalign(32, PERLIN_TEXTURE_SIZE * PERLIN_TEXTURE_SIZE * 2);
    
    for (int y = 0; y < PERLIN_TEXTURE_SIZE; y++) {
        for (int x = 0; x < PERLIN_TEXTURE_SIZE; x++) {
            float sample_x = (x + perlin_params.offset_x) / perlin_params.scale;
            float sample_y = (y + perlin_params.offset_y) / perlin_params.scale;
            
            float noise = fbm_noise_2D(sample_x, sample_y, 
                                       perlin_params.octaves, 
                                       perlin_params.lacunarity, 
                                       perlin_params.persistence);
            
            noise = (noise + 1) * 0.5f;
            
            uint16 color;
            if (perlin_params.color_mode == 2) {
                float hue = fmodf(perlin_params.metallic_hue + noise * 0.5f, 1.0f);
                float saturation = 0.2f + noise * 0.3f;
                float value = 0.5f + noise * 0.5f;
                color = hsv_to_rgb565(hue, saturation, value);
            } else {
                uint16 color1, color2, color3, color4;
                if (perlin_params.color_mode == 0) {
                    color1 = (31 << 11) | (0 << 5) | 0;
                    color2 = (31 << 11) | (15 << 5) | 0;
                    color3 = (31 << 11) | (31 << 5) | 0;
                    color4 = (31 << 11) | (25 << 5) | 20;
                } else {
                    color1 = (8 << 11) | (8 << 5) | 8;
                    color2 = (16 << 11) | (16 << 5) | 16;
                    color3 = (24 << 11) | (24 << 5) | 24;
                    color4 = (28 << 11) | (28 << 5) | 28;
                }
                
                if (noise < 0.25f) {
                    color = color1;
                } else if (noise < 0.5f) {
                    color = color2;
                } else if (noise < 0.75f) {
                    color = color3;
                } else {
                    color = color4;
                }
            }
            
            texture_data[y * PERLIN_TEXTURE_SIZE + x] = color;
        }
    }
    
    perlin_texture = pvr_mem_malloc(PERLIN_TEXTURE_SIZE * PERLIN_TEXTURE_SIZE * 2);
    pvr_txr_load_ex(texture_data, perlin_texture, PERLIN_TEXTURE_SIZE, PERLIN_TEXTURE_SIZE, PVR_TXRLOAD_16BPP);
    
    free(texture_data);
}

void render_png_cube(void) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t *vert;
    pvr_dr_state_t dr_state;
    float scale = 1.0f;
    matrix_t model_view_matrix __attribute__((aligned(32)));

    mat_identity();
    mat_perspective_fov(45.0f, 640.0f / 480.0f, 0.1f, 100.0f);

    point_t eye = {0, 0, 0};
    point_t center = {0, 0, -1};
    vector_t up = {0, 1, 0};
    mat_lookat(&eye, &center, &up);

    float zoom_scale = 300.0f / (-cube_z);

    mat_identity();
    mat_translate(cube_x, cube_y, cube_z);
    mat_rotate_x(xrot);
    mat_rotate_y(yrot);
    mat_scale(scale * zoom_scale, scale * zoom_scale, scale * zoom_scale);
    mat_store(&model_view_matrix);

    float vertices[24][3] = {
        {-scale, -scale, +scale}, {+scale, -scale, +scale}, {-scale, +scale, +scale}, {+scale, +scale, +scale},
        {+scale, -scale, +scale}, {+scale, -scale, -scale}, {+scale, +scale, +scale}, {+scale, +scale, -scale},
        {+scale, -scale, -scale}, {-scale, -scale, -scale}, {+scale, +scale, -scale}, {-scale, +scale, -scale},
        {-scale, -scale, -scale}, {-scale, -scale, +scale}, {-scale, +scale, -scale}, {-scale, +scale, +scale},
        {-scale, +scale, +scale}, {+scale, +scale, +scale}, {-scale, +scale, -scale}, {+scale, +scale, -scale},
        {-scale, -scale, -scale}, {+scale, -scale, -scale}, {-scale, -scale, +scale}, {+scale, -scale, +scale}
    };

    float tex_coords[4][2] = {{0, 1}, {1, 1}, {0, 0}, {1, 0}};

    pvr_dr_init(dr_state);

    for (int i = 0; i < 6; i++) {
        pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_ARGB4444,
                         textures[i]->w, textures[i]->h,
                         textures[i]->ptr, PVR_FILTER_BILINEAR);
        cxt.gen.culling = PVR_CULLING_CCW;
        
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

void render_perlin_cube(void) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t *vert;
    pvr_dr_state_t dr_state;
    float scale = 1.0f;
    matrix_t model_view_matrix __attribute__((aligned(32)));

    mat_identity();
    mat_perspective_fov(45.0f, 640.0f / 480.0f, 0.1f, 100.0f);

    point_t eye = {0, 0, 0};
    point_t center = {0, 0, -1};
    vector_t up = {0, 1, 0};
    mat_lookat(&eye, &center, &up);

    float zoom_scale = 300.0f / (-cube_z);

    mat_identity();
    mat_translate(cube_x, cube_y, cube_z);
    mat_rotate_x(xrot);
    mat_rotate_y(yrot);
    mat_scale(scale * zoom_scale, scale * zoom_scale, scale * zoom_scale);
    mat_store(&model_view_matrix);

    float vertices[24][3] = {
        {-scale, -scale, +scale}, {+scale, -scale, +scale}, {-scale, +scale, +scale}, {+scale, +scale, +scale},
        {+scale, -scale, +scale}, {+scale, -scale, -scale}, {+scale, +scale, +scale}, {+scale, +scale, -scale},
        {+scale, -scale, -scale}, {-scale, -scale, -scale}, {+scale, +scale, -scale}, {-scale, +scale, -scale},
        {-scale, -scale, -scale}, {-scale, -scale, +scale}, {-scale, +scale, -scale}, {-scale, +scale, +scale},
        {-scale, +scale, +scale}, {+scale, +scale, +scale}, {-scale, +scale, -scale}, {+scale, +scale, -scale},
        {-scale, -scale, -scale}, {+scale, -scale, -scale}, {-scale, -scale, +scale}, {+scale, -scale, +scale}
    };

    float tex_coords[4][2] = {{0, 1}, {1, 1}, {0, 0}, {1, 0}};

    pvr_dr_init(dr_state);

    pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_RGB565,
                     PERLIN_TEXTURE_SIZE,PERLIN_TEXTURE_SIZE,
                     perlin_texture, PVR_FILTER_BILINEAR);
    cxt.gen.culling = PVR_CULLING_CCW;
    cxt.blend.src = PVR_BLEND_SRCALPHA;
    cxt.blend.dst = PVR_BLEND_DESTCOLOR	;
    
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));

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
            vert->argb = PVR_PACK_COLOR(0.5f, 1.0f, 1.0f, 1.0f);  // Adjust alpha for blend strength
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
    if (perlin_texture) {
        pvr_mem_free(perlin_texture);
    }
    pvr_shutdown();
}

int main(int argc, char *argv[]) {
    pvr_init_params_t params = {
        { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 },
        512 * 1024
    };

    pvr_init(&params);
    pvr_set_bg_color(0.0f, 0.0f, 0.1f);

    load_cube_textures();
    create_perlin_texture();

    float rotation_speed = 0.05f;
    
     adx_dec( "/cd/sample.adx", 1 );
    
    while (1) {
        pvr_wait_ready();
        pvr_scene_begin();

        pvr_list_begin(PVR_LIST_OP_POLY);
        render_png_cube();
        pvr_list_finish();

        pvr_list_begin(PVR_LIST_TR_POLY);
        render_perlin_cube();
        pvr_list_finish();

        pvr_scene_finish();

        perlin_params.offset_y += 0.01f;
        create_perlin_texture();

        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, state)
            if (state->buttons & CONT_START)
                goto out;

            // Enhanced analog stick control for rotation
            if (abs(state->joyx) > 16 || abs(state->joyy) > 16) {
                float angle_x = (state->joyy / 128.0f) * rotation_speed;
                float angle_y = (state->joyx / 128.0f) * rotation_speed;

                xrot += angle_x;
                yrot += angle_y;

                while (xrot > 2*F_PI) xrot -= 2*F_PI;
                while (xrot < -2*F_PI) xrot += 2*F_PI;
                while (yrot > 2*F_PI) yrot -= 2*F_PI;
                while (yrot < -2*F_PI) yrot += 2*F_PI;
            }

            // Zoom functionality with triggers
            float ZOOM_SPEED = 0.10f;
            if (state->ltrig > 16) {
                cube_z -= (state->ltrig / 255.0f) * ZOOM_SPEED;
            }
            if (state->rtrig > 16) {
                cube_z += (state->rtrig / 255.0f) * ZOOM_SPEED;
            }

            if (cube_z < -10.0f) cube_z = -10.0f;
            if (cube_z > -0.5f) cube_z = -0.5f;

            // Perlin noise parameter controls
            if (state->buttons & CONT_DPAD_UP) {
                perlin_params.scale *= 1.1f;
                create_perlin_texture();
            }
            if (state->buttons & CONT_DPAD_DOWN) {
                perlin_params.scale *= 0.9f;
                create_perlin_texture();
            }
            if (state->buttons & CONT_DPAD_LEFT) {
                perlin_params.persistence = fmaxf(perlin_params.persistence - 0.05f, 0.1f);
                create_perlin_texture();
            }
            if (state->buttons & CONT_DPAD_RIGHT) {
                perlin_params.persistence = fminf(perlin_params.persistence + 0.05f, 1.0f);
                create_perlin_texture();
            }

            // Lacunarity adjustment
            if ((state->buttons & CONT_A) && (state->buttons & CONT_X)) {
                perlin_params.lacunarity = fminf(perlin_params.lacunarity * 1.1f, 4.0f);
                create_perlin_texture();
            }
            if ((state->buttons & CONT_A) && (state->buttons & CONT_Y)) {
                perlin_params.lacunarity = fmaxf(perlin_params.lacunarity * 0.9f, 1.0f);
                create_perlin_texture();
            }

            // Octaves adjustment
            if ((state->buttons & CONT_B) && (state->buttons & CONT_X)) {
                perlin_params.octaves = fminf(perlin_params.octaves + 1, 8);
                create_perlin_texture();
            }
            if ((state->buttons & CONT_B) && (state->buttons & CONT_Y)) {
                perlin_params.octaves = fmaxf(perlin_params.octaves - 1, 1);
                create_perlin_texture();
            }

            // Color mode toggle
            static int color_mode_cooldown = 0;
            if ((state->buttons & CONT_A) && (state->buttons & CONT_B) && color_mode_cooldown == 0) {
                perlin_params.color_mode = (perlin_params.color_mode + 1) % 3;
                create_perlin_texture();
                color_mode_cooldown = 15;
            }
            if (color_mode_cooldown > 0) color_mode_cooldown--;

            // Metallic hue adjustment (only affects metallic mode)
            if (perlin_params.color_mode == 2) {
                if (state->buttons & CONT_X) {
                    perlin_params.metallic_hue = fmodf(perlin_params.metallic_hue + 0.02f, 1.0f);
                    create_perlin_texture();
                }
                if (state->buttons & CONT_Y) {
                    perlin_params.metallic_hue = fmodf(perlin_params.metallic_hue - 0.02f, 1.0f);
                    create_perlin_texture();
                }
            }

        MAPLE_FOREACH_END()
    }

out:
	snddrv_exit();
	thd_sleep(20);
	pvr_shutdown();
    vid_shutdown();
    return 0;
}
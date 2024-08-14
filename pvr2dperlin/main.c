/********************************************************************************************/
/* KallistiOS Direct PVR API:2.0  Perlin 2d noise example                                   */
/********************************************************************************************/
/********************************************************************************************/
/* Name:     main.c                                                                         */
/* Title:    KallistiOS Direct PVR API:2.0  Perlin 2d noise example                         */
/* Author:   (c)Ian Micheal                                                                 */
/* Created:  08/12/24                                                                       */
/*                                                                                          */
/* Version:  1.0                                                                            */
/* Platform: Dreamcast | KallistiOS:2.0 | KOSPVR |                                          */
/*                                                                                          */
/* Description: perlin 2d noise example                                                     */
/* The purpose of this example is to show the use of only the KOSPVR API to do 3D           */
/* And commented so anyone who knows OpenGL can use the DIRECT NO LAYER KOSPVR API.         */
/* History: version 1 - Added Perlin noise2D                                                */
/********************************************************************************************/
/* GhettoPlay: Text font rendering borrowed :)
   (c)2000 Megan Potter
	Name: Font rendering 
	Copyright: 2023
	Author: Ian micheal
	Date: 05/09/23 06:22
	Description: Removed mouse point and made it lean :)
*/
#include <kos.h>
#include <dc/pvr.h>
#include <math.h>
#include <dc/fmath.h>
#include <dc/matrix.h>
#include <dc/pvr.h>
#include <dc/sq.h>
#include <png/png.h>
#include <stdio.h>
#include <stdlib.h>
#include <dc/video.h>
#include "fontnew.h"  // Include the new font header
#include "perlin.h"

#define M_PI 3.14159265358979323846264338327950288419716939937510f
#define PERLIN_TEXTURE_SIZE 16
#define NUM_TEXTURES 1

extern uint8 romdisk[];
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

typedef struct {
    float scale;
    float persistence;
    float lacunarity;
    int octaves;
    float offset_x;
    float offset_y;
    int color_mode; // 0 for fire, 1 for smoke, 2 for metallic
    float metallic_hue; // For metallic color variation
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

pvr_ptr_t perlin_texture = NULL;
kos_texture_t* dc_logo_texture = NULL;
int toggle_cooldown = 0;
int text_needs_update = 1;
int show_interface = 1;
int prev_ltrig = 0;
int prev_rtrig = 0;

float avrg_gpu_time = 0;
float avrg_cpu_time = 0;
float avrg_idle_time = 0;
float total_frame_time = 0;

// Function prototypes
float perlin_noise_2D(float x, float y, int seed);
void setup_util_texture();
void draw_poly_box(float x1, float y1, float x2, float y2, float z, float r1, float g1, float b1, float a1, float r2, float g2, float b2, float a2);
void draw_poly_strf(float x, float y, float z, float r, float g, float b, float a, char *fmt, ...);

static inline uint16 blend_colors(uint16 c1, uint16 c2, float t) {
    uint32 r1 = (c1 >> 11) & 0x1F, g1 = (c1 >> 5) & 0x3F, b1 = c1 & 0x1F;
    uint32 r2 = (c2 >> 11) & 0x1F, g2 = (c2 >> 5) & 0x3F, b2 = c2 & 0x1F;
    uint32 r = (uint32)(r1 * (1-t) + r2 * t);
    uint32 g = (uint32)(g1 * (1-t) + g2 * t);
    uint32 b = (uint32)(b1 * (1-t) + b2 * t);
    return (r << 11) | (g << 5) | b;
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
    
    uint16 color1, color2, color3,color4;
    if (perlin_params.color_mode == 0) {
   // Enhanced Fire colors
        color1 = (31 << 11) | (0 << 5) | 0;        // Red
        color2 = (31 << 11) | (15 << 5) | 0;       // Orange
        color3 = (31 << 11) | (31 << 5) | 0;       // Yellow
        color4 = (31 << 11) | (25 << 5) | 20;      // Light yellow
    } else if (perlin_params.color_mode == 1) {
        // Enhanced Smoke colors
        color1 = (8 << 11) | (8 << 5) | 8;         // Dark gray
        color2 = (16 << 11) | (16 << 5) | 16;      // Medium gray
        color3 = (24 << 11) | (24 << 5) | 24;      // Light gray
        color4 = (28 << 11) | (28 << 5) | 28;      // Very light gray
    } else {
        // Metallic colors (will be calculated per pixel)
        color1 = color2 = color3 = color4 = 0; // Not used for metallic
    }
    
    for (int y = 0; y < PERLIN_TEXTURE_SIZE; y++) {
        for (int x = 0; x < PERLIN_TEXTURE_SIZE; x++) {
            float sample_x = (x + perlin_params.offset_x) / perlin_params.scale;
            float sample_y = (y + perlin_params.offset_y) / perlin_params.scale;
            
            // Use fbm_noise_2D instead of manual octave calculation
            float noise = fbm_noise_2D(sample_x, sample_y, 
                                       perlin_params.octaves, 
                                       perlin_params.lacunarity, 
                                       perlin_params.persistence);
            
            // Normalize to 0-1 range
            noise = (noise + 1) * 0.5f;
            
            uint16 color;
            if (perlin_params.color_mode == 2) {
                // Enhanced metallic color
                float hue = fmodf(perlin_params.metallic_hue + noise * 0.5f, 1.0f);
                float saturation = 0.2f + noise * 0.3f;  // Reduced saturation for metallic look
                float value = 0.5f + noise * 0.5f;
                color = hsv_to_rgb565(hue, saturation, value);
            } else {
                if (noise < 0.25f) {
                    color = blend_colors(color1, color2, noise / 0.25f);
                } else if (noise < 0.5f) {
                    color = blend_colors(color2, color3, (noise - 0.25f) / 0.25f);
                } else if (noise < 0.75f) {
                    color = blend_colors(color3, color4, (noise - 0.5f) / 0.25f);
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

void render_backdrop() {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t *vert;
    pvr_dr_state_t dr_state;

    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565, PERLIN_TEXTURE_SIZE, PERLIN_TEXTURE_SIZE, perlin_texture, PVR_FILTER_BILINEAR);
    cxt.gen.culling = PVR_CULLING_NONE;
    pvr_poly_compile(&hdr, &cxt);
    
    pvr_prim(&hdr, sizeof(hdr));
    
    pvr_dr_init(dr_state);

    vert = pvr_dr_target(dr_state);
    vert->flags = PVR_CMD_VERTEX;
    vert->x = 0.0f; vert->y = 0.0f; vert->z = 1.0f;
    vert->u = 0.0f; vert->v = 0.0f;
    vert->argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert->oargb = 0;
    pvr_dr_commit(vert);

    vert = pvr_dr_target(dr_state);
    vert->flags = PVR_CMD_VERTEX;
    vert->x = 640.0f; vert->y = 0.0f; vert->z = 1.0f;
    vert->u = 1.0f; vert->v = 0.0f;
    vert->argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert->oargb = 0;
    pvr_dr_commit(vert);

    vert = pvr_dr_target(dr_state);
    vert->flags = PVR_CMD_VERTEX;
    vert->x = 0.0f; vert->y = 480.0f; vert->z = 1.0f;
    vert->u = 0.0f; vert->v = 1.0f;
    vert->argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert->oargb = 0;
    pvr_dr_commit(vert);

    vert = pvr_dr_target(dr_state);
    vert->flags = PVR_CMD_VERTEX_EOL;
    vert->x = 640.0f; vert->y = 480.0f; vert->z = 1.0f;
    vert->u = 1.0f; vert->v = 1.0f;
    vert->argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert->oargb = 0;
    pvr_dr_commit(vert);
}
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

void init_poly_context(pvr_poly_cxt_t *cxt) {
    pvr_poly_cxt_txr(cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444, dc_logo_texture->w, dc_logo_texture->h, dc_logo_texture->ptr, PVR_FILTER_BILINEAR);
    cxt->gen.culling = PVR_CULLING_CCW;
}

void render_text_overlay() {
    if (!show_interface) return;

    int y = 20;
    int x = 10;
    int max_width = 0;

    char buffer[384];
    const char* color_mode_str;
    switch(perlin_params.color_mode) {
        case 0: color_mode_str = "Fire"; break;
        case 1: color_mode_str = "Smoke"; break;
        case 2: color_mode_str = "Metallic"; break;
        default: color_mode_str = "Unknown";
    }

    const char* static_lines[] = {
        "Controls:",
        "A+B: Toggle Color Mode",
        "X/Y: Adjust Metallic Hue",
        "D-Pad: Scale/Persistence",
        "A+X/Y: Lacunarity",
        "B+X/Y: Octaves",
        "Analog: Offset",
        "Triggers: Reset"
    };

    char* dynamic_lines[] = {
        buffer,
        buffer + 64,
        buffer + 128,
        buffer + 192,
        buffer + 256,
        buffer + 320
    };

    snprintf(dynamic_lines[0], 64, "Mode: %s", color_mode_str);
    snprintf(dynamic_lines[1], 64, "Scale: %.2f", (double)perlin_params.scale);
    snprintf(dynamic_lines[2], 64, "Persistence: %.2f", (double)perlin_params.persistence);
    snprintf(dynamic_lines[3], 64, "Lacunarity: %.2f", (double)perlin_params.lacunarity);
    snprintf(dynamic_lines[4], 64, "Octaves: %d", perlin_params.octaves);
    snprintf(dynamic_lines[5], 64, "Metallic Hue: %.2f", (double)perlin_params.metallic_hue);

    const char* all_lines[14];
    for (int i = 0; i < 8; i++) all_lines[i] = static_lines[i];
    for (int i = 0; i < 6; i++) all_lines[i+8] = dynamic_lines[i];

    for (int i = 0; i < 14; i++) {
        int width = strlen(all_lines[i]) * 12;  // Assuming 12 pixels per character
        if (width > max_width) max_width = width;
    }

   // Draw the semi-transparent  box for the main menu
    draw_poly_box(x - 15, y - 5, x + max_width + 25, y + 14 * 24 + 5, 1.5f,
                  1.0f, 0.0f, 0.0f, 0.7f, 1.0f, 0.0f, 0.0f, 0.0f);

    // Render text in the blue box
    pvr_prim(&util_txr_hdr, sizeof(util_txr_hdr));
    for (int i = 0; i < 14; i++) {
        draw_poly_strf(x, y, 5.0f, 1.0f, 1.0f, 1.0f, 1.0f, (char *)all_lines[i]);
        y += 24;
    }


    // Render the DC logo
    if (dc_logo_texture) {
        pvr_poly_cxt_t cxt;
        pvr_poly_hdr_t hdr;
        pvr_vertex_t vert;

        init_poly_context(&cxt);
        pvr_poly_compile(&hdr, &cxt);
        pvr_prim(&hdr, sizeof(hdr));

        vert.flags = PVR_CMD_VERTEX;
        vert.x = 640 - dc_logo_texture->w; vert.y = 0; vert.z = 1;
        vert.u = 0.0f; vert.v = 0.0f;
        vert.argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
        vert.oargb = 0;
        pvr_prim(&vert, sizeof(vert));

        vert.x = 640; vert.y = 0;
        vert.u = 1.0f; vert.v = 0.0f;
        pvr_prim(&vert, sizeof(vert));

        vert.x = 640 - dc_logo_texture->w; vert.y = dc_logo_texture->h;
        vert.u = 0.0f; vert.v = 1.0f;
        pvr_prim(&vert, sizeof(vert));

        vert.flags = PVR_CMD_VERTEX_EOL;
        vert.x = 640; vert.y = dc_logo_texture->h;
        vert.u = 1.0f; vert.v = 1.0f;
        pvr_prim(&vert, sizeof(vert));
    }

    float gpu_usage = avrg_gpu_time / total_frame_time;
    float cpu_usage = avrg_cpu_time / total_frame_time;
    float idle_usage = 1.0f - gpu_usage - cpu_usage;

    gpu_usage = fminf(fmaxf(gpu_usage, 0.0f), 1.0f);
    cpu_usage = fminf(fmaxf(cpu_usage, 0.0f), 1.0f);
    idle_usage = fminf(fmaxf(idle_usage, 0.0f), 1.0f);

    int metrics_y = 380;
    int bar_height = 15;
    int bar_spacing = 2;
    int bar_width = 300;
    int label_offset = 5; // The offset of the label relative to the top of the bar
    int bar_y_offset = 380; // Starting y position for the bars
    int box_padding = 10; // Padding for the box around the bars
    
    int box_height = 4 * (bar_height + bar_spacing) + box_padding * 2;

    // Draw semi-transparent dark gray background for performance metrics
    draw_poly_box(5, metrics_y - box_padding, 
                  10 + bar_width + box_padding, 
                  metrics_y + box_height - box_padding,  1.5f,
                  1.0f, 0.0f, 0.0f, 0.7f, 1.0f, 0.0f, 0.0f, 0.0f); // Same color for fill
                  
   
              
              // Draw CPU usage bar (Blue)
              draw_poly_box(10, metrics_y + bar_height + bar_spacing, 10 + (int)(300 * cpu_usage), metrics_y + 2*bar_height + bar_spacing, 1.5f,
                   0.5f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.5f);
              
draw_poly_box(
    10,  // x1 coordinate
    metrics_y + bar_height + bar_spacing + 17,  // y1 coordinate (moved down by 14 pixels in total)
    10 + (int)(300 * idle_usage),  // x2 coordinate
    metrics_y + 2 * bar_height + bar_spacing + 17,  // y2 coordinate (moved down by 14 pixels in total)
    1.5f,  // Line thickness
    0.7f,  // Red component (for the border color)
    0.7f,  // Green component (for the border color)
    0.0f,  // Blue component (for the border color)
    0.0f,  // Alpha (transparency for the border color)
    0.7f,  // Red component (for the fill color)
    0.7f,  // Green component (for the fill color)
    0.0f,  // Blue component (for the fill color)
    0.0f   // Alpha (transparency for the fill color)
);
 draw_poly_box(8, metrics_y - 4, 8 + (int)(300 * gpu_usage), metrics_y + bar_height - 4, 1.5f,
              0.5f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.5f);
 
 

 // draw_poly_box(8, metrics_y - 4, 8 + (int)(300 * gpu_usage), metrics_y + bar_height - 4, 1.5f,
      //        0.0f, 1.0f, 0.2f, 1.0f, 0.0f, 0.8f, 0.1f, 1.0f);

    // Render labels for the bars
    pvr_prim(&util_txr_hdr, sizeof(util_txr_hdr));
         // Render the label for Idle time
    draw_poly_strf(15, bar_y_offset + 2 * (bar_height + bar_spacing) - label_offset, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, "Idle: %.1f%%", idle_usage * 100);
    draw_poly_strf(15, metrics_y - 10, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, "GPU: %.1f%%", gpu_usage * 100);
  
    draw_poly_strf(15, bar_y_offset + bar_height + bar_spacing - label_offset, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, "CPU: %.1f%%", cpu_usage * 100);
  
    
    draw_poly_strf(15, metrics_y + 3*(bar_height + bar_spacing), 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, "Total: %.2fms", total_frame_time);
    
}

int main(int argc, char *argv[]) {
    pvr_init_params_t params = {
        { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 },
        512 * 1024
    };

    pvr_init(&params);
    create_perlin_texture();
    setup_util_texture();  // Initialize the font texture
    dc_logo_texture = load_png_texture("/rd/dc_logo.png");
    if (!dc_logo_texture) {
        printf("Failed to load Dreamcast logo texture\n");
    }

    int prev_buttons = 0;

    while(1) {
        uint64 start_time, gpu_start, gpu_end, cpu_start, cpu_end;
        start_time = timer_us_gettime64();

        pvr_wait_ready();
        gpu_start = timer_us_gettime64();
        pvr_scene_begin();

        pvr_list_begin(PVR_LIST_OP_POLY);
        render_backdrop();
        pvr_list_finish();

        pvr_list_begin(PVR_LIST_TR_POLY);
        render_text_overlay();
        pvr_list_finish();

        pvr_scene_finish();
        gpu_end = timer_us_gettime64();

        cpu_start = timer_us_gettime64();
        
        MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, state)
            if (state->buttons & CONT_START)
                goto out;
            
            // Toggle color mode (A+B)
            if ((state->buttons & (CONT_A | CONT_B)) == (CONT_A | CONT_B) &&
                !(prev_buttons & (CONT_A | CONT_B)) && toggle_cooldown == 0) {
                perlin_params.color_mode = (perlin_params.color_mode + 1) % 3;
                toggle_cooldown = 15;
                text_needs_update = 1;
            }

            // Metallic hue adjustment (X, Y)
            if (state->buttons & CONT_X) {
                perlin_params.metallic_hue = fmodf(perlin_params.metallic_hue + 0.02f, 1.0f);
                text_needs_update = 1;
            }
            if (state->buttons & CONT_Y) {
                perlin_params.metallic_hue = fmodf(perlin_params.metallic_hue - 0.02f, 1.0f);
                text_needs_update = 1;
            }

            // Scale adjustment (Up, Down)
            if (state->buttons & CONT_DPAD_UP) {
                perlin_params.scale *= 1.1f;
                text_needs_update = 1;
            }
            if (state->buttons & CONT_DPAD_DOWN) {
                perlin_params.scale *= 0.9f;
                text_needs_update = 1;
            }
            
            // Persistence adjustment (Left, Right)
            if (state->buttons & CONT_DPAD_LEFT) {
                perlin_params.persistence = fmaxf(perlin_params.persistence - 0.05f, 0.1f);
                text_needs_update = 1;
            }
            if (state->buttons & CONT_DPAD_RIGHT) {
                perlin_params.persistence = fminf(perlin_params.persistence + 0.05f, 1.0f);
                text_needs_update = 1;
            }

            // Lacunarity adjustment (A+X, A+Y)
            if ((state->buttons & (CONT_A | CONT_X)) == (CONT_A | CONT_X)) {
                perlin_params.lacunarity = fminf(perlin_params.lacunarity * 1.1f, 4.0f);
                text_needs_update = 1;
            }
            if ((state->buttons & (CONT_A | CONT_Y)) == (CONT_A | CONT_Y)) {
                perlin_params.lacunarity = fmaxf(perlin_params.lacunarity * 0.9f, 1.0f);
                text_needs_update = 1;
            }

            // Octaves adjustment (B+X, B+Y)
            if ((state->buttons & (CONT_B | CONT_X)) == (CONT_B | CONT_X)) {
                perlin_params.octaves = fminf(perlin_params.octaves + 1, 8);
                text_needs_update = 1;
            }
            if ((state->buttons & (CONT_B | CONT_Y)) == (CONT_B | CONT_Y)) {
                perlin_params.octaves = fmaxf(perlin_params.octaves - 1, 1);
                text_needs_update = 1;
            }

            // Offset adjustment (analog stick)
            if (abs(state->joyx) > 16 || abs(state->joyy) > 16) {
                perlin_params.offset_x += state->joyx / 100.0f;
                perlin_params.offset_y -= state->joyy / 100.0f;
                text_needs_update = 1;
            }

            // Reset parameters (both triggers)
            if ((state->ltrig > 200) && (state->rtrig > 200)) {
                perlin_params.scale = 32.0f;
                perlin_params.persistence = 0.5f;
                perlin_params.lacunarity = 2.0f;
                perlin_params.octaves = 4;
                perlin_params.offset_x = 0.0f;
                perlin_params.offset_y = 0.0f;
                perlin_params.color_mode = 0;
                perlin_params.metallic_hue = 0.0f;
                text_needs_update = 1;
            }

            // Toggle interface visibility
            if (state->ltrig > 200 && prev_ltrig <= 200) {
                show_interface = 0;
            }
            if (state->rtrig > 200 && prev_rtrig <= 200) {
                show_interface = 1;
            }

            prev_ltrig = state->ltrig;
            prev_rtrig = state->rtrig;
            prev_buttons = state->buttons;
        MAPLE_FOREACH_END()

        if (toggle_cooldown > 0) toggle_cooldown--;

        // Constant movement (increased speed)
        perlin_params.offset_y += 2.0f;

        if (text_needs_update) {
            create_perlin_texture();
            text_needs_update = 0;
        }

        cpu_end = timer_us_gettime64();

        float gpu_time = (gpu_end - gpu_start) / 1000.0f;
        float cpu_time = (cpu_end - cpu_start) / 1000.0f;
        total_frame_time = (cpu_end - start_time) / 1000.0f;
        float idle_time = total_frame_time - gpu_time - cpu_time;

        // Update our performance metrics
        avrg_gpu_time = avrg_gpu_time * 0.95f + gpu_time * 0.05f;
        avrg_cpu_time = avrg_cpu_time * 0.95f + cpu_time * 0.05f;
        avrg_idle_time = avrg_idle_time * 0.95f + idle_time * 0.05f;
    }

out:
    if (perlin_texture != NULL) {
        pvr_mem_free(perlin_texture);
    }
    if (util_texture != NULL) {
        pvr_mem_free(util_texture);
    }
    if (dc_logo_texture != NULL) {
        pvr_mem_free(dc_logo_texture->ptr);
        free(dc_logo_texture);
    }
    pvr_shutdown();
    return 0;
}
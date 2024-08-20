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
#include <kos.h> /* Includes necessary KallistiOS (KOS) headers for Dreamcast development */
#include <dc/pvr.h> /* PVR library headers for PowerVR graphics chip functions */
#include <math.h> /* Standard math library headers for mathematical functions */
#include <dc/fmath.h> /* Fast math library headers for optimized mathematical functions */
#include <dc/matrix.h> /* Matrix library headers for handling matrix operations */
#include <dc/sq.h> /* SH-4 Store Queue library headers for optimized memory transfers */
#include <png/png.h> /* PNG library headers for handling PNG images */
#include <stdio.h> /* Standard I/O library headers for input and output functions */
#include <stdlib.h> /* Standard library headers for general-purpose functions */
#include <dc/video.h> /* Video library headers for video display functions */
#include "fontnew.h" /* Custom font header for font rendering */
#include "perlin.h" /* Custom Perlin noise header for procedural texture generation */

#define M_PI 3.14159265358979323846264338327950288419716939937510f
#define PERLIN_TEXTURE_SIZE 16
#define NUM_TEXTURES 1

extern uint8 romdisk[];
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

/**
 * @struct PerlinParams
 * @brief Parameters for Perlin noise generation and visualization
 */
typedef struct {
    float scale;        /* Scale factor for noise */
    float persistence;  /* Persistence of octaves */
    float lacunarity;   /* Frequency multiplier between octaves */
    int octaves;        /* Number of octaves for noise generation */
    float offset_x;     /* X-axis offset for noise sampling */
    float offset_y;     /* Y-axis offset for noise sampling */
    int color_mode;     /* Color mode: 0 for fire, 1 for smoke, 2 for metallic */
    float metallic_hue; /* Hue variation for metallic color mode */
} PerlinParams;

/* Default Perlin noise parameters */
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

/**
 * @struct kos_texture_t
 * @brief Structure for handling textures in KallistiOS
 */
typedef struct {
    pvr_ptr_t ptr;  /* Pointer to texture data in PVR memory */
    int w, h;       /* Width and height of the texture */
    uint32 fmt;     /* Texture format */
} kos_texture_t;

/* Global variables */
pvr_ptr_t perlin_texture = NULL;      /* Pointer to Perlin noise texture */
kos_texture_t* dc_logo_texture = NULL; /* Pointer to Dreamcast logo texture */
int toggle_cooldown = 0;              /* Cooldown for toggling interface */
int text_needs_update = 1;            /* Flag for text update requirement */
int show_interface = 1;               /* Flag to show/hide interface */
int prev_ltrig = 0;                   /* Previous state of left trigger */
int prev_rtrig = 0;                   /* Previous state of right trigger */
float avrg_gpu_time = 0;              /* Average GPU processing time */
float avrg_cpu_time = 0;              /* Average CPU processing time */
float avrg_idle_time = 0;             /* Average idle time */
float total_frame_time = 0;           /* Total frame processing time */

/* Function prototypes */
float perlin_noise_2D(float x, float y, int seed);
void setup_util_texture();
void draw_poly_box(float x1, float y1, float x2, float y2, float z, 
                   float r1, float g1, float b1, float a1, 
                   float r2, float g2, float b2, float a2);
void draw_poly_strf(float x, float y, float z, 
                    float r, float g, float b, float a, 
                    char *fmt, ...);

/**
 * @brief Blend two 16-bit colors
 * @param c1 First color
 * @param c2 Second color
 * @param t Blend factor (0.0 to 1.0)
 * @return Blended color
 */
static inline uint16 blend_colors(uint16 c1, uint16 c2, float t) {
    uint32 r1 = (c1 >> 11) & 0x1F, g1 = (c1 >> 5) & 0x3F, b1 = c1 & 0x1F;
    uint32 r2 = (c2 >> 11) & 0x1F, g2 = (c2 >> 5) & 0x3F, b2 = c2 & 0x1F;
    uint32 r = (uint32)(r1 * (1-t) + r2 * t);
    uint32 g = (uint32)(g1 * (1-t) + g2 * t);
    uint32 b = (uint32)(b1 * (1-t) + b2 * t);
    return (r << 11) | (g << 5) | b;
}

/**
 * @brief Convert HSV color to RGB565 format
 * 
 * This function takes HSV (Hue, Saturation, Value) color values and
 * converts them to the RGB565 color format, which is commonly used
 * in embedded systems and game consoles like the Dreamcast.
 * 
 * @param h Hue value (0.0 to 1.0)
 * @param s Saturation value (0.0 to 1.0)
 * @param v Value (brightness) (0.0 to 1.0)
 * @return uint16 Color in RGB565 format
 */
uint16 hsv_to_rgb565(float h, float s, float v) {
    float r = 0, g = 0, b = 0;  // Initialize RGB values

    // Convert hue to a value between 0 and 6
    int i = (int)(h * 6);
    // Calculate fractional part of h * 6
    float f = h * 6 - i;

    // Calculate intermediate values for HSV to RGB conversion
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    // Determine RGB values based on the hue sector
    switch (i % 6) {
        case 0: r = v, g = t, b = p; break;  // Red to Yellow
        case 1: r = q, g = v, b = p; break;  // Yellow to Green
        case 2: r = p, g = v, b = t; break;  // Green to Cyan
        case 3: r = p, g = q, b = v; break;  // Cyan to Blue
        case 4: r = t, g = p, b = v; break;  // Blue to Magenta
        case 5: r = v, g = p, b = q; break;  // Magenta to Red
    }

    // Convert floating-point RGB (0-1) to integer RGB565 format
    // Red and Blue use 5 bits (0-31), Green uses 6 bits (0-63)
    uint16 r16 = (uint16)(r * 31);
    uint16 g16 = (uint16)(g * 63);
    uint16 b16 = (uint16)(b * 31);

    // Combine RGB components into a single 16-bit value
    // Format: RRRRRGGGGGGBBBBB
    return (r16 << 11) | (g16 << 5) | b16;
}

/**
 * @brief Create a texture using Perlin noise
 * 
 * This function generates a texture based on Perlin noise and the current
 * color mode settings. It supports fire, smoke, and metallic color schemes.
 */
void create_perlin_texture() {
    // Free existing texture if it exists
    if (perlin_texture != NULL) {
        pvr_mem_free(perlin_texture);
    }
    
    // Allocate memory for the texture data
    // 32-byte aligned, 16 bits per pixel
    uint16 *texture_data = (uint16 *)memalign(32, PERLIN_TEXTURE_SIZE * PERLIN_TEXTURE_SIZE * 2);
    
    // Define color palettes for different modes
    uint16 color1, color2, color3, color4;
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
    
    // Generate the texture data
    for (int y = 0; y < PERLIN_TEXTURE_SIZE; y++) {
        for (int x = 0; x < PERLIN_TEXTURE_SIZE; x++) {
            // Calculate sample coordinates
            float sample_x = (x + perlin_params.offset_x) / perlin_params.scale;
            float sample_y = (y + perlin_params.offset_y) / perlin_params.scale;
            
            // Generate fractal Brownian motion noise
            float noise = fbm_noise_2D(sample_x, sample_y, 
                                       perlin_params.octaves, 
                                       perlin_params.lacunarity, 
                                       perlin_params.persistence);
            
            // Normalize noise to 0-1 range
            noise = (noise + 1) * 0.5f;
            
            uint16 color;
            if (perlin_params.color_mode == 2) {
                // Enhanced metallic color
                float hue = fmodf(perlin_params.metallic_hue + noise * 0.5f, 1.0f);
                float saturation = 0.2f + noise * 0.3f;  // Reduced saturation for metallic look
                float value = 0.5f + noise * 0.5f;
                color = hsv_to_rgb565(hue, saturation, value);
            } else {
                // Color gradient for fire and smoke
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
            
            // Store the calculated color in the texture data
            texture_data[y * PERLIN_TEXTURE_SIZE + x] = color;
        }
    }
    
    // Allocate PVR memory for the texture
    perlin_texture = pvr_mem_malloc(PERLIN_TEXTURE_SIZE * PERLIN_TEXTURE_SIZE * 2);
    
    // Load the texture data into PVR memory
    pvr_txr_load_ex(texture_data, perlin_texture, PERLIN_TEXTURE_SIZE, PERLIN_TEXTURE_SIZE, PVR_TXRLOAD_16BPP);
    
    // Free the temporary texture data
    free(texture_data);
}


/**
 * @brief Render the backdrop using the Perlin noise texture
 * 
 * This function renders a full-screen quad textured with the Perlin noise texture
 * to create a dynamic backdrop effect.
 */
void render_backdrop() {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t *vert;
    pvr_dr_state_t dr_state;

    // Set up the polygon context for textured rendering
    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565, 
                     PERLIN_TEXTURE_SIZE, PERLIN_TEXTURE_SIZE, 
                     perlin_texture, PVR_FILTER_BILINEAR);
    
    // Disable culling to ensure the quad is always visible
    cxt.gen.culling = PVR_CULLING_NONE;
    
    // Compile the polygon header
    pvr_poly_compile(&hdr, &cxt);
    
    // Submit the polygon header to the rendering pipeline
    pvr_prim(&hdr, sizeof(hdr));
    
    // Initialize the direct rendering state
    pvr_dr_init(dr_state);

    // Render a quad (two triangles) covering the entire screen
    // Vertex 1: Top-left
    vert = pvr_dr_target(dr_state);
    vert->flags = PVR_CMD_VERTEX;
    vert->x = 0.0f; vert->y = 0.0f; vert->z = 1.0f;
    vert->u = 0.0f; vert->v = 0.0f;
    vert->argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert->oargb = 0;
    pvr_dr_commit(vert);

    // Vertex 2: Top-right
    vert = pvr_dr_target(dr_state);
    vert->flags = PVR_CMD_VERTEX;
    vert->x = 640.0f; vert->y = 0.0f; vert->z = 1.0f;
    vert->u = 1.0f; vert->v = 0.0f;
    vert->argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert->oargb = 0;
    pvr_dr_commit(vert);

    // Vertex 3: Bottom-left
    vert = pvr_dr_target(dr_state);
    vert->flags = PVR_CMD_VERTEX;
    vert->x = 0.0f; vert->y = 480.0f; vert->z = 1.0f;
    vert->u = 0.0f; vert->v = 1.0f;
    vert->argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert->oargb = 0;
    pvr_dr_commit(vert);

    // Vertex 4: Bottom-right (with EOL flag)
    vert = pvr_dr_target(dr_state);
    vert->flags = PVR_CMD_VERTEX_EOL;
    vert->x = 640.0f; vert->y = 480.0f; vert->z = 1.0f;
    vert->u = 1.0f; vert->v = 1.0f;
    vert->argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert->oargb = 0;
    pvr_dr_commit(vert);
}


/**
 * @brief Load a PNG texture file into PVR memory
 *
 * @param filename Path to the PNG file
 * @return kos_texture_t* Pointer to the loaded texture, or NULL if loading failed
 */
kos_texture_t* load_png_texture(const char *filename) {
    kos_texture_t* texture;
    kos_img_t img;

    // Attempt to load the PNG file into a KOS image structure
    if (png_to_img(filename, PNG_FULL_ALPHA, &img) < 0) {
        return NULL;  // Return NULL if PNG loading fails
    }

    // Allocate memory for the texture structure
    texture = (kos_texture_t*)malloc(sizeof(kos_texture_t));
    if (!texture) {
        kos_img_free(&img, 0);  // Free the KOS image if texture allocation fails
        return NULL;
    }

    // Allocate PVR memory for the texture data
    texture->ptr = pvr_mem_malloc(img.byte_count);
    if (!texture->ptr) {
        free(texture);          // Free the texture structure if PVR memory allocation fails
        kos_img_free(&img, 0);  // Free the KOS image
        return NULL;
    }

    // Set texture properties
    texture->w = img.w;    // Width of the texture
    texture->h = img.h;    // Height of the texture
    texture->fmt = PVR_TXRFMT_ARGB4444;  // Set texture format to ARGB4444

    // Load the image data into PVR memory
    pvr_txr_load_kimg(&img, texture->ptr, 0);

    // Free the KOS image as it's no longer needed
    kos_img_free(&img, 0);

    return texture;  // Return the loaded texture
}

/**
 * @brief Initialize a polygon context for rendering textured polygons
 *
 * @param cxt Pointer to the polygon context to initialize
 */
void init_poly_context(pvr_poly_cxt_t *cxt) {
    // Initialize the polygon context for textured rendering
    pvr_poly_cxt_txr(cxt, 
                     PVR_LIST_TR_POLY,          // Use the translucent polygon list
                     PVR_TXRFMT_ARGB4444,       // Texture format (ARGB4444)
                     dc_logo_texture->w,        // Texture width
                     dc_logo_texture->h,        // Texture height
                     dc_logo_texture->ptr,      // Pointer to texture data in PVR memory
                     PVR_FILTER_BILINEAR);      // Use bilinear filtering for texture sampling

    // Set culling mode to counter-clockwise
    cxt->gen.culling = PVR_CULLING_CCW;
}


/**
 * @brief Render the text overlay with controls and current parameters
 */
void render_text_overlay() {
    // Exit if interface is not meant to be shown
    if (!show_interface) return;

    int y = 20;  // Starting y-coordinate for text
    int x = 10;  // Starting x-coordinate for text
    int max_width = 0;  // To store the maximum width of text lines
    char buffer[384];  // Buffer to store dynamic text lines
    const char* color_mode_str;

    // Determine the current color mode string
    switch(perlin_params.color_mode) {
        case 0: color_mode_str = "Fire"; break;
        case 1: color_mode_str = "Smoke"; break;
        case 2: color_mode_str = "Metallic"; break;
        default: color_mode_str = "Unknown";
    }

    // Static text lines (controls)
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

    // Pointers to different parts of the buffer for dynamic lines
    char* dynamic_lines[] = {
        buffer,
        buffer + 64,
        buffer + 128,
        buffer + 192,
        buffer + 256,
        buffer + 320
    };

    // Fill dynamic lines with current parameter values
    snprintf(dynamic_lines[0], 64, "Mode: %s", color_mode_str);
    snprintf(dynamic_lines[1], 64, "Scale: %.2f", (double)perlin_params.scale);
    snprintf(dynamic_lines[2], 64, "Persistence: %.2f", (double)perlin_params.persistence);
    snprintf(dynamic_lines[3], 64, "Lacunarity: %.2f", (double)perlin_params.lacunarity);
    snprintf(dynamic_lines[4], 64, "Octaves: %d", perlin_params.octaves);
    snprintf(dynamic_lines[5], 64, "Metallic Hue: %.2f", (double)perlin_params.metallic_hue);

    // Combine static and dynamic lines into one array
    const char* all_lines[14];
    for (int i = 0; i < 8; i++) all_lines[i] = static_lines[i];
    for (int i = 0; i < 6; i++) all_lines[i+8] = dynamic_lines[i];

    // Calculate the maximum width of all lines
    for (int i = 0; i < 14; i++) {
        int width = strlen(all_lines[i]) * 12;  // Assuming 12 pixels per character
        if (width > max_width) max_width = width;
    }

    // Draw the semi-transparent box for the main menu
    draw_poly_box(x - 15, y - 5, x + max_width + 25, y + 14 * 24 + 5, 1.5f,
                  1.0f, 0.0f, 0.0f, 0.7f, 1.0f, 0.0f, 0.0f, 0.0f);

    // Set up the texture for text rendering
    pvr_prim(&util_txr_hdr, sizeof(util_txr_hdr));

    // Render each line of text
    for (int i = 0; i < 14; i++) {
        draw_poly_strf(x, y, 5.0f, 1.0f, 1.0f, 1.0f, 1.0f, (char *)all_lines[i]);
        y += 24;  // Move to the next line
    }



// Render the Dreamcast logo
if (dc_logo_texture) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t vert;

    // Initialize the polygon context for the logo
    init_poly_context(&cxt);
    // Compile the polygon header
    pvr_poly_compile(&hdr, &cxt);
    // Submit the polygon header to the rendering pipeline
    pvr_prim(&hdr, sizeof(hdr));

    // Set up the first vertex (top-left of the logo)
    vert.flags = PVR_CMD_VERTEX;
    vert.x = 640 - dc_logo_texture->w; vert.y = 0; vert.z = 1;
    vert.u = 0.0f; vert.v = 0.0f;
    vert.argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    // Set up the second vertex (top-right of the logo)
    vert.x = 640; vert.y = 0;
    vert.u = 1.0f; vert.v = 0.0f;
    pvr_prim(&vert, sizeof(vert));

    // Set up the third vertex (bottom-left of the logo)
    vert.x = 640 - dc_logo_texture->w; vert.y = dc_logo_texture->h;
    vert.u = 0.0f; vert.v = 1.0f;
    pvr_prim(&vert, sizeof(vert));

    // Set up the fourth vertex (bottom-right of the logo)
    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = 640; vert.y = dc_logo_texture->h;
    vert.u = 1.0f; vert.v = 1.0f;
    pvr_prim(&vert, sizeof(vert));
}

// Calculate usage percentages
float gpu_usage = avrg_gpu_time / total_frame_time;
float cpu_usage = avrg_cpu_time / total_frame_time;
float idle_usage = 1.0f - gpu_usage - cpu_usage;

// Clamp usage values between 0 and 1
gpu_usage = fminf(fmaxf(gpu_usage, 0.0f), 1.0f);
cpu_usage = fminf(fmaxf(cpu_usage, 0.0f), 1.0f);
idle_usage = fminf(fmaxf(idle_usage, 0.0f), 1.0f);

// Set up metrics display parameters
int metrics_y = 380;
int bar_height = 15;
int bar_spacing = 2;
int bar_width = 300;
int label_offset = 5;
int bar_y_offset = 380;
int box_padding = 10;

int box_height = 4 * (bar_height + bar_spacing) + box_padding * 2;

// Draw semi-transparent dark gray background for performance metrics
draw_poly_box(5, metrics_y - box_padding, 
              10 + bar_width + box_padding, 
              metrics_y + box_height - box_padding,  1.5f,
              1.0f, 0.0f, 0.0f, 0.7f, 1.0f, 0.0f, 0.0f, 0.0f);

// Draw CPU usage bar (Blue)
draw_poly_box(10, metrics_y + bar_height + bar_spacing, 
              10 + (int)(300 * cpu_usage), 
              metrics_y + 2*bar_height + bar_spacing, 1.5f,
              0.5f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.5f);

// Draw Idle usage bar (Gray)
draw_poly_box(
    10,
    metrics_y + bar_height + bar_spacing + 17,
    10 + (int)(300 * idle_usage),
    metrics_y + 2 * bar_height + bar_spacing + 17,
    1.5f,
    0.7f, 0.7f, 0.0f, 0.0f,
    0.7f, 0.7f, 0.0f, 0.0f
);

// Draw GPU usage bar (Red)
draw_poly_box(8, metrics_y - 4, 
              8 + (int)(300 * gpu_usage), 
              metrics_y + bar_height - 4, 1.5f,
              0.5f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.5f);

// Set up the texture for text rendering
    pvr_prim(&util_txr_hdr, sizeof(util_txr_hdr));
    
    // Render the label for Idle time
    draw_poly_strf(15,                                           // X coordinate
                   bar_y_offset + 2 * (bar_height + bar_spacing) - label_offset,  // Y coordinate
                   2.0f,                                         // Z coordinate (depth)
                   1.0f, 1.0f, 1.0f, 1.0f,                       // RGBA color (white)
                   "Idle: %.1f%%", idle_usage * 100);            // Text with formatted idle usage percentage
    
    // Render the label for GPU usage
    draw_poly_strf(15,                                           // X coordinate
                   metrics_y - 10,                               // Y coordinate
                   2.0f,                                         // Z coordinate (depth)
                   1.0f, 1.0f, 1.0f, 1.0f,                       // RGBA color (white)
                   "GPU: %.1f%%", gpu_usage * 100);              // Text with formatted GPU usage percentage
    
    // Render the label for CPU usage
    draw_poly_strf(15,                                           // X coordinate
                   bar_y_offset + bar_height + bar_spacing - label_offset,  // Y coordinate
                   2.0f,                                         // Z coordinate (depth)
                   1.0f, 1.0f, 1.0f, 1.0f,                       // RGBA color (white)
                   "CPU: %.1f%%", cpu_usage * 100);              // Text with formatted CPU usage percentage
    
    // Render the label for Total frame time
    draw_poly_strf(15,                                           // X coordinate
                   metrics_y + 3*(bar_height + bar_spacing),     // Y coordinate
                   2.0f,                                         // Z coordinate (depth)
                   1.0f, 1.0f, 1.0f, 1.0f,                       // RGBA color (white)
                   "Total: %.2fms", total_frame_time);           // Text with formatted total frame time
}


/**
 * @brief Main function for the Dreamcast application
 * @param argc Argument count
 * @param argv Array of argument strings
 * @return int Return code
 */
int main(int argc, char *argv[]) {
    // Initialize PVR (PowerVR) parameters
    pvr_init_params_t params = {
        // Set bin sizes for different types of primitives
        { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_0 },
        512 * 1024  // Set vertex buffer size to 512 KB
    };
    
    // Initialize the PVR system with the specified parameters
    pvr_init(&params);
    
    // Create the initial Perlin noise texture
    create_perlin_texture();
    
    // Initialize the font texture for text rendering
    setup_util_texture();
    
    // Load the Dreamcast logo texture
    dc_logo_texture = load_png_texture("/rd/dc_logo.png");
    if (!dc_logo_texture) {
        printf("Failed to load Dreamcast logo texture\n");
    }
    
    // Initialize previous button state
    int prev_buttons = 0;
    
    // Main game loop
    while(1) {
        uint64 start_time, gpu_start, gpu_end, cpu_start, cpu_end;
        
        // Get the start time of the frame
        start_time = timer_us_gettime64();
        
        // Wait for the PVR system to be ready
        pvr_wait_ready();
        
        // Record the start time for GPU operations
        gpu_start = timer_us_gettime64();
        
        // Begin a new rendering scene
        pvr_scene_begin();
        
        // Begin the opaque polygon list
        pvr_list_begin(PVR_LIST_OP_POLY);
        // Render the backdrop (Perlin noise texture)
        render_backdrop();
        // Finish the opaque polygon list
        pvr_list_finish();
        
        // Begin the translucent polygon list
        pvr_list_begin(PVR_LIST_TR_POLY);
        // Render the text overlay (UI elements)
        render_text_overlay();
        // Finish the translucent polygon list
        pvr_list_finish();
        
        // Finish the rendering scene
        pvr_scene_finish();
        
        // Record the end time for GPU operations
        gpu_end = timer_us_gettime64();
        
        // Record the start time for CPU operations
        cpu_start = timer_us_gettime64();
	    

MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, state)
    // Check if the Start button is pressed to exit the program
    if (state->buttons & CONT_START)
        goto out;
    
    // Toggle color mode (A+B buttons)
    if ((state->buttons & (CONT_A | CONT_B)) == (CONT_A | CONT_B) &&
        !(prev_buttons & (CONT_A | CONT_B)) && toggle_cooldown == 0) {
        perlin_params.color_mode = (perlin_params.color_mode + 1) % 3;
        toggle_cooldown = 15;  // Set cooldown to prevent rapid toggling
        text_needs_update = 1;  // Flag to update UI text
    }

    // Metallic hue adjustment (X, Y buttons)
    if (state->buttons & CONT_X) {
        perlin_params.metallic_hue = fmodf(perlin_params.metallic_hue + 0.02f, 1.0f);
        text_needs_update = 1;
    }
    if (state->buttons & CONT_Y) {
        perlin_params.metallic_hue = fmodf(perlin_params.metallic_hue - 0.02f, 1.0f);
        text_needs_update = 1;
    }

    // Scale adjustment (Up, Down on D-pad)
    if (state->buttons & CONT_DPAD_UP) {
        perlin_params.scale *= 1.1f;  // Increase scale by 10%
        text_needs_update = 1;
    }
    if (state->buttons & CONT_DPAD_DOWN) {
        perlin_params.scale *= 0.9f;  // Decrease scale by 10%
        text_needs_update = 1;
    }
    
    // Persistence adjustment (Left, Right on D-pad)
    if (state->buttons & CONT_DPAD_LEFT) {
        perlin_params.persistence = fmaxf(perlin_params.persistence - 0.05f, 0.1f);
        text_needs_update = 1;
    }
    if (state->buttons & CONT_DPAD_RIGHT) {
        perlin_params.persistence = fminf(perlin_params.persistence + 0.05f, 1.0f);
        text_needs_update = 1;
    }

    // Lacunarity adjustment (A+X, A+Y buttons)
    if ((state->buttons & (CONT_A | CONT_X)) == (CONT_A | CONT_X)) {
        perlin_params.lacunarity = fminf(perlin_params.lacunarity * 1.1f, 4.0f);
        text_needs_update = 1;
    }
    if ((state->buttons & (CONT_A | CONT_Y)) == (CONT_A | CONT_Y)) {
        perlin_params.lacunarity = fmaxf(perlin_params.lacunarity * 0.9f, 1.0f);
        text_needs_update = 1;
    }

    // Octaves adjustment (B+X, B+Y buttons)
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

           // Reset all Perlin noise parameters to their default values when both triggers are pressed
    if ((state->ltrig > 200) && (state->rtrig > 200)) {
        perlin_params.scale = 32.0f;
        perlin_params.persistence = 0.5f;
        perlin_params.lacunarity = 2.0f;
        perlin_params.octaves = 4;
        perlin_params.offset_x = 0.0f;
        perlin_params.offset_y = 0.0f;
        perlin_params.color_mode = 0;
        perlin_params.metallic_hue = 0.0f;
        text_needs_update = 1;  // Flag to update the UI
    }

    // Toggle interface visibility
    if (state->ltrig > 200 && prev_ltrig <= 200) {
        show_interface = 0;  // Hide interface when left trigger is pressed
    }
    if (state->rtrig > 200 && prev_rtrig <= 200) {
        show_interface = 1;  // Show interface when right trigger is pressed
    }

    // Update previous trigger states for next frame
    prev_ltrig = state->ltrig;
    prev_rtrig = state->rtrig;
    prev_buttons = state->buttons;

MAPLE_FOREACH_END()  // End of controller input processing

// Decrease toggle cooldown if it's active
if (toggle_cooldown > 0) toggle_cooldown--;

// Constant movement of the Perlin noise (scrolling effect)
perlin_params.offset_y += 2.0f;

// Recreate the Perlin texture if any parameters have changed
if (text_needs_update) {
    create_perlin_texture();
    text_needs_update = 0;
}

// Record end time for CPU operations
cpu_end = timer_us_gettime64();

// Calculate timings for this frame
float gpu_time = (gpu_end - gpu_start) / 1000.0f;
float cpu_time = (cpu_end - cpu_start) / 1000.0f;
total_frame_time = (cpu_end - start_time) / 1000.0f;
float idle_time = total_frame_time - gpu_time - cpu_time;

// Update moving averages of performance metrics
// Uses exponential moving average with 0.05 weight for new values
avrg_gpu_time = avrg_gpu_time * 0.95f + gpu_time * 0.05f;
avrg_cpu_time = avrg_cpu_time * 0.95f + cpu_time * 0.05f;
avrg_idle_time = avrg_idle_time * 0.95f + idle_time * 0.05f;

}  // End of main loop

out:  // Label for exiting the program

// Clean up resources
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

// Shut down the PVR system
pvr_shutdown();

return 0;  // Exit the program
}

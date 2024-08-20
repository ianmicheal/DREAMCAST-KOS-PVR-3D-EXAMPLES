
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
#include <stdarg.h>
#include <stdio.h>
#include "fontnew.h"

// Global variables for utility texture and its header
pvr_ptr_t util_texture;
pvr_poly_hdr_t util_txr_hdr;

/**
 * @brief Set up the utility texture for font rendering
 *
 * This function creates a texture containing ASCII characters for text rendering.
 * It allocates memory for the texture, draws characters using bfont, and sets up
 * a polygon header for rendering.
 */
void setup_util_texture() {
    uint16 *vram;
    int x, y;
    pvr_poly_cxt_t base;

    // Allocate memory for the utility texture (256x256 pixels, 16-bit color)
    util_texture = pvr_mem_malloc(256 * 256 * 2);
    vram = (uint16 *)util_texture;

    // Draw ASCII characters into the texture
    for (y = 0; y < 8; y++) {
        for (x = 0; x < 16; x++) {
            bfont_draw(vram, 256, 0, y * 16 + x);
            vram += 16;
        }
        vram += 23 * 256;
    }

    // Set up a polygon context for the utility texture
    pvr_poly_cxt_txr(&base, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_NONTWIDDLED,
                     256, 256, util_texture, PVR_FILTER_NONE);
    pvr_poly_compile(&util_txr_hdr, &base);
}

/**
 * @brief Draw a single character as a textured polygon
 *
 * @param x1 X-coordinate of the character
 * @param y1 Y-coordinate of the character
 * @param z1 Z-coordinate (depth) of the character
 * @param a Alpha value for color blending
 * @param r Red component of the character color
 * @param g Green component of the character color
 * @param b Blue component of the character color
 * @param c ASCII value of the character to draw
 */
void draw_poly_char(float x1, float y1, float z1, float a, float r, float g, float b, int c) {
    pvr_vertex_t vert;
    int ix = (c % 16) * 16;
    int iy = (c / 16) * 24;
    float u1 = ix * 1.0f / 256.0f;
    float v1 = iy * 1.0f / 256.0f;
    float u2 = (ix + 12) * 1.0f / 256.0f;
    float v2 = (iy + 24) * 1.0f / 256.0f;

    // Define the four vertices of the character quad
    vert.flags = PVR_CMD_VERTEX;
    vert.x = x1;
    vert.y = y1 + 24.0f;
    vert.z = z1;
    vert.u = u1;
    vert.v = v2;
    vert.argb = PVR_PACK_COLOR(a, r, g, b);
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x1;
    vert.y = y1;
    vert.u = u1;
    vert.v = v1;
    pvr_prim(&vert, sizeof(vert));

    vert.x = x1 + 12.0f;
    vert.y = y1 + 24.0f;
    vert.u = u2;
    vert.v = v2;
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = x1 + 12.0f;
    vert.y = y1;
    vert.u = u2;
    vert.v = v1;
    pvr_prim(&vert, sizeof(vert));
}

// Buffer for storing formatted strings
static char strbuf[1024];

/**
 * @brief Draw a formatted string as textured polygons
 *
 * @param x1 Starting X-coordinate of the string
 * @param y1 Y-coordinate of the string
 * @param z1 Z-coordinate (depth) of the string
 * @param a Alpha value for color blending
 * @param r Red component of the text color
 * @param g Green component of the text color
 * @param b Blue component of the text color
 * @param fmt Format string (printf-style)
 * @param ... Variable arguments for the format string
 */
void draw_poly_strf(float x1, float y1, float z1, float a, float r,
                    float g, float b, char *fmt, ...) {
    va_list args;
    char *s;

    // Format the string
    va_start(args, fmt);
    vsprintf(strbuf, fmt, args);
    va_end(args);

    // Set up the texture for rendering
    pvr_prim(&util_txr_hdr, sizeof(util_txr_hdr));
    s = strbuf;

    // Render each character in the string
    while (*s) {
        if (*s == ' ') {
            x1 += 12.0f;
            s++;
        } else {
            draw_poly_char(x1, y1, z1, a, r, g, b, *s++);
            x1 += 12.0f;
        }
    }
}

/**
 * @brief Draw a shaded box as a polygon
 *
 * @param x1 Left X-coordinate of the box
 * @param y1 Top Y-coordinate of the box
 * @param x2 Right X-coordinate of the box
 * @param y2 Bottom Y-coordinate of the box
 * @param z Z-coordinate (depth) of the box
 * @param a1 Alpha value for the top-left corner
 * @param r1 Red component for the top-left corner
 * @param g1 Green component for the top-left corner
 * @param b1 Blue component for the top-left corner
 * @param a2 Alpha value for the bottom-right corner
 * @param r2 Red component for the bottom-right corner
 * @param g2 Green component for the bottom-right corner
 * @param b2 Blue component for the bottom-right corner
 */
void draw_poly_box(float x1, float y1, float x2, float y2, float z,
                   float a1, float r1, float g1, float b1,
                   float a2, float r2, float g2, float b2) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t poly;
    pvr_vertex_t vert;

    // Set up the polygon context and compile the header
    pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
    pvr_poly_compile(&poly, &cxt);
    pvr_prim(&poly, sizeof(poly));

    // Define the four vertices of the box
    vert.flags = PVR_CMD_VERTEX;
    vert.x = x1;
    vert.y = y2;
    vert.z = z;
    vert.argb = PVR_PACK_COLOR((a1 + a2) / 2, (r1 + r2) / 2, (g1 + g2) / 2, (b1 + b2) / 2);
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));

    vert.y = y1;
    vert.argb = PVR_PACK_COLOR(a1, r1, g1, b1);
    pvr_prim(&vert, sizeof(vert));

    vert.x = x2;
    vert.y = y2;
    vert.argb = PVR_PACK_COLOR(a2, r2, g2, b2);
    pvr_prim(&vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.y = y1;
    vert.argb = PVR_PACK_COLOR((a1 + a2) / 2, (r1 + r2) / 2, (g1 + g2) / 2, (b1 + b2) / 2);
    pvr_prim(&vert, sizeof(vert));
}

/* Draw a polygon for a shaded box - Version 2 */
void draw_poly_box_v2(float x1, float y1, float x2, float y2, float z,
                   float a1, float r1, float g1, float b1,
                   float a2, float r2, float g2, float b2) {
    pvr_poly_cxt_t  cxt;
    pvr_poly_hdr_t  poly;
    pvr_vertex_t    vert;
    
    pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
    pvr_poly_compile(&poly, &cxt);
    pvr_prim(&poly, sizeof(poly));
    
    vert.flags = PVR_CMD_VERTEX;
    vert.x = x1;
    vert.y = y2;
    vert.z = z;
    vert.argb = PVR_PACK_COLOR(
                    (a1 + a2) / 2,
                    (r1 + r2) / 2,
                    (g1 + g2) / 2,
                    (b1 + b2) / 2);
    vert.oargb = 0;
    pvr_prim(&vert, sizeof(vert));
    
    vert.y = y1;
    vert.argb = PVR_PACK_COLOR(a1, r1, g1, b1);
    pvr_prim(&vert, sizeof(vert));
    
    vert.x = x2;
    vert.y = y2;
    vert.argb = PVR_PACK_COLOR(a2, r2, g2, b2);
    pvr_prim(&vert, sizeof(vert));
    
    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.y = y1;
    vert.argb = PVR_PACK_COLOR(
                    (a1 + a2) / 2,
                    (r1 + r2) / 2,
                    (g1 + g2) / 2,
                    (b1 + b2) / 2);
    pvr_prim(&vert, sizeof(vert));
}

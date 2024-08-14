
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
#include <math.h>

/* texture.c */
extern pvr_ptr_t        util_texture;
extern pvr_poly_hdr_t       util_txr_hdr;
void setup_util_texture();
void draw_poly_char(float x1, float y1, float z1, float a, float r, float g, float b, int c);
void draw_poly_strf(float x1, float y1, float z1, float a, float r, float g, float b, char *fmt, ...);
void draw_poly_box(float x1, float y1, float x2, float y2, float z,
                   float a1, float r1, float g1, float b1,
                   float a2, float r2, float g2, float b2);
void draw_poly_box_v2(float x1, float y1, float x2, float y2, float z,
                   float a1, float r1, float g1, float b1,
                   float a2, float r2, float g2, float b2);                   
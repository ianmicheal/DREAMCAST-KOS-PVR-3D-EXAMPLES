
/********************************************************************************************/
/* KallistiOS Direct PVR API:2.0                                                            */
/********************************************************************************************/
/********************************************************************************************/
/* Name:     pngzoom .c                                                                     */
/* Title:    PNG ZOOM AUTO Kos Example                                                      */
/* Author:   (c)Ian Micheal                                                                 */
/* Created:   05/08/24                                                                      */
/*                                                                                          */
/* Version:  1.0                                                                            */
/* Platform: Dreamcast | KallistiOS:2.0 | KOSPVR |                                          */
/*                                                                                          */
/* Description:                                                                             */
/* The purpose of this example is to show pngzoom out then in and back out to a new image.  */
/* Very much like demo disk or loading TRANSITION                                           */
/* History: version 1                                                                       */
/********************************************************************************************/
/********************************************************************************************/
/*                                                                                          */
/********************************************************************************************/ 
#include <kos.h>        /*Includes necessary KallistiOS (KOS) headers for Dreamcast development */
#include <png/png.h>    /* PNG library headers for handling PNG images. */
#include <zlib/zlib.h>  /* Zlib library headers for data compression and decompression.*/
#include <stdlib.h>     /* standard library headers for general-purpose functions.*/
#include <assert.h>     /* assert library headers for runtime assertion checking.*/
#include <math.h>       /*Standard slower math library headers for mathematical function.*/
#include <stdbool.h>    /* boolean library headers for C99 boolean type.*/

// Declare the external ROM disk
extern uint8 romdisk_boot[];
KOS_INIT_ROMDISK(romdisk_boot);

// Declare global texture pointers
pvr_ptr_t back_tex_normal;
pvr_ptr_t back_tex_zoomed;
pvr_ptr_t current_tex; // Current texture used for drawing

// Initialize the zoom level
float zoom_level = 1.0f;

/**
 * @brief Initialize background textures.
 */
void back_init() {
    back_tex_normal = pvr_mem_malloc(512 * 512 * 2);
    png_to_texture("/rd/background_normal.png", back_tex_normal, PNG_FULL_ALPHA);

    back_tex_zoomed = pvr_mem_malloc(512 * 512 * 2);
    png_to_texture("/rd/background_zoomed.png", back_tex_zoomed, PNG_FULL_ALPHA);

    // Set the initial texture to normal
    current_tex = back_tex_normal;
}

/**
 * @brief Draw the background with a given zoom level.
 * @param zoom The zoom level to apply.
 */
void draw_back(float zoom) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t vert;

    // Set up the polygon context for the background texture
    pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444, 512, 512, current_tex, PVR_FILTER_BILINEAR);
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));

    vert.argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
    vert.oargb = 0;
    vert.flags = PVR_CMD_VERTEX;

    float width = 640 * zoom;
    float height = 480 * zoom;

    // Draw the textured quad with the specified zoom level
    vert.x = 320 - width / 2;
    vert.y = 240 - height / 2;
    vert.z = 1;
    vert.u = 0.0;
    vert.v = 0.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = 320 + width / 2;
    vert.y = 240 - height / 2;
    vert.u = 1.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = 320 - width / 2;
    vert.y = 240 + height / 2;
    vert.u = 0.0;
    vert.v = 1.0;
    pvr_prim(&vert, sizeof(vert));

    vert.x = 320 + width / 2;
    vert.y = 240 + height / 2;
    vert.u = 1.0;
    vert.v = 1.0;
    vert.flags = PVR_CMD_VERTEX_EOL;
    pvr_prim(&vert, sizeof(vert));
}

/**
 * @brief Adjust zoom level based on controller input.
 * @param state Pointer to the controller state structure.
 */
void zoom_in_out(cont_state_t *state) {
    if (state->buttons & CONT_A) {
        // Zoom out
        if (zoom_level > 0.0f) {
            zoom_level -= 0.1f;
        } else {
            zoom_level = 0.0f;
        }
    } else if (state->buttons & CONT_B) {
        // Zoom in
        if (zoom_level < 1.1f) {
            zoom_level += 0.1f;
        } else {
            zoom_level = 1.1f;
        }
    }

    // Determine which texture to use based on the zoom level
    if (zoom_level <= 1.0f) {
        current_tex = back_tex_normal;
    } else {
        current_tex = back_tex_zoomed;
    }
}

// PVR initialization parameters
pvr_init_params_t params = {
    { PVR_BINSIZE_16, PVR_BINSIZE_16, PVR_BINSIZE_16, PVR_BINSIZE_16, PVR_BINSIZE_16 },
    512*1024 // Vertex buffer size 512K
};
/********************************************************************************************/
/* main():                                                                                  */
/*      Entry point for C code.                                                              */
/********************************************************************************************/
/**
 * @brief Main function of the program.
 * @return Exit status.
 */
int main() {
    // Initialize the PVR system
    pvr_init(&params);
    vid_set_mode(DM_640x480_VGA, PM_RGB565);

    // Initialize background textures
    back_init();

    int done = 0; // Flag to exit the loop
    float preset_zoom = 2.0f; // Start with maximum zoom
    float current_zoom = preset_zoom;

    // Zoom-out phase
    while (current_zoom > 1.0f) {
        current_zoom -= 0.1f;

        pvr_wait_ready();
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_TR_POLY);
        draw_back(current_zoom);
        pvr_list_finish();
        pvr_scene_finish();

        thd_sleep(18); // Wait for a short delay to visualize the zoom effect
    }

    thd_sleep(1000); // Wait for 1 second

    // Simulate pressing 'A' to zoom to minimum
    while (zoom_level > 0.0f) {
        cont_state_t state;
        state.buttons = CONT_A;
        zoom_in_out(&state);

        pvr_wait_ready();
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_TR_POLY);
        draw_back(zoom_level);
        pvr_list_finish();
        pvr_scene_finish();

        thd_sleep(18); // Wait for a short delay to visualize the zoom effect
    }

    // Simulate pressing 'B' to zoom out to normal display
    while (zoom_level < 1.1f) {
        cont_state_t state;
        state.buttons = CONT_B;
        zoom_in_out(&state);

        pvr_wait_ready();
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_TR_POLY);
        draw_back(zoom_level);
        pvr_list_finish();
        pvr_scene_finish();

        thd_sleep(18); // Wait for a short delay to visualize the zoom effect
    }

    // Main loop - Normal operation
    while (!done) {
        maple_device_t *dev;
        int i = 0; // Index variable for maple_enum_type
        maple_device_t *first_dev = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
        while (first_dev != NULL) {
            dev = first_dev;
            cont_state_t *state = (cont_state_t *)maple_dev_status(dev);

            // Adjust zoom level based on button presses
            zoom_in_out(state);

            // Check if Start button is pressed
            if (state->buttons & CONT_START) {
                done = 1; // Exit loop
                goto out;
            }

            first_dev = maple_enum_type(++i, MAPLE_FUNC_CONTROLLER);
        }

        // Clear the screen
        pvr_wait_ready();
        pvr_scene_begin();
        pvr_list_begin(PVR_LIST_TR_POLY);
        draw_back(zoom_level);
        pvr_list_finish();
        pvr_scene_finish();
    }

out:
    // Clean up resources
    pvr_mem_free(back_tex_normal);
    pvr_mem_free(back_tex_zoomed);
    pvr_shutdown();
    vid_shutdown();

    return 0;
}

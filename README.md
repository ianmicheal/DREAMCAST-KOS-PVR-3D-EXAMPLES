# KallistiOS PVR API Only Examples
# DREAMCAST-KOS-PVR-3D-EXAMPLES
NO LAYERS DIRECT EXAMPLES
Welcome to the KallistiOS PVR API Only Example repository!
##Testing 
* All have been sent using BBA to real hardware, also using DreamSDK 3.0 dev
 * version GCC 4.7, my favorite compiler. If yours does not work, use printf to
 * find what it is getting confused by. I have videos of each running on hardware
 * and the ELF files you can turn into a CDI using BootDreams to run on a DC
 * emulator or a retail Dreamcast.
## About
## Examples
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
[![Watch the first video](http://img.youtube.com/vi/z0IkpKjiDQk/0.jpg)](http://www.youtube.com/watch?v=z0IkpKjiDQk)

[![Watch the second video](http://img.youtube.com/vi/S9obbHs4Hl8/0.jpg)](http://www.youtube.com/watch?v=S9obbHs4Hl8)

This repository contains examples demonstrating how to use the KallistiOS PVR API directly. It focuses solely on the PVR API, without using any additional layers such as OpenGL, SDL, or Parallax. These examples are intended to provide straightforward solutions for common tasks using the KallistiOS PVR API.

## Features

- **Direct KallistiOS PVR API Usage:** No additional layers, pure PVR API examples.
- **Texture Handling:** Loading and displaying textures.
- **Zoom Effects:** Implementing zoom-in and zoom-out effects controlled by controller inputs.
- **3D PVR Cube Example:** Demonstrating texture and matrix usage for 3D rendering.
- **Clear and Commented Code:** Easy to understand with comprehensive comments.

## Getting Started

### Prerequisites

To run these examples, you'll need:

- A Dreamcast development environment set up with KallistiOS.
- A compatible compiler (e.g., GCC).
- A Dreamcast console or emulator to test your builds or hardware serial or bba.

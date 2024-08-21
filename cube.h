#ifndef CUBE_H
#define CUBE_H

#include <stdint.h>
#include <dc/vec3f.h>

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

#define CUBEEXTENT 1.0f

static vec3f_t cube_vertices[8] __attribute__((aligned(32))) = {
    {.x = -CUBEEXTENT, .y = -CUBEEXTENT, .z = +CUBEEXTENT}, // 0
    {.x = -CUBEEXTENT, .y = +CUBEEXTENT, .z = +CUBEEXTENT}, // 1
    {.x = +CUBEEXTENT, .y = -CUBEEXTENT, .z = +CUBEEXTENT}, // 2
    {.x = +CUBEEXTENT, .y = +CUBEEXTENT, .z = +CUBEEXTENT}, // 3
    {.x = +CUBEEXTENT, .y = -CUBEEXTENT, .z = -CUBEEXTENT}, // 4
    {.x = +CUBEEXTENT, .y = +CUBEEXTENT, .z = -CUBEEXTENT}, // 5
    {.x = -CUBEEXTENT, .y = -CUBEEXTENT, .z = -CUBEEXTENT}, // 6
    {.x = -CUBEEXTENT, .y = +CUBEEXTENT, .z = -CUBEEXTENT}  // 7
};

const uint8_t cube_side_strips[6][4] __attribute__((aligned(32))) = {
    {0, 1, 2, 3}, // Front, 0->1->2 & 2->1->3
    {4, 5, 6, 7}, // Back, 4->5->6 & 6->5->7
    {6, 7, 0, 1}, // Left, 6->7->0 & 0->7->1
    {2, 3, 4, 5}, // Right, 2->3->4 & 4->3->5
    {1, 7, 3, 5}, // Top, 1->7->3 & 3->7->5
    {6, 0, 4, 2}  // Bottom, 6->0->4 & 4->0->2
};

const uint32_t cube_side_colors[6] __attribute__((aligned(32))) = {
    // format: 0xAARRGGBB
    0x7FF00000, // Red
    0x7F007F00, // Green
    0x7F00007F, // Blue
    0x7F7F7F00, // Yellow
    0x7F7F007F, // Cyan
    0x7F007F7F  // Magenta
};

struct cube {
  struct {
    float x, y, z;
  } pos;
  struct {
    float x, y;
  } rot;
  struct {
    float x, y;
  } speed;
  uint32_t grid_size;
} cube_state __attribute__((aligned(32))) = {0};

const float cube_tex_coords[4][2] = {
    {0, 0}, // left bottom
    {0, 1}, // left top
    {1, 0}, // right bottom
    {1, 1}, // right top
};

#endif // CUBE_H
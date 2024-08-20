#ifndef PERSPECTIVE_H
#define PERSPECTIVE_H

#include <dc/matrix.h> /* Matrix library headers for handling matrix operations */
#include <dc/matrix3d.h> /* Matrix3D library headers for handling 3D matrix operations */


#ifndef XSCALE
#define XSCALE 1.0f
#endif

matrix_t stored_projection_view __attribute__((aligned(32))) = {0};
void update_projection_view(float fovy) {
  mat_identity();
  float radians = fovy * F_PI / 180.0f;
  float cot_fovy_2 = 1.0f / ftan(radians * 0.5f);
  mat_perspective(XSCALE * 320.0f, 240.0f, cot_fovy_2, -10.f, +10.0f);

  point_t eye = {0.f, -0.00001f, 20.0f};
  point_t center = {0.f, 0.f, 0.f};
  vector_t up = {0.f, 0.f, 1.f};
  mat_lookat(&eye, &center, &up);
  mat_store(&stored_projection_view);
}
#endif // PERSPECTIVE_H
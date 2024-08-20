/********************************************************************************************/
/* KallistiOS Direct PVR API:2.0  Perlin 2d noise example                                   */
/********************************************************************************************/
/********************************************************************************************/
/* Name:     vector.h                                                                       */
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
/* Borrowed from CRTC Vector fast math file and example                                     */
/********************************************************************************************/


#ifndef VECTOR_H
#define VECTOR_H 1

#ifndef FLOAT_TYPE
#define FLOAT_TYPE float
#endif

#include <math.h>
#include <assert.h>

// Define DC_FAST_MATHS to enable fast math operations specific to Dreamcast hardware
#define DC_FAST_MATHS 1

// If DC_FAST_MATHS is defined, include the Dreamcast-specific fast math library
#ifdef DC_FAST_MATHS
#include <dc/fmath.h>
#endif

// Include the Dreamcast matrix operations library
#include <dc/matrix.h>

// Define M_PI if it's not already defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * @brief Calculate the dot product of two 3D vectors
 * 
 * @param a Pointer to the first vector
 * @param b Pointer to the second vector
 * @return FLOAT_TYPE The dot product of vectors a and b
 */
static inline FLOAT_TYPE
vec_dot (const FLOAT_TYPE *a, const FLOAT_TYPE *b)
{
#ifdef DC_FAST_MATHS
  // Use Dreamcast-specific FIPR (Floating-point Inner Product) instruction
  return fipr (a[0], a[1], a[2], 0.0, b[0], b[1], b[2], 0.0);
#else
  // Standard dot product calculation
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
#endif
}

/**
 * @brief Normalize a 3D vector
 * 
 * @param dst Pointer to the destination vector where the normalized result will be stored
 * @param in Pointer to the input vector to be normalized
 */
static inline void
vec_normalize (FLOAT_TYPE *dst, const FLOAT_TYPE *in)
{
#ifdef DC_FAST_MATHS
  // Use Dreamcast-specific fast math operations
  // Calculate the squared magnitude using FIPR
  FLOAT_TYPE mag = __fipr_magnitude_sqr (in[0], in[1], in[2], 0.0);
  // Use fast reciprocal square root
  FLOAT_TYPE rsqrt_mag = frsqrt (mag);
  // Multiply each component by the reciprocal square root of the magnitude
  dst[0] = in[0] * rsqrt_mag;
  dst[1] = in[1] * rsqrt_mag;
  dst[2] = in[2] * rsqrt_mag;
#else
  // Standard normalization procedure
  // Calculate the magnitude of the vector
  FLOAT_TYPE factor = sqrtf (in[0] * in[0] + in[1] * in[1] + in[2] * in[2]);
  
  // Avoid division by zero
  if (factor != 0.0)
    factor = 1.0 / factor;
  
  // Multiply each component by the reciprocal of the magnitude
  dst[0] = in[0] * factor;
  dst[1] = in[1] * factor;
  dst[2] = in[2] * factor;
#endif
}

/**
 * @brief Calculate the length (magnitude) of a 3D vector
 *
 * @param vec Pointer to the 3D vector
 * @return FLOAT_TYPE The length of the vector
 */
static inline FLOAT_TYPE
vec_length (const FLOAT_TYPE *vec)
{
#ifdef DC_FAST_MATHS
    // Use Dreamcast-specific fast math operations
    // fipr_magnitude_sqr calculates the squared magnitude using FIPR (Floating-point Inner Product)
    // fsqrt is a fast square root function
    return fsqrt(fipr_magnitude_sqr(vec[0], vec[1], vec[2], 0.0));
#else
    // Standard vector length calculation
    // Calculate the sum of squares and then take the square root
    return sqrtf(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
#endif
}

/**
 * @brief Define a 4x4 matrix aligned to a 32-byte boundary
 *
 * This matrix is initialized with all zeros. The alignment is important
 * for optimal memory access on the Dreamcast hardware.
 */
static matrix_t b_mat __attribute__((aligned(32))) =
{
    { 0.0, 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0, 0.0 }
};

/**
 * @brief Calculate the cross product of two 3D vectors
 *
 * @param dst Pointer to the destination vector where the result will be stored
 * @param a Pointer to the first input vector
 * @param b Pointer to the second input vector
 */
static inline void
vec_cross(FLOAT_TYPE *dst, const FLOAT_TYPE *a, const FLOAT_TYPE *b)
{
#ifdef DC_FAST_MATHS
    // Dreamcast-specific optimized version
    
    #if 0
    // This block is commented out, but shows an alternative method using FIPR
    FLOAT_TYPE tmpx = fipr(a[1], a[2], 0.0, 0.0, b[2], -b[1], 0.0, 0.0);
    FLOAT_TYPE tmpy = fipr(a[2], a[0], 0.0, 0.0, b[0], -b[2], 0.0, 0.0);
    FLOAT_TYPE tmpz = fipr(a[0], a[1], 0.0, 0.0, b[1], -b[0], 0.0, 0.0);
    dst[0] = tmpx;
    dst[1] = tmpy;
    dst[2] = tmpz;
    #else
    // Current implementation uses matrix transformation
    
    // Set up temporary variables for vector 'a'
    FLOAT_TYPE tmp1 = a[0], tmp2 = a[1], tmp3 = a[2], tmp4 = 0.0;
    
    // Set up the transformation matrix using vector 'b'
    // Note: The commented out lines are presumably zero and don't need to be set explicitly
    /*
    b_mat[0][0] = b_mat[3][0] = b_mat[1][1] = b_mat[3][1] = 0.0;
    b_mat[2][2] = b_mat[3][2] = b_mat[0][3] = b_mat[1][3] = 0.0;
    b_mat[2][3] = b_mat[3][3] = 0.0;
    */
    b_mat[1][0] = b[2];
    b_mat[2][0] = -b[1];
    b_mat[0][1] = -b[2];
    b_mat[2][1] = b[0];
    b_mat[0][2] = b[1];
    b_mat[1][2] = -b[0];
    
    // Load the matrix into the Dreamcast's matrix unit
    mat_load(&b_mat);
    
    // Perform matrix transformation
    mat_trans_nodiv(tmp1, tmp2, tmp3, tmp4);
    
    // Store the result
    dst[0] = tmp1;
    dst[1] = tmp2;
    dst[2] = tmp3;

    #endif

#else
    // Standard cross product calculation for non-Dreamcast platforms
    FLOAT_TYPE dstc[3];
    
    dstc[0] = a[1] * b[2] - a[2] * b[1];
    dstc[1] = a[2] * b[0] - a[0] * b[2];
    dstc[2] = a[0] * b[1] - a[1] * b[0];
    
    dst[0] = dstc[0];
    dst[1] = dstc[1];
    dst[2] = dstc[2];
#endif
}

/**
 * @brief Scale a 3D vector by a scalar value
 *
 * @param dst Pointer to the destination vector where the result will be stored
 * @param src Pointer to the source vector to be scaled
 * @param scale The scalar value to multiply the vector by
 */
static inline void
vec_scale(FLOAT_TYPE *dst, const FLOAT_TYPE *src, FLOAT_TYPE scale)
{
    dst[0] = src[0] * scale;  // Scale x component
    dst[1] = src[1] * scale;  // Scale y component
    dst[2] = src[2] * scale;  // Scale z component
}

/**
 * @brief Add two 3D vectors
 *
 * @param dst Pointer to the destination vector where the result will be stored
 * @param a Pointer to the first vector
 * @param b Pointer to the second vector
 */
static inline void
vec_add(FLOAT_TYPE *dst, const FLOAT_TYPE *a, const FLOAT_TYPE *b)
{
    dst[0] = a[0] + b[0];  // Add x components
    dst[1] = a[1] + b[1];  // Add y components
    dst[2] = a[2] + b[2];  // Add z components
}

/**
 * @brief Subtract one 3D vector from another
 *
 * @param dst Pointer to the destination vector where the result will be stored
 * @param a Pointer to the vector to subtract from
 * @param b Pointer to the vector to subtract
 */
static inline void
vec_sub(FLOAT_TYPE *dst, const FLOAT_TYPE *a, const FLOAT_TYPE *b)
{
    dst[0] = a[0] - b[0];  // Subtract x components
    dst[1] = a[1] - b[1];  // Subtract y components
    dst[2] = a[2] - b[2];  // Subtract z components
}

/**
 * @brief Interpolate between two 3D vectors
 *
 * This function performs linear interpolation between vectors 'a' and 'b'
 * based on the parameter 'param'. When param is 0, the result is 'a',
 * when param is 1, the result is 'b', and for values in between, it's
 * a weighted average of 'a' and 'b'.
 *
 * @param dst Pointer to the destination vector where the result will be stored
 * @param a Pointer to the starting vector
 * @param b Pointer to the ending vector
 * @param param Interpolation parameter (0.0 to 1.0)
 */
static inline void
vec_interpolate(FLOAT_TYPE *dst, const FLOAT_TYPE *a, const FLOAT_TYPE *b,
                FLOAT_TYPE param)
{
    FLOAT_TYPE tmp[3];
    vec_sub(&tmp[0], b, a);        // Calculate the difference vector (b - a)
    vec_scale(&tmp[0], &tmp[0], param);  // Scale the difference by param
    vec_add(dst, a, &tmp[0]);      // Add the scaled difference to 'a'
}

/**
 * @brief Interpolate between two 2D vectors
 *
 * This function performs linear interpolation between 2D vectors 'a' and 'b'
 * based on the parameter 'param'. When param is 0, the result is 'a',
 * when param is 1, the result is 'b', and for values in between, it's
 * a weighted average of 'a' and 'b'.
 *
 * @param dst Pointer to the destination vector where the result will be stored
 * @param a Pointer to the starting vector
 * @param b Pointer to the ending vector
 * @param param Interpolation parameter (0.0 to 1.0)
 */
static inline void
vec_interpolate2(FLOAT_TYPE *dst, const FLOAT_TYPE *a, const FLOAT_TYPE *b,
                 FLOAT_TYPE param)
{
    dst[0] = a[0] + param * (b[0] - a[0]);  // Interpolate x component
    dst[1] = a[1] + param * (b[1] - a[1]);  // Interpolate y component
}

/**
 * @brief Transform a vector by a 4x4 matrix using FIPR (Floating-point Inner Product) or standard math
 *
 * This function is designed for one-off transformations when it's not worth loading the matrix
 * into the hardware matrix unit (xmtrx).
 *
 * @param dst Pointer to the destination vector where the result will be stored
 * @param mat Pointer to the 4x4 transformation matrix (in column-major order)
 * @param src Pointer to the source vector to be transformed
 */
static inline void
vec_transform_fipr(FLOAT_TYPE *dst, const FLOAT_TYPE *mat,
                   const FLOAT_TYPE *src)
{
    assert(dst != src);  // Ensure dst and src are not the same to avoid aliasing issues

#ifdef DC_FAST_MATHS
    // Use Dreamcast-specific FIPR instruction for faster computation
    dst[0] = fipr(mat[0], mat[4], mat[8],  mat[12], src[0], src[1], src[2], src[3]);
    dst[1] = fipr(mat[1], mat[5], mat[9],  mat[13], src[0], src[1], src[2], src[3]);
    dst[2] = fipr(mat[2], mat[6], mat[10], mat[14], src[0], src[1], src[2], src[3]);
    dst[3] = fipr(mat[3], mat[7], mat[11], mat[15], src[0], src[1], src[2], src[3]);
#else
    // Standard matrix multiplication for non-Dreamcast platforms
    dst[0] = mat[0] * src[0] + mat[4] * src[1] + mat[8]  * src[2] + mat[12] * src[3];
    dst[1] = mat[1] * src[0] + mat[5] * src[1] + mat[9]  * src[2] + mat[13] * src[3];
    dst[2] = mat[2] * src[0] + mat[6] * src[1] + mat[10] * src[2] + mat[14] * src[3];
    dst[3] = mat[3] * src[0] + mat[7] * src[1] + mat[11] * src[2] + mat[15] * src[3];
#endif
}

/**
 * @brief Transform a 3D vector by a 4x4 matrix using FIPR (Floating-point Inner Product)
 *
 * This function is similar to vec_transform_fipr, but it's specifically for 3D vectors.
 * It treats the input vector as if it had a fourth component of 1.0 (homogeneous coordinates).
 *
 * @param dst Pointer to the destination 3D vector where the result will be stored
 * @param mat Pointer to the 4x4 transformation matrix (in column-major order)
 * @param src Pointer to the source 3D vector to be transformed
 */
static inline void
vec_transform3_fipr(FLOAT_TYPE *dst, const FLOAT_TYPE *mat,
                    const FLOAT_TYPE *src)
{
    // Transform x component
    dst[0] = fipr(mat[0], mat[4], mat[8],  mat[12],
                  src[0], src[1], src[2], 1.0f);
    // Transform y component
    dst[1] = fipr(mat[1], mat[5], mat[9],  mat[13],
                  src[0], src[1], src[2], 1.0f);
    // Transform z component
    dst[2] = fipr(mat[2], mat[6], mat[10], mat[14],
                  src[0], src[1], src[2], 1.0f);
    // Note: The w component (1.0f) is implicit and not stored in the result
}

/**
 * @brief Transform a vector by a 4x4 matrix (post-multiplication)
 *
 * This function performs a vector-matrix multiplication where the vector is
 * treated as a row vector and multiplied on the right side of the matrix.
 *
 * @param dst Pointer to the destination vector where the result will be stored
 * @param src Pointer to the source vector to be transformed
 * @param mat Pointer to the 4x4 transformation matrix (in row-major order)
 */
static inline void
vec_transform_post(FLOAT_TYPE *dst, const FLOAT_TYPE *src,
                   const FLOAT_TYPE *mat)
{
    assert(dst != src);  // Ensure dst and src are not the same to avoid aliasing issues

#ifdef DC_FAST_MATHS
    // Use Dreamcast-specific FIPR instruction for faster computation
    dst[0] = fipr(mat[0], mat[1], mat[2],  mat[3],  src[0], src[1], src[2], src[3]);
    dst[1] = fipr(mat[4], mat[5], mat[6],  mat[7],  src[0], src[1], src[2], src[3]);
    dst[2] = fipr(mat[8], mat[9], mat[10], mat[11], src[0], src[1], src[2], src[3]);
    dst[3] = fipr(mat[12], mat[13], mat[14], mat[15], src[0], src[1], src[2], src[3]);
#else
    // Standard matrix multiplication for non-Dreamcast platforms
    dst[0] = mat[0] * src[0] + mat[1] * src[1] + mat[2]  * src[2] + mat[3]  * src[3];
    dst[1] = mat[4] * src[0] + mat[5] * src[1] + mat[6]  * src[2] + mat[7]  * src[3];
    dst[2] = mat[8] * src[0] + mat[9] * src[1] + mat[10] * src[2] + mat[11] * src[3];
    dst[3] = mat[12] * src[0] + mat[13] * src[1] + mat[14] * src[2] + mat[15] * src[3];
#endif
}

/**
 * @brief Multiply two 4x4 matrices
 *
 * This function multiplies matrix 'a' by matrix 'b' and stores the result in 'dst'.
 * It uses vec_transform_fipr to multiply each column of 'b' by 'a'.
 *
 * @param dst Pointer to the destination matrix where the result will be stored
 * @param a Pointer to the first input matrix (left operand)
 * @param b Pointer to the second input matrix (right operand)
 */
static inline void
vec_mat_apply(FLOAT_TYPE *dst, const FLOAT_TYPE *a, const FLOAT_TYPE *b)
{
    // Multiply 'a' by the first column of 'b'
    vec_transform_fipr(&dst[0], a, &b[0]);
    // Multiply 'a' by the second column of 'b'
    vec_transform_fipr(&dst[4], a, &b[4]);
    // Multiply 'a' by the third column of 'b'
    vec_transform_fipr(&dst[8], a, &b[8]);
    // Multiply 'a' by the fourth column of 'b'
    vec_transform_fipr(&dst[12], a, &b[12]);
}

/**
 * @brief Extract the rotation component from a 4x4 transformation matrix
 *
 * This function creates a new 4x4 matrix that contains only the rotation
 * component of the input matrix, removing any translation or scaling.
 *
 * @param dst Pointer to the destination matrix where the result will be stored
 * @param src Pointer to the source transformation matrix
 */
static inline void
vec_extract_rotation(FLOAT_TYPE *dst, const FLOAT_TYPE *src)
{
    // Copy the upper-left 3x3 submatrix (rotation component)
    dst[0] = src[0];  dst[1] = src[1];  dst[2] = src[2];  dst[3] = 0.0;
    dst[4] = src[4];  dst[5] = src[5];  dst[6] = src[6];  dst[7] = 0.0;
    dst[8] = src[8];  dst[9] = src[9];  dst[10] = src[10]; dst[11] = 0.0;
    
    // Set the last row to [0, 0, 0, 1] to complete the rotation matrix
    dst[12] = 0.0;    dst[13] = 0.0;    dst[14] = 0.0;     dst[15] = 1.0;
}
/* Transpose the rotation part of (an orthogonal) matrix only.  */

/**
 * @brief Transpose the rotation component of a 4x4 transformation matrix
 *
 * This function creates a new 4x4 matrix that contains the transposed
 * rotation component of the input matrix. The translation components
 * are set to zero, and the bottom-right element is set to 1.
 *
 * @param dst Pointer to the destination matrix where the result will be stored
 * @param src Pointer to the source transformation matrix
 */
static inline void
vec_transpose_rotation(FLOAT_TYPE *dst, const FLOAT_TYPE *src)
{
    // Ensure that dst and src are not the same pointer to avoid aliasing issues
    assert(dst != src);
    
    // Transpose the upper-left 3x3 submatrix (rotation component)
    dst[0] = src[0];   // [0][0] remains the same
    dst[1] = src[4];   // [0][1] becomes [1][0]
    dst[2] = src[8];   // [0][2] becomes [2][0]
    dst[3] = 0.0;      // Clear any translation component
    
    dst[4] = src[1];   // [1][0] becomes [0][1]
    dst[5] = src[5];   // [1][1] remains the same
    dst[6] = src[9];   // [1][2] becomes [2][1]
    dst[7] = 0.0;      // Clear any translation component
    
    dst[8] = src[2];   // [2][0] becomes [0][2]
    dst[9] = src[6];   // [2][1] becomes [1][2]
    dst[10] = src[10]; // [2][2] remains the same
    dst[11] = 0.0;     // Clear any translation component
    
    // Set the last row to [0, 0, 0, 1] to complete the rotation matrix
    dst[12] = 0.0;
    dst[13] = 0.0;
    dst[14] = 0.0;
    dst[15] = 1.0;
}

/**
 * @brief Calculate the angle between two vectors
 *
 * This function computes the angle between vectors A and B, in the range [-PI, PI].
 * The result is as if A is rotated onto B, around the axis perpendicular to both.
 * If a non-NULL pointer is provided for AXB, it will store the cross product of A and B.
 *
 * @param a Pointer to the first vector
 * @param b Pointer to the second vector
 * @param axb Pointer to store the cross product of A and B (can be NULL if not needed)
 * @return FLOAT_TYPE The angle between vectors A and B in radians
 */
static inline FLOAT_TYPE
vec_angle(const FLOAT_TYPE *a, const FLOAT_TYPE *b, FLOAT_TYPE *axb)
{
    FLOAT_TYPE a_cross_b[3];
    FLOAT_TYPE *a_cross_b_p = &a_cross_b[0];
    FLOAT_TYPE res;
    
    // If axb is provided, use it to store the cross product
    if (axb)
        a_cross_b_p = axb;
    
    // Calculate the cross product of A and B
    vec_cross(a_cross_b_p, a, b);
    
    // Calculate the angle using the arcsine of the ratio of cross product magnitude
    // to the product of the magnitudes of A and B
    res = asinf(vec_length(a_cross_b_p) / (vec_length(a) * vec_length(b)));
    
    // Adjust the angle if the dot product of A and B is negative
    // This handles cases where the angle is greater than 90 degrees
    if (vec_dot(a, b) < 0)
    {
        if (res > 0)
            res = M_PI - res;  // Angle is in the range (90, 180] degrees
        else
            res = -M_PI - res; // Angle is in the range [-180, -90) degrees
    }
    
    return res;
}

/**
 * @brief Invert a 3x3 matrix within a 4x4 matrix structure
 *
 * This function calculates the inverse of the upper-left 3x3 submatrix of a 4x4 matrix.
 * The result is stored in a 4x4 matrix format with the last row and column set appropriately.
 *
 * @param dst Pointer to the destination 4x4 matrix where the result will be stored
 * @param src Pointer to the source 4x4 matrix containing the 3x3 submatrix to be inverted
 */
static inline void
vec_invertmat3(FLOAT_TYPE *dst, const FLOAT_TYPE *src)
{
    // Extract the elements of the 3x3 submatrix
    FLOAT_TYPE e11 = src[0],  e12 = src[4],  e13 = src[8];
    FLOAT_TYPE e21 = src[1],  e22 = src[5],  e23 = src[9];
    FLOAT_TYPE e31 = src[2],  e32 = src[6],  e33 = src[10];

    // Calculate the determinant of the 3x3 matrix
    FLOAT_TYPE det = e11 * e22 * e33 - e11 * e32 * e23 +
                     e21 * e32 * e13 - e21 * e12 * e33 +
                     e31 * e12 * e23 - e31 * e22 * e13;

    // Calculate the inverse of the determinant, handling the case where det = 0
    FLOAT_TYPE idet = (det == 0) ? 1.0 : 1.0 / det;

    // Calculate the elements of the inverted matrix
    dst[0]  =  (e22 * e33 - e23 * e32) * idet;  // New e11
    dst[4]  = -(e12 * e33 - e13 * e32) * idet;  // New e12
    dst[8]  =  (e12 * e23 - e13 * e22) * idet;  // New e13
    dst[12] = 0.0f;  // Last column, first row

    dst[1]  = -(e21 * e33 - e23 * e31) * idet;  // New e21
    dst[5]  =  (e11 * e33 - e13 * e31) * idet;  // New e22
    dst[9]  = -(e11 * e23 - e13 * e21) * idet;  // New e23
    dst[13] = 0.0f;  // Last column, second row

    dst[2]  =  (e21 * e32 - e22 * e31) * idet;  // New e31
    dst[6]  = -(e11 * e32 - e12 * e31) * idet;  // New e32
    dst[10] =  (e11 * e22 - e12 * e21) * idet;  // New e33
    dst[14] = 0.0f;  // Last column, third row

    // Set the last row to [0, 0, 0, 1]
    dst[3]  = 0.0f;
    dst[7]  = 0.0f;
    dst[11] = 0.0f;
    dst[15] = 1.0f;
}

/**
 * @brief Calculate the normal transformation matrix from a modelview matrix
 *
 * This function computes the inverse transpose of the rotation part of the modelview matrix,
 * which is suitable for transforming normal vectors. It also normalizes the resulting
 * column vectors to ensure unit length.
 *
 * @param dst Pointer to the destination 4x4 matrix where the result will be stored
 * @param src Pointer to the source 4x4 modelview matrix
 */
static inline void
vec_normal_from_modelview(FLOAT_TYPE *dst, const FLOAT_TYPE *src)
{
    FLOAT_TYPE tmp[16];

    // Invert the 3x3 rotation part of the modelview matrix
    vec_invertmat3(tmp, src);

    // Transpose the inverted matrix while copying it to the destination
    // This effectively creates the inverse transpose of the rotation part
    dst[0] = tmp[0];   dst[1] = tmp[4];   dst[2] = tmp[8];   dst[3] = 0.0f;
    dst[4] = tmp[1];   dst[5] = tmp[5];   dst[6] = tmp[9];   dst[7] = 0.0f;
    dst[8] = tmp[2];   dst[9] = tmp[6];   dst[10] = tmp[10]; dst[11] = 0.0f;

    // Set the last row to [0, 0, 0, 1]
    dst[12] = 0.0f;    dst[13] = 0.0f;    dst[14] = 0.0f;    dst[15] = 1.0f;

    // Normalize each column vector of the 3x3 part
    vec_normalize(&dst[0], &dst[0]);  // Normalize first column
    vec_normalize(&dst[4], &dst[4]);  // Normalize second column
    vec_normalize(&dst[8], &dst[8]);  // Normalize third column
}
#endif

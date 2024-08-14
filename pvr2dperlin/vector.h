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

#define DC_FAST_MATHS 1

#ifdef DC_FAST_MATHS
#include <dc/fmath.h>
#endif

#include <dc/matrix.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static __inline__ FLOAT_TYPE
vec_dot (const FLOAT_TYPE *a, const FLOAT_TYPE *b)
{
#ifdef DC_FAST_MATHS
  return fipr (a[0], a[1], a[2], 0.0, b[0], b[1], b[2], 0.0);
#else
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
#endif
}

static __inline__ void
vec_normalize (FLOAT_TYPE *dst, const FLOAT_TYPE *in)
{
#ifdef DC_FAST_MATHS
  FLOAT_TYPE mag = __fipr_magnitude_sqr (in[0], in[1], in[2], 0.0);
  FLOAT_TYPE rsqrt_mag = frsqrt (mag);
  dst[0] = in[0] * rsqrt_mag;
  dst[1] = in[1] * rsqrt_mag;
  dst[2] = in[2] * rsqrt_mag;
#else
  FLOAT_TYPE factor = sqrtf (in[0] * in[0] + in[1] * in[1] + in[2] * in[2]);
  
  if (factor != 0.0)
    factor = 1.0 / factor;
  
  dst[0] = in[0] * factor;
  dst[1] = in[1] * factor;
  dst[2] = in[2] * factor;
#endif
}

static __inline__ FLOAT_TYPE
vec_length (const FLOAT_TYPE *vec)
{
#ifdef DC_FAST_MATHS
  return fsqrt (fipr_magnitude_sqr (vec[0], vec[1], vec[2], 0.0));
#else
  return sqrtf (vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
#endif
}

static matrix_t b_mat __attribute__((aligned(32))) =
  {
    { 0.0, 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0, 0.0 }
  };

static __inline__ void
vec_cross (FLOAT_TYPE *dst, const FLOAT_TYPE *a, const FLOAT_TYPE *b)
{
#ifdef DC_FAST_MATHS
    
#if 0
  FLOAT_TYPE tmpx = fipr (a[1], a[2], 0.0, 0.0, b[2], -b[1], 0.0, 0.0);
  FLOAT_TYPE tmpy = fipr (a[2], a[0], 0.0, 0.0, b[0], -b[2], 0.0, 0.0);
  FLOAT_TYPE tmpz = fipr (a[0], a[1], 0.0, 0.0, b[1], -b[0], 0.0, 0.0);
  dst[0] = tmpx;
  dst[1] = tmpy;
  dst[2] = tmpz;
#else
  /*** Alternatively, we can do a ftrv like:
  
  [  0    b[2] -b[1] 0 ] [ a[0] ]
  [-b[2]   0    b[0] 0 ] [ a[1] ]
  [ b[1] -b[0]   0   0 ] [ a[2] ]
  [  0     0     0   0 ] [  0   ]
  
  ***/

  FLOAT_TYPE tmp1 = a[0], tmp2 = a[1], tmp3 = a[2], tmp4 = 0.0;
  
  /*b_mat[0][0] = b_mat[3][0] = b_mat[1][1] = b_mat[3][1] = 0.0;
  b_mat[2][2] = b_mat[3][2] = b_mat[0][3] = b_mat[1][3] = 0.0;
  b_mat[2][3] = b_mat[3][3] = 0.0;*/
  b_mat[1][0] = b[2];
  b_mat[2][0] = -b[1];
  b_mat[0][1] = -b[2];
  b_mat[2][1] = b[0];
  b_mat[0][2] = b[1];
  b_mat[1][2] = -b[0];
  mat_load (&b_mat);
  
  mat_trans_nodiv (tmp1, tmp2, tmp3, tmp4);
  dst[0] = tmp1;
  dst[1] = tmp2;
  dst[2] = tmp3;
//dirty
#endif
#else
  FLOAT_TYPE dstc[3];
  
  dstc[0] = a[1] * b[2] - a[2] * b[1];
  dstc[1] = a[2] * b[0] - a[0] * b[2];
  dstc[2] = a[0] * b[1] - a[1] * b[0];
  dst[0] = dstc[0];
  dst[1] = dstc[1];
  dst[2] = dstc[2];
#endif
}

static __inline__ void
vec_scale (FLOAT_TYPE *dst, const FLOAT_TYPE *src, FLOAT_TYPE scale)
{
  dst[0] = src[0] * scale;
  dst[1] = src[1] * scale;
  dst[2] = src[2] * scale;
}

static __inline__ void
vec_add (FLOAT_TYPE *dst, const FLOAT_TYPE *a, const FLOAT_TYPE *b)
{
  dst[0] = a[0] + b[0];
  dst[1] = a[1] + b[1];
  dst[2] = a[2] + b[2];
}

static __inline__ void
vec_sub (FLOAT_TYPE *dst, const FLOAT_TYPE *a, const FLOAT_TYPE *b)
{
  dst[0] = a[0] - b[0];
  dst[1] = a[1] - b[1];
  dst[2] = a[2] - b[2];
}

static __inline__ void
vec_interpolate (FLOAT_TYPE *dst, const FLOAT_TYPE *a, const FLOAT_TYPE *b,
		 FLOAT_TYPE param)
{
  FLOAT_TYPE tmp[3];
  vec_sub (&tmp[0], b, a);
  vec_scale (&tmp[0], &tmp[0], param);
  vec_add (dst, a, &tmp[0]);
}

static __inline__ void
vec_interpolate2 (FLOAT_TYPE *dst, const FLOAT_TYPE *a, const FLOAT_TYPE *b,
		  FLOAT_TYPE param)
{
  dst[0] = a[0] + param * (b[0] - a[0]);
  dst[1] = a[1] + param * (b[1] - a[1]);
}

/* Use this for one-off transformations, when it's not worth loading xmtrx.  */
static __inline__ void
vec_transform_fipr (FLOAT_TYPE *dst, const FLOAT_TYPE *mat,
		    const FLOAT_TYPE *src)
{
  assert (dst != src);

#ifdef DC_FAST_MATHS
  dst[0] = fipr (mat[0], mat[4], mat[8], mat[12],
		 src[0], src[1], src[2], src[3]);
  dst[1] = fipr (mat[1], mat[5], mat[9], mat[13],
		 src[0], src[1], src[2], src[3]);
  dst[2] = fipr (mat[2], mat[6], mat[10], mat[14],
		 src[0], src[1], src[2], src[3]);
  dst[3] = fipr (mat[3], mat[7], mat[11], mat[15],
		 src[0], src[1], src[2], src[3]);
#else
  dst[0] = mat[0] * src[0]
           + mat[4] * src[1]
	   + mat[8] * src[2] 
	   + mat[12] * src[3];
  dst[1] = mat[1] * src[0]
	   + mat[5] * src[1]
	   + mat[9] * src[2]
	   + mat[13] * src[3];
  dst[2] = mat[2] * src[0]
	   + mat[6] * src[1]
	   + mat[10] * src[2]
           + mat[14] * src[3];
  dst[3] = mat[3] * src[0]
	   + mat[7] * src[1]
	   + mat[11] * src[2]
           + mat[15] * src[3];
#endif
}

/* Same, but 3-element src & dest vectors.  */
static __inline__ void
vec_transform3_fipr (FLOAT_TYPE *dst, const FLOAT_TYPE *mat,
		     const FLOAT_TYPE *src)
{
  dst[0] = fipr (mat[0], mat[4], mat[8], mat[12],
		 src[0], src[1], src[2], 1.0f);
  dst[1] = fipr (mat[1], mat[5], mat[9], mat[13],
		 src[0], src[1], src[2], 1.0f);
  dst[2] = fipr (mat[2], mat[6], mat[10], mat[14],
		 src[0], src[1], src[2], 1.0f);
}

static __inline__ void
vec_transform_post (FLOAT_TYPE *dst, const FLOAT_TYPE *src,
		    const FLOAT_TYPE *mat)
{
  assert (dst != src);

#ifdef DC_FAST_MATHS
  dst[0] = fipr (mat[0], mat[1], mat[2], mat[3],
		 src[0], src[1], src[2], src[3]);
  dst[1] = fipr (mat[4], mat[5], mat[6], mat[7],
		 src[0], src[1], src[2], src[3]);
  dst[2] = fipr (mat[8], mat[9], mat[10], mat[11],
		 src[0], src[1], src[2], src[3]);
  dst[3] = fipr (mat[12], mat[13], mat[14], mat[15],
		 src[0], src[1], src[2], src[3]);
#else
  dst[0] = mat[0] * src[0]
           + mat[1] * src[1]
	   + mat[2] * src[2] 
	   + mat[3] * src[3];
  dst[1] = mat[4] * src[0]
	   + mat[5] * src[1]
	   + mat[6] * src[2]
	   + mat[7] * src[3];
  dst[2] = mat[8] * src[0]
	   + mat[9] * src[1]
	   + mat[10] * src[2]
           + mat[11] * src[3];
  dst[3] = mat[12] * src[0]
	   + mat[13] * src[1]
	   + mat[14] * src[2]
           + mat[15] * src[3];
#endif
}

static __inline__ void
vec_mat_apply (FLOAT_TYPE *dst, const FLOAT_TYPE *a, const FLOAT_TYPE *b)
{
  vec_transform_fipr (&dst[0], a, &b[0]);
  vec_transform_fipr (&dst[4], a, &b[4]);
  vec_transform_fipr (&dst[8], a, &b[8]);
  vec_transform_fipr (&dst[12], a, &b[12]);
}

static __inline__ void
vec_extract_rotation (FLOAT_TYPE *dst, const FLOAT_TYPE *src)
{
  dst[0] = src[0];
  dst[1] = src[1];
  dst[2] = src[2];
  dst[3] = 0.0;
  
  dst[4] = src[4];
  dst[5] = src[5];
  dst[6] = src[6];
  dst[7] = 0.0;
  
  dst[8] = src[8];
  dst[9] = src[9];
  dst[10] = src[10];
  dst[11] = 0.0;
  
  dst[12] = 0.0;
  dst[13] = 0.0;
  dst[14] = 0.0;
  dst[15] = 1.0;
}

/* Transpose the rotation part of (an orthogonal) matrix only.  */

static __inline__ void
vec_transpose_rotation (FLOAT_TYPE *dst, const FLOAT_TYPE *src)
{
  assert (dst != src);
  
  dst[0] = src[0];
  dst[1] = src[4];
  dst[2] = src[8];
  dst[3] = 0.0;
  
  dst[4] = src[1];
  dst[5] = src[5];
  dst[6] = src[9];
  dst[7] = 0.0;
  
  dst[8] = src[2];
  dst[9] = src[6];
  dst[10] = src[10];
  dst[11] = 0.0;
  
  dst[12] = 0.0;
  dst[13] = 0.0;
  dst[14] = 0.0;
  dst[15] = 1.0;
}

/* The angle between two vectors, between -PI and PI.  Result is as if
   A is rotated onto B, around the axis perpendicular to both.  If you want
   to know the axis too, pass non-NULL for AXB.  */

static __inline__ FLOAT_TYPE
vec_angle (const FLOAT_TYPE *a, const FLOAT_TYPE *b, FLOAT_TYPE *axb)
{
  FLOAT_TYPE a_cross_b[3];
  FLOAT_TYPE *a_cross_b_p = &a_cross_b[0];
  FLOAT_TYPE res;
  
  if (axb)
    a_cross_b_p = axb;
  
  vec_cross (a_cross_b_p, a, b);
  res = asinf (vec_length (a_cross_b_p) / (vec_length (a) * vec_length (b)));
  
  if (vec_dot (a, b) < 0)
    {
      if (res > 0)
        res = M_PI - res;
      else
        res = -M_PI - res;
    }

  return res;
}

static __inline__ void
vec_invertmat3 (FLOAT_TYPE *dst, const FLOAT_TYPE *src)
{
  FLOAT_TYPE e11 = src[0], e12 = src[4], e13 = src[8];
  FLOAT_TYPE e21 = src[1], e22 = src[5], e23 = src[9];
  FLOAT_TYPE e31 = src[2], e32 = src[6], e33 = src[10];
  FLOAT_TYPE det = e11 * e22 * e33 - e11 * e32 * e23 +
		   e21 * e32 * e13 - e21 * e12 * e33 +
		   e31 * e12 * e23 - e31 * e22 * e13;
  FLOAT_TYPE idet = (det == 0) ? 1.0 : 1.0 / det;

  dst[0]  =  (e22 * e33 - e23 * e32) * idet;
  dst[4]  = -(e12 * e33 - e13 * e32) * idet;
  dst[8]  =  (e12 * e23 - e13 * e22) * idet;
  dst[12] = 0.0f;

  dst[1]  = -(e21 * e33 - e23 * e31) * idet;
  dst[5]  =  (e11 * e33 - e13 * e31) * idet;
  dst[9]  = -(e11 * e23 - e13 * e21) * idet;
  dst[13] = 0.0f;

  dst[2]  =  (e21 * e32 - e22 * e31) * idet;
  dst[6]  = -(e11 * e32 - e12 * e31) * idet;
  dst[10] =  (e11 * e22 - e12 * e21) * idet;
  dst[14] = 0.0f;
  
  dst[3]  = 0.0f;
  dst[7]  = 0.0f;
  dst[11] = 0.0f;
  dst[15] = 1.0f;
}

/* This is the inverse of the transpose of the rotation part of the modelview
   matrix, suitable for transforming normals.  */

static __inline__ void
vec_normal_from_modelview (FLOAT_TYPE *dst, const FLOAT_TYPE *src)
{
  FLOAT_TYPE tmp[16];

  vec_invertmat3 (tmp, src);

  dst[0] = tmp[0];
  dst[1] = tmp[4];
  dst[2] = tmp[8];
  dst[3] = 0.0f;
  
  dst[4] = tmp[1];
  dst[5] = tmp[5];
  dst[6] = tmp[9];
  dst[7] = 0.0f;
  
  dst[8] = tmp[2];
  dst[9] = tmp[6];
  dst[10] = tmp[10];
  dst[11] = 0.0f;
  
  dst[12] = 0.0f;
  dst[13] = 0.0f;
  dst[14] = 0.0f;
  dst[15] = 1.0f;
  
  vec_normalize (&dst[0], &dst[0]);
  vec_normalize (&dst[4], &dst[4]);
  vec_normalize (&dst[8], &dst[8]);
}

#endif

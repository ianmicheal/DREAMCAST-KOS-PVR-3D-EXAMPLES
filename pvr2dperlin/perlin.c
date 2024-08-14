

/********************************************************************************************/
/* KallistiOS Direct PVR API:2.0  Perlin 2d noise example                                   */
/********************************************************************************************/
/********************************************************************************************/
/* Name:     perlin.c                                                                       */
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
#include "perlin.h"
#include "vector.h"


#ifndef FLOAT_TYPE
#define FLOAT_TYPE float
#endif

/**
 * @brief Generate a hash value for given coordinates
 * 
 * This function creates a pseudo-random value based on the input coordinates.
 * It uses bitwise operations and prime numbers to generate a seemingly random,
 * but deterministic output for any given input.
 *
 * @param x The x-coordinate
 * @param y The y-coordinate
 * @return A float value between -1 and 1
 */
static __inline__ float hash(int x, int y)
{
    x += y * 1471;  // Combine x and y using a prime number
    x = (x << 13) ^ x;  // Bit shifting and XOR for more randomness
    // Complex mathematical operation to generate a pseudo-random value
    return (1.0f - ((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

/**
 * @brief Smooth the noise at given coordinates
 * 
 * This function creates a smoothed noise value by sampling the surrounding
 * points and performing a weighted average using fast inner product (fipr).
 *
 * @param x The x-coordinate
 * @param y The y-coordinate
 * @return A smoothed float value
 */
static __inline__ float smooth(int x, int y)
{
    FLOAT_TYPE hash_values[4];
    
    // Calculate hash values for the four corners
    hash_values[0] = hash(x - 1, y - 1);
    hash_values[1] = hash(x + 1, y - 1);
    hash_values[2] = hash(x - 1, y + 1);
    hash_values[3] = hash(x + 1, y + 1);

    // Use fast inner product (fipr) to calculate weighted sum of corner values
    FLOAT_TYPE result = fipr(hash_values[0], hash_values[1], hash_values[2], hash_values[3],
                             1.0f, 1.0f, 1.0f, 1.0f);

    // Calculate hash values for the four edges
    hash_values[0] = hash(x - 1, y);
    hash_values[1] = hash(x + 1, y);
    hash_values[2] = hash(x, y - 1);
    hash_values[3] = hash(x, y + 1);

    // Add weighted sum of edge values (with double weight)
    result += fipr(hash_values[0], hash_values[1], hash_values[2], hash_values[3],
                   2.0f, 2.0f, 2.0f, 2.0f);

    // Add center value with quadruple weight
    result += hash(x, y) * 4.0f;

    // Normalize the result
    return result / 16.0f;
}

/**
 * @brief Perform cubic interpolation
 * 
 * This function interpolates between four values using a cubic curve.
 *
 * @param v0, v1, v2, v3 The four values to interpolate between
 * @param x The interpolation factor (0 to 1)
 * @return The interpolated value
 */
static __inline__ float cubic(float v0, float v1, float v2, float v3, float x)
{
    FLOAT_TYPE p = (v3 - v2) - (v0 - v1);
    FLOAT_TYPE q = (v0 - v1) - p;
    FLOAT_TYPE r = v2 - v0;
    FLOAT_TYPE s = v1;
    
    // Calculate the cubic interpolation
    s += r * x;
    s += q * x * x;
    s += p * x * x * x;
    
    return s;
}

/**
 * @brief Perform 2D cubic interpolation
 * 
 * This function creates a smoothed 2D noise value using cubic interpolation.
 *
 * @param x The x-coordinate
 * @param y The y-coordinate
 * @return A smoothly interpolated noise value
 */
static __inline__ float cubicxy(float x, float y)
{
    int ix = (int)x, iy = (int)y;
    float fx = x - ix, fy = y - iy;

    FLOAT_TYPE hash_values[16];
    
    // Calculate hash values for a 4x4 grid around the point
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            hash_values[i * 4 + j] = hash(ix + i - 1, iy + j - 1);
        }
    }

    // Perform bicubic interpolation
    return cubic(
        cubic(hash_values[0], hash_values[1], hash_values[2], hash_values[3], fy),
        cubic(hash_values[4], hash_values[5], hash_values[6], hash_values[7], fy),
        cubic(hash_values[8], hash_values[9], hash_values[10], hash_values[11], fy),
        cubic(hash_values[12], hash_values[13], hash_values[14], hash_values[15], fy),
        fx);
}

/**
 * @brief Generate 2D Perlin noise
 * 
 * This function creates Perlin noise by combining multiple octaves of noise.
 * Each octave adds detail to the noise pattern.
 *
 * @param x The x-coordinate
 * @param y The y-coordinate
 * @param octaves The number of octaves to combine
 * @return The final Perlin noise value
 */
float perlin_noise_2D(float x, float y, int octaves)
{
    float result = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    
    for (int i = 0; i < octaves; i++)
    {
        result += smooth(x * frequency, y * frequency) * amplitude;
    }
    
    return result;
}

/**
 * @brief Generate a fractal Brownian motion (fBm) noise value
 * 
 * This function creates fBm noise by combining multiple octaves of noise,
 * similar to perlin_noise_2D but with additional control parameters.
 *
 * @param x The x-coordinate
 * @param y The y-coordinate
 * @param octaves The number of octaves to combine
 * @param lacunarity Frequency multiplier between octaves
 * @param gain Amplitude multiplier between octaves
 * @return The fBm noise value
 */
float fbm_noise_2D(float x, float y, int octaves, float lacunarity, float gain)
{
    float result = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
  
    for (int i = 0; i < octaves; i++)
    {
        result += smooth(x * frequency, y * frequency) * amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }
  
    return result;
}

#ifndef PERLIN_H
#define PERLIN_H 1

extern float perlin_noise_2D(float x, float y, int octaves);

// New fractal Brownian motion (fBm) noise function
extern float fbm_noise_2D(float x, float y, int octaves, float lacunarity, float gain);

#endif

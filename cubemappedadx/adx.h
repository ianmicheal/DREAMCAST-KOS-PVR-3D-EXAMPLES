/* LibADX for KallistiOS ##version##
 *
 * ADX Decoder Library
 * 
 * Copyright (C) 2011-2013 Josh 'PH3NOM' Pearson
 * Copyright (C) 2024 The KOS Team and contributors
 *
 * This code was contributed to KallistiOS (KOS) by Mickaël Cardoso (SiZiOUS).
 * It was originally made by Josh Pearson (PH3NOM). Some portions of code were
 * made by BERO. Sightly improved by Headshotnoby and Ian Micheal.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ADX_H
#define ADX_H

#ifdef __cplusplus
extern "C" {
#endif

#define ADX_CRI_SIZE 0x06
#define ADX_PAD_SIZE 0x0e
#define ADX_HDR_SIZE 0x2c
#define ADX_HDR_SIG  0x80
#define ADX_EXIT_SIG 0x8001

#define ADX_ADDR_START      0x02
#define ADX_ADDR_CHUNK      0x05
#define ADX_ADDR_CHAN       0x07
#define ADX_ADDR_RATE       0x08
#define ADX_ADDR_SAMP       0x0c
#define ADX_ADDR_TYPE       0x12
#define ADX_ADDR_LOOP       0x18
#define ADX_ADDR_SAMP_START 0x1c
#define ADX_ADDR_BYTE_START 0x20
#define ADX_ADDR_SAMP_END   0x24
#define ADX_ADDR_BYTE_END   0x28

typedef struct
{
    int sample_offset;              
    int chunk_size;
    int channels;
    int rate;
    int samples;
    int loop_type;
    int loop;
    int loop_start;
    int loop_end;
    int loop_samp_start;
    int loop_samp_end;
    int loop_samples;
}ADX_INFO;

typedef struct {
	int s1,s2;
} PREV;

/* LibADX Public Function Definitions */
/* Return 1 on success, 0 on failure */

/* Start Straming the ADX in a seperate thread */
int adx_dec( const char * adx_file, int loop_enable );

/* Stop Streaming the ADX */
int adx_stop();

/* Restart Streaming the ADX */
int adx_restart();

/* Pause Streaming the ADX (if streaming) */
int adx_pause();

/* Resume Streaming the ADX (if paused) */
int adx_resume();

#ifdef __cplusplus
};
#endif

#endif

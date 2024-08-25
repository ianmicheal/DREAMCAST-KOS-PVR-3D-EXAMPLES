#/********************************************************************************************/
#/* KallistiOS Direct PVR API:2.0  PVR TEXTURE CUBE With perlin textures                     */
#/********************************************************************************************/
#/********************************************************************************************/
#/* KallistiOS Direct PVR API:2.0  PVR TEXTURE CUBE WITH 6 TEXTURES: ZOOM AND ROTATION, V2.0 */
#/********************************************************************************************/
#/********************************************************************************************/
#/* Name:     cube6.c                                                                        */
#/* Title:    PVR TEXTURE CUBE With perlin textures, V2.0 Kos Example                        */
#/* Author:   (c)Ian Micheal                                                                 */
#/* Created:  08/05/24                                                                       */
#/*                                                                                          */
#/* Version:  2.0                                                                            */
#/* Platform: Dreamcast | KallistiOS:2.0 | KOSPVR |                                          */
#/*                                                                                          */
#/* Description: 6 Textures one each cube face                                               */
#/* The purpose of this example is to show the use of only the KOSPVR API to do 3D           */
#/* And commented so anyone who knows OpenGL can use the DIRECT NO LAYER KOSPVR API.         */
#/* History: version 2 Demonstrating procedural texture generation                           */
#/********************************************************************************************/
#/********************************************************************************************/
#/*        >>>  Help and code examples and advice these people where invaluable  <<<         */
#/*     Mvp's:  dRxL with my_perspective_mat_lh and explaining to me the concepts            */
#/*     Mvp's:  Bruce tested and found both annoying bugs and texture distortion.            */
#/*                                                                                          */
#/********************************************************************************************/ 

KOS_CFLAGS+= -g -std=c99 
TARGET = pvrcube.elf
OBJS =  perlin.o 6cube2.o 

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS) romdisk.*
rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS) romdisk.o
	kos-c++ -o $(TARGET) $(OBJS)romdisk.o -lADX -lpng -ljpeg -lkmg -lz -lkosutils -lm

romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	rm -f $(OBJS) romdisk.o romdisk.img
	$(KOS_STRIP) $(TARGET)

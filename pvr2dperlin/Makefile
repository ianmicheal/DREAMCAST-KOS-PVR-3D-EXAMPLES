#/********************************************************************************************/
#/* KallistiOS Direct PVR API:2.0  Perlin 2d noise example                                   */
#/********************************************************************************************/
#/********************************************************************************************/
#/*                                                                                          */
#/********************************************************************************************/
#/********************************************************************************************/
#/* Name:     main.c                                                                         */
#/* Title:    Perlin 2d noise example, V1.0 Kos Example                                      */
#/* Author:   (c)Ian Micheal                                                                 */
#/* Created:  08/05/24                                                                       */
#/*                                                                                          */
#/* Version:  1.0                                                                            */
#/* Platform: Dreamcast | KallistiOS:2.0 | KOSPVR | Perlin 2d noise example                  */
#/*                                                                                          */
#/* Description: perlin 2d noise example                                                     */
#/* The purpose of this example is to show the use of only the KOSPVR API to do 3D           */
#/* And commented so anyone who knows OpenGL can use the DIRECT NO LAYER KOSPVR API.         */
#/* History: version 2                                                                       */
#/********************************************************************************************/
#/********************************************************************************************/
#/*        >>>  Help and code examples and advice these people where invaluable  <<<         */
#/*     Mvp's:  dRxL with my_perspective_mat_lh and explaining to me the concepts            */
#/*     Mvp's:  Bruce tested and found both annoying bugs and texture distortion.            */
#/*                                                                                          */
#/********************************************************************************************/ 

KOS_CFLAGS+= -g -std=c99 
TARGET = perlin2d.elf
OBJS = perlin.o fontnew.o main.o  

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS) romdisk.*
rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS) romdisk.o
	kos-c++ -o $(TARGET) $(OBJS)romdisk.o -lpng -ljpeg -lkmg -lz -lkosutils -lm

romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

dist:
	rm -f $(OBJS) romdisk.o romdisk.img
	$(KOS_STRIP) $(TARGET)

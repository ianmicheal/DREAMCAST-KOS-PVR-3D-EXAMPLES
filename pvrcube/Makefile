#/********************************************************************************************/
#/* KallistiOS Direct PVR API:2.0  PVR TEXTURE CUBE WITH ZOOM AND ROTATION  V1               */
#/********************************************************************************************/
#/********************************************************************************************/
#/* Name:     pngzoom .c                                                                     */
#/* Title:    PVR TEXTURE CUBE WITH ZOOM AND ROTATION Kos Example                            */
#/* Author:   (c)Ian Micheal                                                                 */
#/* Created:   05/08/24                                                                      */
#/*                                                                                          */
#/* Version:  1.0                                                                            */
#/* Platform: Dreamcast | KallistiOS:2.0 | KOSPVR |                                          */
#/*                                                                                          */
#/* Description:                                                                             */
#/* The purpose of this example is to show the use of only the KOSPVR API to do 3D matching  */
#/* And commented so anyone that knows opengl can use DIRECT NO LAYER KOSPVR API             */
#/* History: version 1                                                                       */
#/********************************************************************************************/
#/********************************************************************************************/
#/*        >>>  Help and code examples and advice these people where invaluable  <<<         */
#/*     Mvp's:  dRxL with my_perspective_mat_lh and explaining to me the concepts            */
#/*     Mvp's:  Bruce tested and found both annoying bugs and texture distortion.            */
#/*                                                                                          */
#/********************************************************************************************/ 
KOS_CFLAGS+= -g -std=c99 -Og
TARGET = pvrcube.elf
OBJS = pvrcube.o 

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

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

clean:
	-rm -f $(TARGET) $(OBJS) romdisk.*

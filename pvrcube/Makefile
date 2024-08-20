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
KOS_CFLAGS+= -g -std=c99 -Og -I$(KOS_BASE)/utils
TARGET = pvrcube.elf
OBJS = pvrcube.o 

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS) romdisk.o
	kos-c++ -o $(TARGET) $(OBJS)romdisk.o -lpng -ljpeg -lkmg -lz -lkosutils -lm


DTTEXTURES:=$(shell find assets/texture -name '*.png'| sed -e 's,assets/\(.*\)/\([a-z_A-Z0-9]*\).png,romdisk/\1/\2.dt,g')
# DTTEXTURES+=$(shell find assets/texture -name '*.jpg'| sed -e 's,assets/\(.*\)/\([a-z_A-Z0-9]*\).jpg,romdisk/\1/\2.dt,g')
$(info $(DTTEXTURES))

# TEXDIR_PAL4=romdisk/texture/pal4
# $(TEXDIR_PAL4):
# 	mkdir -p $@

# $(TEXDIR_PAL4)/%.dt: assets/texture/pal4/%.png $(TEXDIR_PAL4)
# 	pvrtex -f PAL4BPP -c -i $< -o $@

TEXDIR_PAL8=romdisk/texture/pal8
$(TEXDIR_PAL8):
	mkdir -p $@

TEXDIR_RGB565_VQ_TW=romdisk/texture/rgb565_vq_tw
$(TEXDIR_RGB565_VQ_TW):
	mkdir -p $@

$(TEXDIR_RGB565_VQ_TW)/%.dt: assets/texture/rgb565_vq_tw/%.png $(TEXDIR_RGB565_VQ_TW)
	pvrtex -f RGB565 -c -i $< -o $@

romdisk.img: $(DTTEXTURES)
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

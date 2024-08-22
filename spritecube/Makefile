KOS_CFLAGS+= -g -std=c99 -O3 -I$(KOS_BASE)/utils
TARGET = spritecube.elf
OBJS = spritecube.o 

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -rf $(TARGET) $(OBJS) romdisk.* romdisk/*

rm-elf:
	-rm -f $(TARGET) romdisk.*

$(TARGET): $(OBJS) romdisk.o
	kos-c++ -o $(TARGET) $(OBJS)romdisk.o -lpng -ljpeg -lkmg -lz -lkosutils -lm


DTTEXTURES:=$(shell find assets/texture -name '*.png'| sed -e 's,assets/\(.*\)/\([a-z_A-Z0-9]*\).png,romdisk/\1/\2.dt,g')
$(info $(DTTEXTURES))

TEXDIR_PAL4=romdisk/texture/pal4
$(TEXDIR_PAL4):
	mkdir -p $@
$(TEXDIR_PAL4)/%.dt: assets/texture/pal4/%.png $(TEXDIR_PAL4)
	pvrtex -f PAL4BPP -c --max-color 16 -i $< -o $@

TEXDIR_PAL8=romdisk/texture/pal8
$(TEXDIR_PAL8):
	mkdir -p $@
$(TEXDIR_PAL8)/%.dt: assets/texture/pal8/%.png $(TEXDIR_PAL8)
	pvrtex -f PAL8BPP -c --max-color 256 -i $< -o $@


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

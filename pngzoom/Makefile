######################################################################
#           PNG ZOOM TRANSISTION V1                                  #
######################################################################
#																	 #
#																	 #
#								           (c)2024 from Ian Micheal  #
######################################################################

# Files ---------------------------------------------------------------------------
all: rm-elf pngzoom.elf
# Optimize -------------------------------------------------------------------------
KOS_CFLAGS+=  -std=c99 
include $(KOS_BASE)/Makefile.rules
# Files ---------------------------------------------------------------------------
OBJS = pngzoom.o 

KOS_LOCAL_CFLAGS = -I$(KOS_BASE)/addons/zlib
	
clean:
	-rm -f pngzoom.elf $(OBJS)
	-rm -f romdisk_boot.*

dist:
	-rm -f $(OBJS)
	-rm -f romdisk_boot.*
	$(KOS_STRIP) pngzoom.elf
	
rm-elf:
	-rm -f pngzoom.elf
	-rm -f romdisk_boot.*
# Link ----------------------------------------------------------------------------
pngzoom.elf: $(OBJS) romdisk_boot.o 
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $@ $(KOS_START) $^ -lpng -lz -lm $(KOS_LIBS)


# ROM Disk Creation ---------------------------------------------------------------
romdisk_boot.img:
	$(KOS_GENROMFS) -f $@ -d romdisk_boot -v

romdisk_boot.o: romdisk_boot.img
	$(KOS_BASE)/utils/bin2o/bin2o $< romdisk_boot $@

run: pngzoom.elf
	$(KOS_LOADER) $<



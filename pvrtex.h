#ifndef PVTEX_H
#define PVTEX_H

#include <dc/pvr.h>
#include <pvrtex/file_dctex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef fDtHeader dt_header_t;

typedef struct {
  dt_header_t hdr;
  uint32_t pvrformat;
  union flags {
    uint32_t raw;
    struct {
      uint32_t palettised : 1;
      uint32_t twiddled : 1;
      uint32_t compressed : 1;
      uint32_t strided : 1;
      uint32_t mipmapped : 1;
      uint32_t : 27;
    };
  } flags;
  uint16_t width;
  uint16_t height;
  pvr_ptr_t ptr;
} dttex_info_t;

/**
 * @brief Load a texture from a file
 *
 * @param filename The filename of the texture
 * @param texinfo The texture texinfo struct
 * @return int 1 on success, 0 on failure
 */
int load_texture(const char *filename, dttex_info_t *texinfo) {
  int success = 1;
  FILE *fp = NULL;
  do {
    fp = fopen(filename, "rb");
    if (fp == NULL) {
      printf("Error: fopen %s failed, %s\n", filename, strerror(errno));
      success = 0;
      break;
    }
    fread(&(texinfo->hdr), sizeof(dt_header_t), 1, fp);
    size_t tdatasize =
        texinfo->hdr.chunk_size - ((1 + texinfo->hdr.header_size) << 5);

    texinfo->flags.compressed = fDtIsCompressed(&texinfo->hdr);
    texinfo->flags.mipmapped = fDtIsMipmapped(&texinfo->hdr);
    texinfo->flags.palettised = fDtIsPalettized(&texinfo->hdr);
    texinfo->flags.strided = fDtIsStrided(&texinfo->hdr);
    texinfo->flags.twiddled = fDtIsTwiddled(&texinfo->hdr);
    texinfo->width = fDtGetPvrWidth(&texinfo->hdr);
    texinfo->height = fDtGetPvrHeight(&texinfo->hdr);

    texinfo->pvrformat = texinfo->hdr.pvr_type & 0xFFC00000;

    void *buffer = malloc(tdatasize);
    fread(buffer, tdatasize, 1, fp);

    texinfo->ptr = pvr_mem_malloc(tdatasize);
    if (texinfo->ptr == NULL) {
      printf("Error: pvr_mem_malloc failed\n");
      success = 0;
      break;
    }

    pvr_txr_load(buffer, texinfo->ptr, tdatasize);
    free(buffer);
  } while (0);

  if (fp != NULL) {
    fclose(fp);
  }
  return success;
}

/**
 * @brief Load a palette from a file
 *
 * @param filename The filename of the palette
 * @param offset The offset to load the palette into
 * @return int 1 on success, 0 on failure
 */
int load_palette(const char *filename, int fmt, size_t offset) {
  int success = 1;

  struct {
    char fourcc[4];
    size_t colors;
  } palette_hdr;

  FILE *fp = NULL;
  do {
    fp = fopen(filename, "rb");
    if (fp == NULL) {
      printf("Error: fopen %s failed, %s\n", filename, strerror(errno));
      success = 0;
      break;
    }
    fread(&palette_hdr, sizeof(palette_hdr), 1, fp);
    uint32_t colors[palette_hdr.colors];
    fread(&colors, sizeof(uint32_t), palette_hdr.colors, fp);

    pvr_set_pal_format(fmt);
    for (size_t i = 0; i < palette_hdr.colors; i++) {
      pvr_set_pal_entry(i + offset, colors[i]);
    }
  } while (0);

  if (fp != NULL) {
    fclose(fp);
  }
  return success;
}

/**
 * @brief Unload a texture from memory
 * @param texinfo The texture texinfo struct
 * @return int 1 on success, 0 on failure
 */
int unload_texture(dttex_info_t *texinfo) {
    if (texinfo->ptr != NULL) {
    pvr_mem_free(texinfo->ptr);
    texinfo->ptr = NULL;
    return 1;
  }
  return 0;
}
#endif  // PVTEX_H
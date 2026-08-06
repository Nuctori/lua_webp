#include "lua_webp.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Output format parsing
// ---------------------------------------------------------------------------

static char dwebp_lower(char c) {
  return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

// Case-insensitive string compare.
static int dwebp_ieq(const char* a, const char* b) {
  for (; *a != '\0' && *b != '\0'; ++a, ++b) {
    if (dwebp_lower(*a) != dwebp_lower(*b)) return 0;
  }
  return *a == *b;
}

// Maps an output format string to a WebPOutputFileFormat.
// File formats ("png", "ppm", ...) are matched case-insensitively; pixel
// formats (RGB, RGB_565, rgbA, ...) are case-sensitive because e.g. "RGB"
// and "rgbA" are distinct colorspaces.
static int dwebp_parseFormat(const char* fmt, WebPOutputFileFormat* out) {
  // case-sensitive pixel formats first
  if (!strcmp(fmt, "RGB")) { *out = RGB; return 1; }
  if (!strcmp(fmt, "RGBA")) { *out = RGBA; return 1; }
  if (!strcmp(fmt, "BGR")) { *out = BGR; return 1; }
  if (!strcmp(fmt, "BGRA")) { *out = BGRA; return 1; }
  if (!strcmp(fmt, "ARGB")) { *out = ARGB; return 1; }
  if (!strcmp(fmt, "RGBA_4444")) { *out = RGBA_4444; return 1; }
  if (!strcmp(fmt, "RGB_565")) { *out = RGB_565; return 1; }
  if (!strcmp(fmt, "rgbA")) { *out = rgbA; return 1; }
  if (!strcmp(fmt, "bgrA")) { *out = bgrA; return 1; }
  if (!strcmp(fmt, "Argb")) { *out = Argb; return 1; }
  if (!strcmp(fmt, "rgbA_4444")) { *out = rgbA_4444; return 1; }

  // file formats, case-insensitive
  if (dwebp_ieq(fmt, "png")) { *out = PNG; return 1; }
  if (dwebp_ieq(fmt, "pam")) { *out = PAM; return 1; }
  if (dwebp_ieq(fmt, "ppm")) { *out = PPM; return 1; }
  if (dwebp_ieq(fmt, "bmp")) { *out = BMP; return 1; }
  if (dwebp_ieq(fmt, "tiff")) { *out = TIFF; return 1; }
  if (dwebp_ieq(fmt, "pgm")) { *out = PGM; return 1; }
  if (dwebp_ieq(fmt, "yuv")) { *out = RAW_YUV; return 1; }
  if (dwebp_ieq(fmt, "yuva")) { *out = YUVA; return 1; }
  if (dwebp_ieq(fmt, "alpha") || dwebp_ieq(fmt, "alpha_plane_only")) {
    *out = ALPHA_PLANE_ONLY;
    return 1;
  }
  return 0;
}

// Picks the decoder output colorspace for the requested format.
// 'bitstream' must be populated first (WebPGetFeatures / file probe) so that
// alpha-aware formats can be chosen correctly.
static int dwebp_setColorspace(WebPDecBuffer* output,
                               const WebPBitstreamFeatures* bitstream,
                               WebPOutputFileFormat format) {
  switch (format) {
    case PNG:
      output->colorspace = bitstream->has_alpha ? MODE_RGBA : MODE_RGB;
      return 1;
    case PAM:
      output->colorspace = MODE_RGBA;
      return 1;
    case PPM:
      output->colorspace = MODE_RGB;   // drops alpha
      return 1;
    case BMP:
      output->colorspace = bitstream->has_alpha ? MODE_BGRA : MODE_BGR;
      return 1;
    case TIFF:
      output->colorspace = bitstream->has_alpha ? MODE_RGBA : MODE_RGB;
      return 1;
    case PGM:
    case RAW_YUV:
      output->colorspace = bitstream->has_alpha ? MODE_YUVA : MODE_YUV;
      return 1;
    case ALPHA_PLANE_ONLY:
      output->colorspace = MODE_YUVA;
      return 1;
    // forced colorspaces
    case RGB:  output->colorspace = MODE_RGB;  return 1;
    case RGBA: output->colorspace = MODE_RGBA; return 1;
    case BGR:  output->colorspace = MODE_BGR;  return 1;
    case BGRA: output->colorspace = MODE_BGRA; return 1;
    case ARGB: output->colorspace = MODE_ARGB; return 1;
    case RGBA_4444: output->colorspace = MODE_RGBA_4444; return 1;
    case RGB_565:   output->colorspace = MODE_RGB_565;   return 1;
    case rgbA:  output->colorspace = MODE_rgbA;  return 1;
    case bgrA:  output->colorspace = MODE_bgrA;  return 1;
    case Argb:  output->colorspace = MODE_Argb;  return 1;
    case rgbA_4444: output->colorspace = MODE_rgbA_4444; return 1;
    case YUV:  output->colorspace = MODE_YUV;  return 1;
    case YUVA: output->colorspace = MODE_YUVA; return 1;
    default:
      return 0;
  }
}

// Writes the decoded 'buffer' in 'format' to the already-open FILE* 'fout'.
// Mirrors libwebp's WebPSaveImage() for the non-WIC build.
static int dwebp_webPSaveImage(const WebPDecBuffer* const buffer,
                               WebPOutputFileFormat format, FILE* fout) {
  int ok = 1;
  if (format == PNG ||
      format == RGBA || format == BGRA || format == ARGB ||
      format == rgbA || format == bgrA || format == Argb) {
    ok &= WebPWritePNG(fout, buffer);
  } else if (format == PAM) {
    ok &= WebPWritePAM(fout, buffer);
  } else if (format == PPM || format == RGB || format == BGR) {
    ok &= WebPWritePPM(fout, buffer);
  } else if (format == RGBA_4444 || format == RGB_565 || format == rgbA_4444) {
    ok &= WebPWrite16bAsPGM(fout, buffer);
  } else if (format == BMP) {
    ok &= WebPWriteBMP(fout, buffer);
  } else if (format == TIFF) {
    ok &= WebPWriteTIFF(fout, buffer);
  } else if (format == RAW_YUV) {
    ok &= WebPWriteYUV(fout, buffer);
  } else if (format == PGM || format == YUV || format == YUVA) {
    ok &= WebPWritePGM(fout, buffer);
  } else if (format == ALPHA_PLANE_ONLY) {
    ok &= WebPWriteAlphaPlane(fout, buffer);
  }
  return ok;
}

// Serializes 'buffer' in 'format' into a malloc'd byte array
// ('*out'/'*out_size'). The libwebp imageio writers only support FILE*
// output, so we go through a temporary file; this is portable across
// Linux / macOS / Windows.
static int dwebp_saveImage(const WebPDecBuffer* const buffer,
                           WebPOutputFileFormat format,
                           unsigned char** out, size_t* out_size) {
  FILE* const stream = tmpfile();
  long size;
  unsigned char* buf;
  int ok;
  if (stream == NULL) return 0;
  ok = dwebp_webPSaveImage(buffer, format, stream);
  if (ok) ok = (fflush(stream) == 0);
  if (ok && (fseek(stream, 0, SEEK_END) != 0 ||
             (size = ftell(stream)) <= 0 ||
             fseek(stream, 0, SEEK_SET) != 0)) {
    ok = 0;
  }
  if (!ok) {
    fclose(stream);
    return 0;
  }
  buf = (unsigned char*)malloc((size_t)size);
  if (buf == NULL) {
    fclose(stream);
    return 0;
  }
  ok = (fread(buf, 1, (size_t)size, stream) == (size_t)size);
  fclose(stream);
  if (!ok) {
    free(buf);
    return 0;
  }
  *out = buf;
  *out_size = (size_t)size;
  return 1;
}

// ---------------------------------------------------------------------------
// WebPDecoderOptions table mapping
// ---------------------------------------------------------------------------

typedef struct {
  const char* name;
  size_t offset;   // offsetof(WebPDecoderOptions, field)
} dwebp_opt_field;

#define OPT_FIELD(field) { #field, offsetof(WebPDecoderOptions, field) }

static const dwebp_opt_field dwebp_int_options[] = {
  OPT_FIELD(bypass_filtering),
  OPT_FIELD(no_fancy_upsampling),
  OPT_FIELD(use_cropping),
  OPT_FIELD(crop_left),
  OPT_FIELD(crop_top),
  OPT_FIELD(crop_width),
  OPT_FIELD(crop_height),
  OPT_FIELD(use_scaling),
  OPT_FIELD(scaled_width),
  OPT_FIELD(scaled_height),
  OPT_FIELD(use_threads),
  OPT_FIELD(dithering_strength),
  OPT_FIELD(alpha_dithering_strength),
  OPT_FIELD(flip),
};

#undef OPT_FIELD

// Applies the Lua table at stack 'index' (must be a table) onto
// 'config->options'. Raises a Lua error on unknown field names or
// non-numeric values.
static void dwebp_loadWebpDecConf(lua_State* L, int index,
                                  WebPDecoderConfig* config) {
  static const size_t kNum =
      sizeof(dwebp_int_options) / sizeof(dwebp_int_options[0]);
  size_t i;

  luaL_checktype(L, index, LUA_TTABLE);
  lua_pushnil(L);
  while (lua_next(L, index) != 0) {
    const char* name;
    if (lua_type(L, -2) != LUA_TSTRING) {
      luaL_error(L, "dwebp: options table keys must be strings");
    }
    name = lua_tostring(L, -2);
    for (i = 0; i < kNum; ++i) {
      if (strcmp(dwebp_int_options[i].name, name) == 0) break;
    }
    if (i == kNum) {
      luaL_error(L, "dwebp: unknown option '%s'", name);
    }
    if (!lua_isnumber(L, -1)) {
      luaL_error(L, "dwebp: option '%s' must be a number", name);
    }
    *(int*)((char*)&config->options + dwebp_int_options[i].offset) =
        (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
  }
}

// Applies the optional options table at stack index 4 (nil or absent is fine).
static void dwebp_applyOptions(lua_State* L, WebPDecoderConfig* config) {
  if (lua_gettop(L) >= 4 && !lua_isnil(L, 4)) {
    dwebp_loadWebpDecConf(L, 4, config);
  }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

int ldwebp_webp2Image(lua_State* L) {
  WebPDecoderConfig config;
  WebPBitstreamFeatures* const bitstream = &config.input;
  WebPOutputFileFormat format;
  unsigned char* buf = NULL;
  size_t buf_size = 0;
  size_t data_size = 0;
  const uint8_t* const data =
      (const uint8_t*)luaL_checklstring(L, 2, &data_size);
  const char* const fmt = luaL_checkstring(L, 3);
  int ok = 0;

  luaL_checkudata(L, 1, "__dwebp__");
  if (lua_gettop(L) >= 4 && !lua_isnil(L, 4)) {
    luaL_checktype(L, 4, LUA_TTABLE);
  }
  if (!dwebp_parseFormat(fmt, &format)) {
    return luaL_error(L, "dwebp: unknown output format '%s'", fmt);
  }
  if (!WebPInitDecoderConfig(&config)) {
    return luaL_error(L, "dwebp: webp library version mismatch");
  }
  dwebp_applyOptions(L, &config);

  // Parse the bitstream features BEFORE choosing a colorspace: has_alpha
  // must reflect the actual file.
  if (WebPGetFeatures(data, data_size, bitstream) != VP8_STATUS_OK) {
    WebPFreeDecBuffer(&config.output);
    return luaL_error(L, "dwebp: invalid or corrupt webp data");
  }
  if (!dwebp_setColorspace(&config.output, bitstream, format)) {
    WebPFreeDecBuffer(&config.output);
    return luaL_error(L, "dwebp: unsupported output format '%s'", fmt);
  }
  if (WebPDecode(data, data_size, &config) != VP8_STATUS_OK) {
    WebPFreeDecBuffer(&config.output);
    return luaL_error(L, "dwebp: failed to decode webp data");
  }

  ok = dwebp_saveImage(&config.output, format, &buf, &buf_size);
  WebPFreeDecBuffer(&config.output);
  if (!ok) {
    return luaL_error(L, "dwebp: failed to write %s output", fmt);
  }
  lua_pushlstring(L, (const char*)buf, buf_size);
  free(buf);
  return 1;
}

int ldwebp_path2Image(lua_State* L) {
  WebPDecoderConfig config;
  WebPBitstreamFeatures* const bitstream = &config.input;
  WebPOutputFileFormat format;
  unsigned char* buf = NULL;
  size_t buf_size = 0;
  const uint8_t* data = NULL;
  size_t data_size = 0;
  const char* const path = luaL_checkstring(L, 2);
  const char* const fmt = luaL_checkstring(L, 3);
  int ok = 0;

  luaL_checkudata(L, 1, "__dwebp__");
  if (lua_gettop(L) >= 4 && !lua_isnil(L, 4)) {
    luaL_checktype(L, 4, LUA_TTABLE);
  }
  if (!dwebp_parseFormat(fmt, &format)) {
    return luaL_error(L, "dwebp: unknown output format '%s'", fmt);
  }
  if (!WebPInitDecoderConfig(&config)) {
    return luaL_error(L, "dwebp: webp library version mismatch");
  }
  dwebp_applyOptions(L, &config);

  if (!ImgIoUtilReadFile(path, &data, &data_size)) {
    return luaL_error(L, "dwebp: failed to load webp file '%s'", path);
  }
  if (WebPGetFeatures(data, data_size, bitstream) != VP8_STATUS_OK) {
    WebPFree((void*)data);
    return luaL_error(L, "dwebp: invalid webp file '%s'", path);
  }
  if (!dwebp_setColorspace(&config.output, bitstream, format)) {
    WebPFree((void*)data);
    WebPFreeDecBuffer(&config.output);
    return luaL_error(L, "dwebp: unsupported output format '%s'", fmt);
  }
  if (WebPDecode(data, data_size, &config) != VP8_STATUS_OK) {
    WebPFree((void*)data);
    WebPFreeDecBuffer(&config.output);
    return luaL_error(L, "dwebp: failed to decode webp file '%s'", path);
  }
  WebPFree((void*)data);

  ok = dwebp_saveImage(&config.output, format, &buf, &buf_size);
  WebPFreeDecBuffer(&config.output);
  if (!ok) {
    return luaL_error(L, "dwebp: failed to write %s output", fmt);
  }
  lua_pushlstring(L, (const char*)buf, buf_size);
  free(buf);
  return 1;
}

#include "lua_webp.h"

#include <stddef.h>
#include <string.h>

// Reads an image file (PNG/JPEG/TIFF/WebP/PNM) into 'pic'. Adapted from
// libwebp's examples/cwebp.c, minus the Windows WIC fast path.
static int cwebp_ReadImage(const char filename[], WebPPicture* const pic) {
  const uint8_t* data = NULL;
  size_t data_size = 0;
  int ok;
  if (!ImgIoUtilReadFile(filename, &data, &data_size)) return 0;
  ok = WebPGuessImageReader(data, data_size)(data, data_size, pic, 1, NULL);
  WebPFree((void*)data);
  return ok;
}

// ---------------------------------------------------------------------------
// WebPConfig table mapping
// ---------------------------------------------------------------------------

typedef struct {
  const char* name;
  size_t offset;   // offsetof(WebPConfig, field)
} cwebp_config_field;

#define CONFIG_FIELD(field) { #field, offsetof(WebPConfig, field) }

static const cwebp_config_field cwebp_int_fields[] = {
  CONFIG_FIELD(lossless),
  CONFIG_FIELD(method),
  CONFIG_FIELD(target_size),
  CONFIG_FIELD(segments),
  CONFIG_FIELD(sns_strength),
  CONFIG_FIELD(filter_strength),
  CONFIG_FIELD(filter_sharpness),
  CONFIG_FIELD(filter_type),
  CONFIG_FIELD(autofilter),
  CONFIG_FIELD(alpha_compression),
  CONFIG_FIELD(alpha_filtering),
  CONFIG_FIELD(alpha_quality),
  CONFIG_FIELD(pass),
  CONFIG_FIELD(show_compressed),
  CONFIG_FIELD(preprocessing),
  CONFIG_FIELD(partitions),
  CONFIG_FIELD(partition_limit),
  CONFIG_FIELD(emulate_jpeg_size),
  CONFIG_FIELD(thread_level),
  CONFIG_FIELD(low_memory),
  CONFIG_FIELD(near_lossless),
  CONFIG_FIELD(exact),
  CONFIG_FIELD(use_delta_palette),
  CONFIG_FIELD(use_sharp_yuv),
  CONFIG_FIELD(qmin),
  CONFIG_FIELD(qmax),
};

static const cwebp_config_field cwebp_float_fields[] = {
  CONFIG_FIELD(quality),
  CONFIG_FIELD(target_PSNR),
};

#undef CONFIG_FIELD

static int cwebp_find_field(const cwebp_config_field* fields, size_t n,
                            const char* name) {
  size_t i;
  for (i = 0; i < n; ++i) {
    if (strcmp(fields[i].name, name) == 0) return (int)i;
  }
  return -1;
}

// Applies the Lua table at stack 'index' (must be a table) onto 'config'.
// Raises a Lua error on unknown field names or non-numeric values.
static void cwebp_loadWebpConf(lua_State* L, int index, WebPConfig* config) {
  static const size_t kNumInt =
      sizeof(cwebp_int_fields) / sizeof(cwebp_int_fields[0]);
  static const size_t kNumFloat =
      sizeof(cwebp_float_fields) / sizeof(cwebp_float_fields[0]);

  luaL_checktype(L, index, LUA_TTABLE);
  lua_pushnil(L);
  while (lua_next(L, index) != 0) {
    const char* name;
    int fi;
    if (lua_type(L, -2) != LUA_TSTRING) {
      luaL_error(L, "cwebp: config table keys must be strings");
    }
    name = lua_tostring(L, -2);

    fi = cwebp_find_field(cwebp_int_fields, kNumInt, name);
    if (fi >= 0) {
      if (!lua_isnumber(L, -1)) {
        luaL_error(L, "cwebp: config field '%s' must be a number", name);
      }
      *(int*)((char*)config + cwebp_int_fields[fi].offset) =
          (int)lua_tointeger(L, -1);
      lua_pop(L, 1);
      continue;
    }

    fi = cwebp_find_field(cwebp_float_fields, kNumFloat, name);
    if (fi >= 0) {
      if (!lua_isnumber(L, -1)) {
        luaL_error(L, "cwebp: config field '%s' must be a number", name);
      }
      *(float*)((char*)config + cwebp_float_fields[fi].offset) =
          (float)lua_tonumber(L, -1);
      lua_pop(L, 1);
      continue;
    }

    if (strcmp(name, "image_hint") == 0) {
      // Accept either a string ("photo", "picture", "graph", "default") or
      // the raw WEBP_HINT_* integer value.
      if (lua_type(L, -1) == LUA_TSTRING) {
        const char* const hint = lua_tostring(L, -1);
        if (!strcmp(hint, "photo")) config->image_hint = WEBP_HINT_PHOTO;
        else if (!strcmp(hint, "picture")) config->image_hint = WEBP_HINT_PICTURE;
        else if (!strcmp(hint, "graph")) config->image_hint = WEBP_HINT_GRAPH;
        else if (!strcmp(hint, "default")) config->image_hint = WEBP_HINT_DEFAULT;
        else {
          luaL_error(L, "cwebp: config field 'image_hint' has unknown value '%s'",
                     hint);
        }
      } else if (lua_isnumber(L, -1)) {
        config->image_hint = (WebPImageHint)lua_tointeger(L, -1);
      } else {
        luaL_error(L, "cwebp: config field 'image_hint' must be a string or number");
      }
      lua_pop(L, 1);
      continue;
    }

    luaL_error(L, "cwebp: unknown config field '%s'", name);
  }
}

// Applies the optional config table at stack index 3 (nil or absent is fine)
// and validates the result.
static void cwebp_applyConfig(lua_State* L, WebPConfig* config) {
  if (lua_gettop(L) >= 3 && !lua_isnil(L, 3)) {
    cwebp_loadWebpConf(L, 3, config);
    if (!WebPValidateConfig(config)) {
      luaL_error(L, "cwebp: invalid WebP configuration");
    }
  }
}

// Shared setup: initializes config/picture/writer for encoding.
static void cwebp_begin(lua_State* L, WebPConfig* config, WebPPicture* picture,
                        WebPMemoryWriter* writer) {
  if (!WebPConfigInit(config) || !WebPPictureInit(picture)) {
    luaL_error(L, "cwebp: webp library version mismatch");
  }
  WebPMemoryWriterInit(writer);
  picture->use_argb = 1;   // decoders always produce ARGB samples
  picture->writer = WebPMemoryWrite;
  picture->custom_ptr = (void*)writer;
}

int lcwebp_path2webp(lua_State* L) {
  WebPConfig config;
  WebPPicture picture;
  WebPMemoryWriter writer;
  const char* const path = luaL_checkstring(L, 2);

  luaL_checkudata(L, 1, "__cwebp__");
  if (lua_gettop(L) >= 3 && !lua_isnil(L, 3)) {
    luaL_checktype(L, 3, LUA_TTABLE);
  }
  cwebp_begin(L, &config, &picture, &writer);
  cwebp_applyConfig(L, &config);

  if (!cwebp_ReadImage(path, &picture)) {
    WebPPictureFree(&picture);
    return luaL_error(L, "cwebp: failed to read image '%s'", path);
  }
  if (!WebPEncode(&config, &picture)) {
    WebPPictureFree(&picture);
    WebPMemoryWriterClear(&writer);
    return luaL_error(L, "cwebp: failed to encode image '%s'", path);
  }
  lua_pushlstring(L, (const char*)writer.mem, writer.size);
  WebPPictureFree(&picture);
  WebPMemoryWriterClear(&writer);
  return 1;
}

int lcwebp_image2webp(lua_State* L) {
  WebPConfig config;
  WebPPicture picture;
  WebPMemoryWriter writer;
  size_t data_size = 0;
  const uint8_t* const data = (const uint8_t*)luaL_checklstring(L, 2, &data_size);

  luaL_checkudata(L, 1, "__cwebp__");
  if (lua_gettop(L) >= 3 && !lua_isnil(L, 3)) {
    luaL_checktype(L, 3, LUA_TTABLE);
  }
  cwebp_begin(L, &config, &picture, &writer);
  cwebp_applyConfig(L, &config);

  if (!WebPGuessImageReader(data, data_size)(data, data_size, &picture, 1, NULL)) {
    WebPPictureFree(&picture);
    return luaL_error(L, "cwebp: unsupported or corrupt input image data");
  }
  if (!WebPEncode(&config, &picture)) {
    WebPPictureFree(&picture);
    WebPMemoryWriterClear(&writer);
    return luaL_error(L, "cwebp: failed to encode image");
  }
  lua_pushlstring(L, (const char*)writer.mem, writer.size);
  WebPPictureFree(&picture);
  WebPMemoryWriterClear(&writer);
  return 1;
}

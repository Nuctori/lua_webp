#include <stdio.h>
#include "lua_webp.h"


static int ReadImage(const char filename[], WebPPicture* const pic) {
  const uint8_t* data = NULL;
  size_t data_size = 0;
  WebPImageReader reader;
  int ok;
#ifdef HAVE_WINCODEC_H
  // Try to decode the file using WIC falling back to the other readers for
  // e.g., WebP.
  ok = ReadPictureWithWIC(filename, pic, 1, NULL);
  if (ok) return 1;
#endif
  if (!ImgIoUtilReadFile(filename, &data, &data_size)) return 0;
  reader = WebPGuessImageReader(data, data_size);
  ok = reader(data, data_size, pic, 1, NULL);
  WebPFree((void*)data);
  return ok;
}


// 这不是用来暴露的
inline static int cwebp_loadWebpConf(lua_State* L, WebPConfig* config) {
    // 检查参数有效性
    if (!L || !config)
        return 0;
    // 将表的字段名和字段值依次入栈
    lua_pushnil(L);  // 将nil入栈，用于遍历表
    while (lua_next(L, -2) != 0) {
        // 检查字段名类型是否为字符串
        if (lua_type(L, -2) != LUA_TSTRING) {
            printf("Invalid table key type\n");
            lua_pop(L, 2);  // 弹出键值对
            return 0;
        }

        // 获取字段名和字段值
        const char* fieldName = lua_tostring(L, -2);
        int fieldType = lua_type(L, -1);

        // 根据字段名设置相应的字段值
        if (strcmp(fieldName, "method") == 0 && fieldType == LUA_TNUMBER) {
            config->method = lua_tointeger(L, -1);
        } else if (strcmp(fieldName, "quality") == 0 && fieldType == LUA_TNUMBER) {
            config->quality = (float)lua_tonumber(L, -1);
        }
        else if (strcmp(fieldName, "method") == 0 && fieldType == LUA_TNUMBER) {
            config->method = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "target_size") == 0 && fieldType == LUA_TNUMBER) {
            config->target_size = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "target_PSNR") == 0 && fieldType == LUA_TNUMBER) {
            config->target_PSNR = (float)lua_tonumber(L, -1);
        }
        else if (strcmp(fieldName, "segments") == 0 && fieldType == LUA_TNUMBER) {
            config->segments = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "sns_strength") == 0 && fieldType == LUA_TNUMBER) {
            config->sns_strength = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "filter_strength") == 0 && fieldType == LUA_TNUMBER) {
            config->filter_strength = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "filter_sharpness") == 0 && fieldType == LUA_TNUMBER) {
            config->filter_sharpness = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "filter_type") == 0 && fieldType == LUA_TNUMBER) {
            config->filter_type = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "autofilter") == 0 && fieldType == LUA_TNUMBER) {
            config->autofilter = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "alpha_compression") == 0 && fieldType == LUA_TNUMBER) {
            config->alpha_compression = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "alpha_quality") == 0 && fieldType == LUA_TNUMBER) {
            config->alpha_quality = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "pass") == 0 && fieldType == LUA_TNUMBER) {
            config->pass = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "show_compressed") == 0 && fieldType == LUA_TNUMBER) {
            config->show_compressed = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "partitions") == 0 && fieldType == LUA_TNUMBER) {
            config->partitions = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "emulate_jpeg_size") == 0 && fieldType == LUA_TNUMBER) {
            config->emulate_jpeg_size = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "thread_level") == 0 && fieldType == LUA_TNUMBER) {
            config->thread_level = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "low_memory") == 0 && fieldType == LUA_TNUMBER) {
            config->low_memory = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "near_lossless") == 0 && fieldType == LUA_TNUMBER) {
            config->near_lossless = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "exact") == 0 && fieldType == LUA_TNUMBER) {
            config->exact = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "use_delta_palette") == 0 && fieldType == LUA_TNUMBER) {
            config->use_delta_palette = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "use_sharp_yuv") == 0 && fieldType == LUA_TNUMBER) {
            config->use_sharp_yuv = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "qmin") == 0 && fieldType == LUA_TNUMBER) {
            config->qmin = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "qmax") == 0 && fieldType == LUA_TNUMBER) {
            config->qmax = lua_tointeger(L, -1);
        }
        lua_pop(L, 1);  // 弹出值，保留键
    }
    return 1;
}

int lcwebp_path2webp(lua_State* L) {
    luaL_checkudata(L, 1, "__cwebp__");
    const char * path = luaL_checkstring(L, 2);
    WebPConfig config;
    WebPMemoryWriter memory_writer;
    WebPMemoryWriterInit(&memory_writer);
    WebPPicture picture;
    if (!WebPPictureInit(&picture) ||
        !WebPConfigInit(&config)) {
        fprintf(stderr, "Error! Version mismatch!\n");
        return -1;
    }
    config.method = 0;
    config.quality = 100;
    picture.use_argb = 1;
    picture.width = 1;  // width and height will auto get, bug need greater than 0
    picture.height = 1;
    picture.writer = WebPMemoryWrite;
    picture.custom_ptr = (void*)&memory_writer;
    if (!WebPPictureAlloc(&picture)) {
        return 0;   // memory error
    }
    ReadImage((const char*)path, &picture);
    int ok = WebPEncode(&config, &picture);
    if (!ok) {
        printf("encoding error\n");
    }
    lua_pushlstring(L, (const char *)memory_writer.mem, memory_writer.size);
    WebPPictureFree(&picture);
    WebPMemoryWriterClear(&memory_writer);
    return 1;
}

int lcwebp_image2webp(lua_State* L) {
    luaL_checkudata(L, 1, "__cwebp__");
    size_t imageDataSize;
    const uint8_t * imageData = (const uint8_t *)luaL_checklstring (L, 2, &imageDataSize);
    WebPConfig config;
    WebPMemoryWriter memory_writer;
    WebPMemoryWriterInit(&memory_writer);
    WebPPicture picture;
        if (!WebPPictureInit(&picture) ||
        !WebPConfigInit(&config)) {
        fprintf(stderr, "Error! Version mismatch!\n");
        return -1;
    }
    config.method = 0;
    config.quality = 100;
    picture.use_argb = 1;
    picture.width = 1; // width and height will auto get, bug need greater than 0
    picture.height = 1;
    picture.writer = WebPMemoryWrite;
    picture.custom_ptr = (void*)&memory_writer;
    if (!WebPPictureAlloc(&picture)) {
        return 0;   // memory error
    }

    if (!lua_istable(L, 5)) {
        printf("Error: Expected a table\n");
    }
    cwebp_loadWebpConf(L, &config);
    WebPImageReader reader;

    reader = WebPGuessImageReader(imageData, imageDataSize);
    int ok = reader(imageData, imageDataSize, &picture, 1, NULL);
    if (!ok) {
        printf("encoding error\n");
    }
    ok = WebPEncode(&config, &picture);
    if (!ok) {
        printf("encoding error\n");
    }
    lua_pushlstring(L, (const char *)memory_writer.mem, memory_writer.size);
    WebPPictureFree(&picture);
    WebPMemoryWriterClear(&memory_writer);
    return 1;
}
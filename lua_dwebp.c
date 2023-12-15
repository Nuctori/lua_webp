#include <stdio.h>
#include "lua_webp.h"

// 这不是用来暴露的
inline static int dwebp_loadWebpDecConf(lua_State* L, WebPDecoderConfig* config) {
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

        if (strcmp(fieldName, "no_fancy_upsampling") == 0 && fieldType == LUA_TNUMBER) {
            config->options.no_fancy_upsampling = lua_tointeger(L, -1);
        } else if (strcmp(fieldName, "bypass_filtering") == 0 && fieldType == LUA_TNUMBER) {
            config->options.bypass_filtering = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "use_threads") == 0 && fieldType == LUA_TNUMBER) {
            
            config->options.use_threads = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "alpha_dithering_strength") == 0 && fieldType == LUA_TNUMBER) {
            config->options.alpha_dithering_strength = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "dithering_strength") == 0 && fieldType == LUA_TNUMBER) {
            config->options.dithering_strength = lua_tointeger(L, -1);
        }
        else if (strcmp(fieldName, "use_cropping") == 0 && fieldType == LUA_TNUMBER) {
            config->options.use_cropping = lua_tonumber(L, -1);
        }
        else if (strcmp(fieldName, "crop_left") == 0 && fieldType == LUA_TNUMBER) {
            config->options.crop_left = lua_tonumber(L, -1);
        }
        else if (strcmp(fieldName, "crop_top") == 0 && fieldType == LUA_TNUMBER) {
            config->options.crop_top = lua_tonumber(L, -1);
        }
        else if (strcmp(fieldName, "crop_width") == 0 && fieldType == LUA_TNUMBER) {
            config->options.crop_width = lua_tonumber(L, -1);
        }
        else if (strcmp(fieldName, "crop_height") == 0 && fieldType == LUA_TNUMBER) {
            config->options.crop_height = lua_tonumber(L, -1);
        }
        else if (strcmp(fieldName, "use_scaling") == 0 && fieldType == LUA_TNUMBER) {
            config->options.use_scaling = lua_tonumber(L, -1);
        }
        else if (strcmp(fieldName, "scaled_width") == 0 && fieldType == LUA_TNUMBER) {
            config->options.scaled_width = lua_tonumber(L, -1);
        }
        else if (strcmp(fieldName, "scaled_height") == 0 && fieldType == LUA_TNUMBER) {
            config->options.scaled_height = lua_tonumber(L, -1);
        }
        else if (strcmp(fieldName, "flip") == 0 && fieldType == LUA_TNUMBER) {
            config->options.flip = lua_tonumber(L, -1);
        }
    
        lua_pop(L, 1);  // 弹出值，保留键
    }
    return 1;
}

static int dwebp_webPSaveImage(const WebPDecBuffer* const buffer,
                  WebPOutputFileFormat format,
                  FILE* fout) {
  int ok = 1;
  if (format == PNG ||
      format == RGBA || format == BGRA || format == ARGB ||
      format == rgbA || format == bgrA || format == Argb) {
#ifdef HAVE_WINCODEC_H
    ok &= WebPWritePNG(out_file_name, use_stdout, buffer);
#else
    ok &= WebPWritePNG(fout, buffer);
#endif
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

int ldwebp_webp2Image(lua_State* L) {
    printf("ldwebp_webp2Image");
    luaL_checkudata(L, 1, "__dwebp__");
    size_t imageDataSize;
    const uint8_t * webpData = (const uint8_t *)luaL_checklstring (L, 2, &imageDataSize);
    const char * fmt = luaL_checkstring(L, 3);

    WebPDecoderConfig config;
    WebPDecBuffer* const output_buffer = &config.output;
    WebPBitstreamFeatures* const bitstream = &config.input;
    WebPOutputFileFormat format = PNG;
    if (!WebPInitDecoderConfig(&config)) {
        lua_pushstring(L, "Library version mismatch");
        return lua_error(L);
    }

    if      (!strcmp(fmt, "RGB"))  format = RGB;
    else if (!strcmp(fmt, "RGBA")) format = RGBA;
    else if (!strcmp(fmt, "BGR"))  format = BGR;
    else if (!strcmp(fmt, "BGRA")) format = BGRA;
    else if (!strcmp(fmt, "ARGB")) format = ARGB;
    else if (!strcmp(fmt, "RGBA_4444")) format = RGBA_4444;
    else if (!strcmp(fmt, "RGB_565")) format = RGB_565;
    else if (!strcmp(fmt, "rgbA")) format = rgbA;
    else if (!strcmp(fmt, "bgrA")) format = bgrA;
    else if (!strcmp(fmt, "Argb")) format = Argb;
    else if (!strcmp(fmt, "rgbA_4444")) format = rgbA_4444;
    else if (!strcmp(fmt, "YUV"))  format = YUV;
    else if (!strcmp(fmt, "YUVA")) format = YUVA;
    else if (!strcmp(fmt, "pam")) format = PAM;
    else if (!strcmp(fmt, "ppm")) format = PPM;
    else if (!strcmp(fmt, "bmp")) format = BMP;
    else if (!strcmp(fmt, "tiff")) format = TIFF;
    dwebp_loadWebpDecConf(L, &config);

    VP8StatusCode status = VP8_STATUS_OK;
    switch (format) {
        case PNG:
        #ifdef HAVE_WINCODEC_H
                output_buffer->colorspace = bitstream->has_alpha ? MODE_BGRA : MODE_BGR;
        #else
                output_buffer->colorspace = bitstream->has_alpha ? MODE_RGBA : MODE_RGB;
        #endif
            break;
        case PAM:
            output_buffer->colorspace = MODE_RGBA;
            break;
        case PPM:
            output_buffer->colorspace = MODE_RGB;  // drops alpha for PPM
            break;
        case BMP:
            output_buffer->colorspace = bitstream->has_alpha ? MODE_BGRA : MODE_BGR;
            break;
        case TIFF:
            output_buffer->colorspace = bitstream->has_alpha ? MODE_RGBA : MODE_RGB;
            break;
        case PGM:
        case RAW_YUV:
            output_buffer->colorspace = bitstream->has_alpha ? MODE_YUVA : MODE_YUV;
            break;
        case ALPHA_PLANE_ONLY:
            output_buffer->colorspace = MODE_YUVA;
            break;
        // forced modes:
        case RGB: output_buffer->colorspace = MODE_RGB; break;
        case RGBA: output_buffer->colorspace = MODE_RGBA; break;
        case BGR: output_buffer->colorspace = MODE_BGR; break;
        case BGRA: output_buffer->colorspace = MODE_BGRA; break;
        case ARGB: output_buffer->colorspace = MODE_ARGB; break;
        case RGBA_4444: output_buffer->colorspace = MODE_RGBA_4444; break;
        case RGB_565: output_buffer->colorspace = MODE_RGB_565; break;
        case rgbA: output_buffer->colorspace = MODE_rgbA; break;
        case bgrA: output_buffer->colorspace = MODE_bgrA; break;
        case Argb: output_buffer->colorspace = MODE_Argb; break;
        case rgbA_4444: output_buffer->colorspace = MODE_rgbA_4444; break;
        case YUV: output_buffer->colorspace = MODE_YUV; break;
        case YUVA: output_buffer->colorspace = MODE_YUVA; break;
        default: goto Exit;
    }

    status = DecodeWebP(webpData, imageDataSize, &config);
    int ok = 0;
    ok = (status == VP8_STATUS_OK);
    if (!ok) {
      goto Exit;
    }
    FILE *stream;
    char *ptr;
    size_t imgSize = 0;
    stream = open_memstream(&ptr, &imgSize);
    dwebp_webPSaveImage(output_buffer, format, stream);
    fclose(stream);
    lua_pushlstring(L, ptr, imgSize);
    Exit:
    WebPFreeDecBuffer(output_buffer);
    if (!ok) {
        return lua_error(L);
    }

    return 1;
}

int ldwebp_path2Image(lua_State* L) {
    luaL_checkudata(L, 1, "__dwebp__");
    const char * path = luaL_checkstring(L, 2);
    const char * fmt = luaL_checkstring(L, 3);

    WebPDecoderConfig config;
    WebPDecBuffer* const output_buffer = &config.output;
    WebPBitstreamFeatures* const bitstream = &config.input;
    WebPOutputFileFormat format = PNG;
    if (!WebPInitDecoderConfig(&config)) {
        lua_pushstring(L, "Library version mismatch");
        return lua_error(L);
    }

    if      (!strcmp(fmt, "RGB"))  format = RGB;
    else if (!strcmp(fmt, "RGBA")) format = RGBA;
    else if (!strcmp(fmt, "BGR"))  format = BGR;
    else if (!strcmp(fmt, "BGRA")) format = BGRA;
    else if (!strcmp(fmt, "ARGB")) format = ARGB;
    else if (!strcmp(fmt, "RGBA_4444")) format = RGBA_4444;
    else if (!strcmp(fmt, "RGB_565")) format = RGB_565;
    else if (!strcmp(fmt, "rgbA")) format = rgbA;
    else if (!strcmp(fmt, "bgrA")) format = bgrA;
    else if (!strcmp(fmt, "Argb")) format = Argb;
    else if (!strcmp(fmt, "rgbA_4444")) format = rgbA_4444;
    else if (!strcmp(fmt, "YUV"))  format = YUV;
    else if (!strcmp(fmt, "YUVA")) format = YUVA;
    else if (!strcmp(fmt, "pam")) format = PAM;
    else if (!strcmp(fmt, "ppm")) format = PPM;
    else if (!strcmp(fmt, "bmp")) format = BMP;
    else if (!strcmp(fmt, "tiff")) format = TIFF;
    dwebp_loadWebpDecConf(L, &config);
    
    const uint8_t* data = NULL;
    VP8StatusCode status = VP8_STATUS_OK;
    size_t data_size = 0;
    if (!LoadWebP(path, &data, &data_size, bitstream)) {
        lua_pushstring(L, "webp load error");
        return lua_error(L);
    }
    switch (format) {
        case PNG:
        #ifdef HAVE_WINCODEC_H
                output_buffer->colorspace = bitstream->has_alpha ? MODE_BGRA : MODE_BGR;
        #else
                output_buffer->colorspace = bitstream->has_alpha ? MODE_RGBA : MODE_RGB;
        #endif
            break;
        case PAM:
            output_buffer->colorspace = MODE_RGBA;
            break;
        case PPM:
            output_buffer->colorspace = MODE_RGB;  // drops alpha for PPM
            break;
        case BMP:
            output_buffer->colorspace = bitstream->has_alpha ? MODE_BGRA : MODE_BGR;
            break;
        case TIFF:
            output_buffer->colorspace = bitstream->has_alpha ? MODE_RGBA : MODE_RGB;
            break;
        case PGM:
        case RAW_YUV:
            output_buffer->colorspace = bitstream->has_alpha ? MODE_YUVA : MODE_YUV;
            break;
        case ALPHA_PLANE_ONLY:
            output_buffer->colorspace = MODE_YUVA;
            break;
        // forced modes:
        case RGB: output_buffer->colorspace = MODE_RGB; break;
        case RGBA: output_buffer->colorspace = MODE_RGBA; break;
        case BGR: output_buffer->colorspace = MODE_BGR; break;
        case BGRA: output_buffer->colorspace = MODE_BGRA; break;
        case ARGB: output_buffer->colorspace = MODE_ARGB; break;
        case RGBA_4444: output_buffer->colorspace = MODE_RGBA_4444; break;
        case RGB_565: output_buffer->colorspace = MODE_RGB_565; break;
        case rgbA: output_buffer->colorspace = MODE_rgbA; break;
        case bgrA: output_buffer->colorspace = MODE_bgrA; break;
        case Argb: output_buffer->colorspace = MODE_Argb; break;
        case rgbA_4444: output_buffer->colorspace = MODE_rgbA_4444; break;
        case YUV: output_buffer->colorspace = MODE_YUV; break;
        case YUVA: output_buffer->colorspace = MODE_YUVA; break;
        default: goto Exit;
    }
    // some time LoadWebP will not return a right data_size
    status = DecodeWebP(data, data_size * 2, &config);
    int ok = 0;
    ok = (status == VP8_STATUS_OK);
    if (!ok) {
        lua_pushstring(L, "webp decode error");
        goto Exit;
    }

    FILE *stream;
    char *ptr;
    size_t imgSize = 0;
    stream = open_memstream(&ptr, &imgSize);
    dwebp_webPSaveImage(output_buffer, format, stream);
    fclose(stream);
    lua_pushlstring(L, ptr, imgSize);
    Exit:
    WebPFreeDecBuffer(output_buffer);
    WebPFree((void*)data);
    if (!ok) {
        return lua_error(L);
    }
    return 1;
}
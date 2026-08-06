#ifndef LUA_WEBP_H_
#define LUA_WEBP_H_

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include <webp/decode.h>
#include <webp/encode.h>
#include <webp/types.h>

#include "imageio/image_dec.h"
#include "imageio/image_enc.h"
#include "imageio/imageio_util.h"

int lcwebp_path2webp(lua_State* L);
int lcwebp_image2webp(lua_State* L);
int ldwebp_webp2Image(lua_State* L);
int ldwebp_path2Image(lua_State* L);

#endif  /* LUA_WEBP_H_ */

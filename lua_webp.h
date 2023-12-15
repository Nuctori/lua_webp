#include <stdio.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "libwebp/src/webp/decode.h"
#include "libwebp/src/webp/encode.h"
#include "libwebp/examples/example_util.h"
#include "libwebp/imageio/image_dec.h"
#include "libwebp/imageio/image_enc.h"
#include "libwebp/imageio/imageio_util.h"
#include "libwebp/examples/unicode.h"

int lcwebp_path2webp(lua_State* L);
int lcwebp_image2webp(lua_State* L);
int ldwebp_webp2Image(lua_State* L);
int ldwebp_path2Image(lua_State* L);


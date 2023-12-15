#include <stdio.h>
#include "lua_webp.h"

static inline void lua_webp_register(lua_State *L, const char* meta, luaL_Reg libs[]) {
  if (LUA_MINSTACK - lua_gettop(L) < 3)
    luaL_checkstack(L, 3, "check lua stack error.");

  luaL_newmetatable(L, meta);

  lua_pushstring (L, "__index");
  lua_pushvalue(L, -2);
  lua_rawset(L, -3);
  
  lua_pushliteral(L, "__mode");
  lua_pushliteral(L, "kv");
  lua_rawset(L, -3);

  luaL_setfuncs(L, libs, 0);

  lua_pop(L, 1);
}

LUAMOD_API int luaopen_lua_webp(lua_State *L) {
  luaL_checkversion(L);
  // cwebp 使用 WebP 格式压缩图片。输入格式可以是 PNG、JPEG、TIFF、WebP 或原始 Y'CbCr 样本。注意：不支持 PNG 和 WebP 动画文件。
  lua_webp_register(L, "__cwebp__", (luaL_Reg []) {
    {"image2Webp", lcwebp_image2webp},
    {"path2Webp", lcwebp_path2webp},
    {NULL, NULL},
  });

  lua_webp_register(L, "__dwebp__", (luaL_Reg []) {
    {"webp2Image", ldwebp_webp2Image},
    {"path2Image", ldwebp_path2Image},
    {NULL, NULL},
  });

  lua_createtable(L, 0, 2);
  lua_newuserdata(L, 1);
  luaL_setmetatable(L, "__cwebp__");
  lua_setfield(L, -2, "cwebp");

  lua_newuserdata(L, 1);
  luaL_setmetatable(L, "__dwebp__");
  lua_setfield(L, -2, "dwebp");
  return 1;
}
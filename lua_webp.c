#include "lua_webp.h"

// Creates a metatable 'meta' in the registry, wires up __index to it and
// registers the 'libs' member functions on it. The metatable is then shared
// by all userdata created with it.
static void lua_webp_register(lua_State* L, const char* meta,
                              const luaL_Reg libs[]) {
  luaL_checkstack(L, 4, "lua_webp: stack overflow");
  luaL_newmetatable(L, meta);
  lua_pushliteral(L, "__index");
  lua_pushvalue(L, -2);
  lua_rawset(L, -3);
  luaL_setfuncs(L, libs, 0);
  lua_pop(L, 1);
}

LUAMOD_API int luaopen_lua_webp(lua_State* L) {
  luaL_checkversion(L);

  // cwebp: compress PNG/JPEG/TIFF/WebP/PNM images into WebP.
  lua_webp_register(L, "__cwebp__", (const luaL_Reg[]){
    {"image2Webp", lcwebp_image2webp},
    {"path2Webp", lcwebp_path2webp},
    {NULL, NULL},
  });

  // dwebp: decompress WebP images into PNG/PPM/PAM/BMP/TIFF/raw formats.
  lua_webp_register(L, "__dwebp__", (const luaL_Reg[]){
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

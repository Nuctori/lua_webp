package = "lua-webp"
version = "scm-1"
source = {
  url = "git+https://github.com/Nuctori/lua_webp",
}
description = {
  summary = "Lua bindings for WebP image encoding/decoding",
  detailed = "Lua C extension exposing cwebp/dwebp-style conversion between " ..
             "WebP and PNG/JPEG/TIFF/PPM/PAM/BMP formats, plus full encoder " ..
             "and decoder configuration.",
  license = "MIT",
  homepage = "https://github.com/Nuctori/lua_webp",
}
dependencies = {
  "lua >= 5.3",
}
build = {
  type = "make",
  build_target = "build",
  install_target = "install",
  build_variables = {
    LUA_VERSION = "$(LUA_VERSION)",
    PREFIX = "$(PREFIX)",
  },
}

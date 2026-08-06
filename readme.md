# lua_webp

`lua_webp` is a Lua C extension for WebP image conversion. It exposes
`cwebp`- and `dwebp`-style entry points:

- **cwebp** — compress PNG, JPEG, TIFF, WebP and PNM images into WebP
- **dwebp** — decompress WebP into PNG, PPM, PAM, BMP, TIFF, PGM, YUV and raw
  pixel formats

The module is built on the [libwebp](https://github.com/webmproject/libwebp)
public API plus the libwebp `imageio` glue (vendored under
`third_party/libwebp`, libwebp v1.4.0). When a system libwebp is not found via
`pkg-config`, the vendored source tree is built with CMake, so a clone of this
repo is enough to build the module.

## Requirements

- A C99 compiler
- Lua 5.3 or newer
- `pkg-config`
- System development packages, found via `pkg-config`:
  - `libwebp` + `libwebpdemux`
  - `libpng`, `libjpeg`, `libtiff` (for the PNG/JPEG/TIFF input readers)
  - `cmake` (only when no system libwebp is available — the vendored fallback)

Install examples:

```sh
# Debian / Ubuntu
sudo apt-get install build-essential pkg-config lua5.4 liblua5.4-dev \
  libwebp-dev libpng-dev libjpeg-dev libtiff-dev

# macOS (Homebrew)
brew install pkgconf lua@5.4 webp libpng jpeg-turbo libtiff

# Windows (MSYS2 / UCRT64)
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make \
  mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-lua54 \
  mingw-w64-ucrt-x86_64-libwebp mingw-w64-ucrt-x86_64-libpng \
  mingw-w64-ucrt-x86_64-libjpeg-turbo mingw-w64-ucrt-x86_64-libtiff
```

## Build

```bash
make build
```

You can choose the Lua version at build time:

```bash
make LUA_VERSION=5.3 build
make LUA_VERSION=5.4 build
```

## Test

```bash
make test
```

The test suite (`tests/test.lua`) covers module loading, file and memory
encoding, lossless pixel-exact round-trips (PPM/PAM), PNG/BMP/TIFF/YUV
decoding, encoder quality and decoder options (crop, scale, threads), and
error handling. Fixtures are committed under `tests/` and can be regenerated
with `make fixtures`.

## Usage

```lua
local webp = require "lua_webp"
local cwebp = webp.cwebp
local dwebp = webp.dwebp

-- Encode a file to WebP (full encoder configuration is optional)
local outWebp = cwebp:path2Webp("test.jpg", { quality = 75, method = 4 })
local file = io.open("test.webp", "wb")
file:write(outWebp)
file:close()

-- Encode image bytes to WebP
local f = io.open("test.jpg", "rb")
local jpgData = f:read("*a")
f:close()
local outWebp2 = cwebp:image2Webp(jpgData, { lossless = 1 })
io.open("test2.webp", "wb"):write(outWebp2):close()

-- Decode a WebP file to PNG (decoder options are optional)
local png = dwebp:path2Image("test.webp", "png", { use_threads = 1 })
io.open("webp2png1.png", "wb"):write(png):close()

-- Decode WebP bytes to PPM
local ppm = dwebp:webp2Image(outWebp2, "ppm", {})
io.open("webp2ppm2.ppm", "wb"):write(ppm):close()
```

### API

`cwebp:path2Webp(path, config?)` → webp string
`cwebp:image2Webp(data, config?)` → webp string
`dwebp:path2Image(path, format, options?)` → image string
`dwebp:webp2Image(data, format, options?)` → image string
`dwebp:info(data)` → `{ width, height, has_alpha, has_animation, format }`
`dwebp:infoFromPath(path)` → same info table
`webp.version()` → `{ encoder, decoder }` version strings

- `config` maps directly onto `WebPConfig` (`quality`, `lossless`, `method`,
  `target_size`, `alpha_quality`, `near_lossless`, `exact`, `use_sharp_yuv`,
  ...; see `src/webp/encode.h`). `image_hint` accepts `"photo"`, `"picture"`,
  `"graph"` or a raw `WEBP_HINT_*` integer. Unknown fields and out-of-range
  values raise a Lua error.
- `format` is one of:
  - container formats (single string result): `png`, `ppm`, `pam`, `bmp`,
    `tiff`, `pgm`, `yuv`, `yuva`, `alpha`
  - forced colorspaces, returned as **raw, tightly packed pixel bytes** plus
    their dimensions — `bytes, width, height = dwebp:webp2Image(data, "RGBA")`:
    `RGB`/`BGR` (3 bytes/px), `RGBA`/`BGRA`/`ARGB`/`rgbA`/`bgrA`/`Argb`
    (4 bytes/px), `RGBA_4444`/`RGB_565`/`rgbA_4444` (2 bytes/px)
- `options` maps onto `WebPDecoderOptions` (`use_threads`, `use_cropping` +
  `crop_*`, `use_scaling` + `scaled_*`, `flip`, `dithering_strength`, ...).

All failures raise Lua errors with descriptive messages. Animated WebP files
are not supported (only the first frame is decoded); `info()` reports
`has_animation` so callers can detect them.

## Installation

The simplest path is a source build:

```bash
make build
```

With LuaRocks:

```bash
luarocks make lua-webp-scm-1.rockspec
```

Tagged GitHub releases publish prebuilt module artifacts for Linux, macOS and
Windows (see the `release` workflow); drop the matching asset next to your
Lua package path.

## License

MIT. See [LICENSE](LICENSE). The vendored libwebp tree is BSD-licensed
(see `third_party/libwebp/COPYING` and `third_party/libwebp/PATENTS`).

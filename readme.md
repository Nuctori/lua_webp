# lua_webp

> **[中文文档](README_zh.md)** · English

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
  - `libwebp` **>= 1.3.0** + `libwebpdemux` (an older system libwebp, e.g.
    Ubuntu 22.04's 1.2.x, is detected and the vendored build is used instead)
  - `libpng`, `libjpeg`, `libtiff` (for the PNG/JPEG/TIFF input readers)
  - `cmake` (only when no suitable system libwebp is available — the
    vendored fallback)

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

| Function | Returns |
| ---------- | --------- |
| `cwebp:path2Webp(path, config?)` | WebP bytes (string) |
| `cwebp:image2Webp(data, config?)` | WebP bytes (string) |
| `dwebp:path2Image(path, format, options?)` | image bytes (string) |
| `dwebp:webp2Image(data, format, options?)` | image bytes, or `bytes, width, height` for forced colorspaces |
| `dwebp:info(data)` | info table |
| `dwebp:infoFromPath(path)` | info table |
| `webp.version()` | `{ encoder, decoder }` |

#### `config` — encoder settings (optional)

Maps directly onto libwebp's `WebPConfig` (see `src/webp/encode.h` for the
full list). All fields are numbers unless noted. Unknown fields and
out-of-range values raise a Lua error.

| Field | Range / values | Meaning |
| ------- | ---------------- | --------- |
| `quality` | 0–100 | lossy quality (75 default) |
| `lossless` | 0 or 1 | lossless encoding |
| `method` | 0–6 | quality/speed trade-off |
| `target_size` | ≥ 0 | desired output size in bytes |
| `target_PSNR` | ≥ 0 | minimal distortion target |
| `segments`, `sns_strength`, `filter_strength`, `filter_sharpness`, `filter_type` | — | VP8 analysis / filtering |
| `autofilter` | 0 or 1 | auto filter strength |
| `alpha_compression`, `alpha_filtering`, `alpha_quality` | — | alpha plane coding |
| `pass` | 1–10 | entropy-analysis passes |
| `preprocessing`, `partitions`, `partition_limit` | — | internal knobs |
| `emulate_jpeg_size`, `thread_level`, `low_memory` | — | |
| `near_lossless` | 0–100 | near-lossless strength |
| `exact` | 0 or 1 | preserve exact RGB values |
| `use_delta_palette`, `use_sharp_yuv` | 0 or 1 | |
| `qmin`, `qmax` | 0–100 | quality factor bounds |
| `image_hint` | `"photo"` \| `"picture"` \| `"graph"` \| `"default"` \| int | image type hint (also accepts the raw `WEBP_HINT_*` value) |

#### `format` — decoder output format

| Category | Values | Returns |
| ---------- | -------- | --------- |
| container | `png`, `ppm`, `pam`, `bmp`, `tiff`, `pgm`, `yuv`, `yuva`, `alpha` | single string (file bytes) |
| forced colorspace | `RGB`, `BGR` — 3 bytes/px | `bytes, width, height` |
| forced colorspace | `RGBA`, `BGRA`, `ARGB`, `rgbA`, `bgrA`, `Argb` — 4 bytes/px | `bytes, width, height` |
| forced colorspace | `RGBA_4444`, `RGB_565`, `rgbA_4444` — 2 bytes/px | `bytes, width, height` |

Forced colorspaces return **raw, tightly packed pixel bytes** (no stride
padding) plus their dimensions:

```lua
local bytes, w, h = dwebp:webp2Image(data, "RGBA")   -- w*h*4 bytes
local rgb, w, h = dwebp:webp2Image(data, "RGB")      -- w*h*3 bytes
```

#### `options` — decoder settings (optional)

Maps directly onto libwebp's `WebPDecoderOptions`. All fields are numbers.

| Field | Meaning |
| ------- | --------- |
| `use_threads` | multi-threaded decoding |
| `use_cropping` + `crop_left`, `crop_top`, `crop_width`, `crop_height` | crop the output |
| `use_scaling` + `scaled_width`, `scaled_height` | scale the output (after cropping) |
| `flip` | flip output vertically |
| `bypass_filtering` | skip in-loop filtering |
| `no_fancy_upsampling` | use faster pointwise upsampler |
| `dithering_strength` | 0–100, dithering strength |
| `alpha_dithering_strength` | 0–100, alpha-plane dithering |

#### `info` table

`dwebp:info(data)` / `dwebp:infoFromPath(path)` return:

| Field | Type | Meaning |
| ------- | ------ | --------- |
| `width`, `height` | integer | pixel dimensions |
| `has_alpha` | boolean | bitstream contains an alpha channel |
| `has_animation` | boolean | bitstream is an animation |
| `format` | string | `"undefined"`, `"lossy"`, or `"lossless"` |

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

# lua_webp

`lua_webp` 是一个 Lua C 扩展，用于 WebP 图像格式转换。它提供 `cwebp` 和
`dwebp` 两套入口：

- **cwebp** — 将 PNG、JPEG、TIFF、WebP、PNM 图像压缩为 WebP
- **dwebp** — 将 WebP 解压为 PNG、PPM、PAM、BMP、TIFF、PGM、YUV 及原始像素格式

模块基于 [libwebp](https://github.com/webmproject/libwebp) 公共 API 及其
`imageio` 胶水层（vendor 在 `third_party/libwebp`，libwebp v1.4.0）。当系统
未安装 libwebp 时，会用 CMake 构建 vendored 源码树，因此克隆本仓库即可完成
构建。

## 依赖要求

- C99 编译器
- Lua 5.3 及以上
- `pkg-config`
- 通过 `pkg-config` 查找的系统开发包：
  - `libwebp` **>= 1.3.0** + `libwebpdemux`（若系统 libwebp 过旧，例如
    Ubuntu 22.04 的 1.2.x，会自动改用 vendored 构建）
  - `libpng`、`libjpeg`、`libtiff`（PNG/JPEG/TIFF 输入解码器）
  - `cmake`（仅当没有合适的系统 libwebp 时才需要——vendored 回退路径）

安装示例：

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

## 构建

```bash
make build
```

可在构建时指定 Lua 版本：

```bash
make LUA_VERSION=5.3 build
make LUA_VERSION=5.4 build
```

## 测试

```bash
make test
```

测试套件（`tests/test.lua`）覆盖：模块加载、文件/内存编码、无损逐像素回环
（PPM/PAM）、PNG/BMP/TIFF/YUV 解码、编码质量、解码器选项（裁剪、缩放、
多线程）以及错误处理。固定测试图片位于 `tests/` 下，可通过 `make fixtures`
重新生成。

## 使用示例

```lua
local webp = require "lua_webp"
local cwebp = webp.cwebp
local dwebp = webp.dwebp

-- 将文件编码为 WebP（编码器配置可选）
local outWebp = cwebp:path2Webp("test.jpg", { quality = 75, method = 4 })
local file = io.open("test.webp", "wb")
file:write(outWebp)
file:close()

-- 将图像字节编码为 WebP
local f = io.open("test.jpg", "rb")
local jpgData = f:read("*a")
f:close()
local outWebp2 = cwebp:image2Webp(jpgData, { lossless = 1 })
io.open("test2.webp", "wb"):write(outWebp2):close()

-- 将 WebP 文件解码为 PNG（解码器选项可选）
local png = dwebp:path2Image("test.webp", "png", { use_threads = 1 })
io.open("webp2png1.png", "wb"):write(png):close()

-- 将 WebP 字节解码为 PPM
local ppm = dwebp:webp2Image(outWebp2, "ppm", {})
io.open("webp2ppm2.ppm", "wb"):write(ppm):close()
```

## API

| 函数 | 返回 |
| ------ | ------ |
| `cwebp:path2Webp(path, config?)` | WebP 字节串 |
| `cwebp:image2Webp(data, config?)` | WebP 字节串 |
| `dwebp:path2Image(path, format, options?)` | 图像字节串 |
| `dwebp:webp2Image(data, format, options?)` | 图像字节串；强制色彩空间返回 `bytes, width, height` |
| `dwebp:info(data)` | 信息表 |
| `dwebp:infoFromPath(path)` | 信息表 |
| `webp.version()` | `{ encoder, decoder }` |

### `config` — 编码器设置（可选）

直接映射 libwebp 的 `WebPConfig`（完整字段见 `src/webp/encode.h`）。除特别
说明外均为数字。未知字段或越界值会抛出 Lua 错误。

| 字段 | 取值范围 | 含义 |
| ------ | ---------- | ------ |
| `quality` | 0–100 | 有损质量（默认 75） |
| `lossless` | 0 或 1 | 无损编码 |
| `method` | 0–6 | 质量/速度权衡 |
| `target_size` | ≥ 0 | 目标输出大小（字节） |
| `target_PSNR` | ≥ 0 | 最小失真目标 |
| `segments`、`sns_strength`、`filter_strength`、`filter_sharpness`、`filter_type` | — | VP8 分析/滤波 |
| `autofilter` | 0 或 1 | 自动滤波强度 |
| `alpha_compression`、`alpha_filtering`、`alpha_quality` | — | 透明通道编码 |
| `pass` | 1–10 | 熵分析遍数 |
| `preprocessing`、`partitions`、`partition_limit` | — | 内部参数 |
| `emulate_jpeg_size`、`thread_level`、`low_memory` | — | |
| `near_lossless` | 0–100 | 近无损强度 |
| `exact` | 0 或 1 | 保留精确 RGB 值 |
| `use_delta_palette`、`use_sharp_yuv` | 0 或 1 | |
| `qmin`、`qmax` | 0–100 | 质量因子上下限 |
| `image_hint` | `"photo"`、`"picture"`、`"graph"`、`"default"` 或整数 | 图像类型提示（也接受原始 `WEBP_HINT_*` 值） |

### `format` — 解码器输出格式

| 类别 | 取值 | 返回 |
| ------ | ------ | ------ |
| 容器格式 | `png`、`ppm`、`pam`、`bmp`、`tiff`、`pgm`、`yuv`、`yuva`、`alpha` | 单个字节串（文件字节） |
| 强制色彩空间 | `RGB`、`BGR` — 3 字节/像素 | `bytes, width, height` |
| 强制色彩空间 | `RGBA`、`BGRA`、`ARGB`、`rgbA`、`bgrA`、`Argb` — 4 字节/像素 | `bytes, width, height` |
| 强制色彩空间 | `RGBA_4444`、`RGB_565`、`rgbA_4444` — 2 字节/像素 | `bytes, width, height` |

强制色彩空间返回**紧凑排列的原始像素字节**（无 stride 填充）及图像尺寸：

```lua
local bytes, w, h = dwebp:webp2Image(data, "RGBA")   -- w*h*4 字节
local rgb, w, h = dwebp:webp2Image(data, "RGB")      -- w*h*3 字节
```

### `options` — 解码器设置（可选）

直接映射 libwebp 的 `WebPDecoderOptions`，均为数字字段。

| 字段 | 含义 |
| ------ | ------ |
| `use_threads` | 多线程解码 |
| `use_cropping` + `crop_left`、`crop_top`、`crop_width`、`crop_height` | 裁剪输出 |
| `use_scaling` + `scaled_width`、`scaled_height` | 缩放输出（在裁剪之后） |
| `flip` | 垂直翻转输出 |
| `bypass_filtering` | 跳过环路滤波 |
| `no_fancy_upsampling` | 使用更快的逐点升采样 |
| `dithering_strength` | 0–100，抖动强度 |
| `alpha_dithering_strength` | 0–100，透明通道抖动 |

### `info` 信息表

`dwebp:info(data)` / `dwebp:infoFromPath(path)` 返回：

| 字段 | 类型 | 含义 |
| ------ | ------ | ------ |
| `width`、`height` | 整数 | 像素尺寸 |
| `has_alpha` | 布尔 | 码流是否含透明通道 |
| `has_animation` | 布尔 | 码流是否为动画 |
| `format` | 字符串 | `"undefined"`、`"lossy"` 或 `"lossless"` |

所有失败都会抛出带描述信息的 Lua 错误。不支持动画 WebP（只解码第一帧）；
`info()` 会报告 `has_animation` 以便调用方检测。

## 安装

最简单的路径是源码构建：

```bash
make build
```

使用 LuaRocks：

```bash
luarocks make lua-webp-scm-1.rockspec
```

打 tag 的 GitHub 发布会自动产出 Linux、macOS、Windows 的预编译模块
（见 `release` workflow）；将匹配平台的文件放到 Lua 的 package path 即可。

## 许可证

MIT，见 [LICENSE](LICENSE)。vendored libwebp 树为 BSD 许可（见
`third_party/libwebp/COPYING` 和 `third_party/libwebp/PATENTS`）。

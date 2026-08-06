# lua_webp — Lua bindings for the WebP image format (cwebp/dwebp).
#
# Build/test conventions follow lua-stb: pkg-config for Lua, a portable
# Makefile that works on Linux, macOS and Windows (MSYS2), and a `make test`
# target running tests/test.lua.
#
# Dependencies (found via pkg-config):
#   - Lua >= 5.3            (e.g. liblua5.4-dev / lua@5.4 / mingw-w64-*-lua54)
#   - libwebp + libwebpdemux (e.g. libwebp-dev / brew webp / mingw-w64-*-libwebp)
#   - libpng, libjpeg, libtiff (for the PNG/JPEG/TIFF input readers)
#
# If libwebp is not found via pkg-config, the vendored source tree
# (third_party/libwebp) is built with CMake instead.

CC ?= cc
LUA_VERSION ?= 5.4
LUA_PKG ?= lua$(LUA_VERSION)
LUA_BIN ?= lua$(LUA_VERSION)
PKG_CONFIG ?= pkg-config

CFLAGS ?= -O2 -std=c99 -Wall -Wextra
CPPFLAGS += -I. -Ithird_party/libwebp $(shell $(PKG_CONFIG) --cflags $(LUA_PKG) 2>/dev/null)
LDFLAGS += $(shell $(PKG_CONFIG) --libs $(LUA_PKG) 2>/dev/null)
LIBS ?= -lm

# ---------------------------------------------------------------------------
# libwebp: system install first, vendored CMake build as fallback
# ---------------------------------------------------------------------------
WEBP_PC ?= libwebp
WEBP_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(WEBP_PC) 2>/dev/null)
WEBP_LIBS := $(shell $(PKG_CONFIG) --libs $(WEBP_PC) 2>/dev/null) \
             $(shell $(PKG_CONFIG) --libs libwebpdemux 2>/dev/null)

# The presence of the main libwebp pc file decides between a system build and
# the vendored CMake fallback (a stray libwebpdemux.pc must not count).
ifeq ($(strip $(WEBP_CFLAGS)),)
WEBP_BUILD_DIR := third_party/libwebp/build
WEBP_CFLAGS := -Ithird_party/libwebp/src
WEBP_LIBS := -L$(WEBP_BUILD_DIR) -lwebp -lwebpdemux
WEBP_TARGETS := $(WEBP_BUILD_DIR)/.built
$(info NOTE: system libwebp not found via pkg-config; building the vendored \
libwebp (third_party/libwebp) with CMake)

$(WEBP_BUILD_DIR)/.built:
	mkdir -p $(WEBP_BUILD_DIR)
	cmake -S third_party/libwebp -B $(WEBP_BUILD_DIR) \
	  -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release \
	  -DWEBP_BUILD_EXAMPLES=OFF -DWEBP_BUILD_EXTRAS=OFF \
	  -DWEBP_BUILD_CWEBP=OFF -DWEBP_BUILD_DWEBP=OFF \
	  -DWEBP_BUILD_GIF2WEBP=OFF -DWEBP_BUILD_IMG2WEBP=OFF \
	  -DWEBP_BUILD_ANIMDIFF=OFF -DWEBP_BUILD_WEBPINFO=OFF \
	  -DWEBP_BUILD_ANIM_UTILS=OFF -DWEBP_BUILD_VWEBP=OFF \
	  -DWEBP_BUILD_WEBPMUX=OFF
	# --build works with any generator (Makefiles or Ninja).
	cmake --build $(WEBP_BUILD_DIR) --target webp webpdemux
	touch $@
else
WEBP_TARGETS :=
endif

# ---------------------------------------------------------------------------
# PNG/JPEG/TIFF codecs for the vendored imageio input readers
# ---------------------------------------------------------------------------
PNG_CFLAGS := $(shell $(PKG_CONFIG) --cflags libpng 2>/dev/null)
PNG_LIBS := $(shell $(PKG_CONFIG) --libs libpng 2>/dev/null)
JPEG_CFLAGS := $(shell $(PKG_CONFIG) --cflags libjpeg 2>/dev/null)
JPEG_LIBS := $(shell $(PKG_CONFIG) --libs libjpeg 2>/dev/null)
TIFF_CFLAGS := $(shell $(PKG_CONFIG) --cflags libtiff-4 2>/dev/null)
TIFF_LIBS := $(shell $(PKG_CONFIG) --libs libtiff-4 2>/dev/null)

ifeq ($(strip $(PNG_LIBS)),)
$(error libpng development files not found via pkg-config; install libpng-dev)
endif
ifeq ($(strip $(JPEG_LIBS)),)
$(error libjpeg development files not found via pkg-config; install libjpeg-dev)
endif
ifeq ($(strip $(TIFF_LIBS)),)
$(error libtiff development files not found via pkg-config; install libtiff-dev)
endif

# imageio code compiled from the vendored libwebp glue (imageio/) needs the
# codec feature defines and headers.
IMAGEIO_CFLAGS := -DWEBP_HAVE_PNG -DWEBP_HAVE_JPEG -DWEBP_HAVE_TIFF \
  $(PNG_CFLAGS) $(JPEG_CFLAGS) $(TIFF_CFLAGS)

# ---------------------------------------------------------------------------
# Target extension / shared flags per platform
# ---------------------------------------------------------------------------
UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(OS),Windows_NT)
TARGET_EXT ?= dll
SHARED_FLAGS ?= -shared
else ifneq (,$(findstring MINGW,$(UNAME_S)))
# MSYS2/MinGW environments: uname reports MINGW64_NT-* / MSYS_NT-*.
TARGET_EXT ?= dll
SHARED_FLAGS ?= -shared
else ifneq (,$(findstring MSYS,$(UNAME_S)))
TARGET_EXT ?= dll
SHARED_FLAGS ?= -shared
else ifneq (,$(findstring Darwin,$(UNAME_S)))
TARGET_EXT ?= so
SHARED_FLAGS ?= -bundle -undefined dynamic_lookup
else
TARGET_EXT ?= so
SHARED_FLAGS ?= -shared -fPIC
endif

TARGET := lua_webp.$(TARGET_EXT)

MODULE_SOURCES := lua_webp.c lua_cwebp.c lua_dwebp.c
IMAGEIO_SOURCES := \
  third_party/libwebp/imageio/image_dec.c \
  third_party/libwebp/imageio/image_enc.c \
  third_party/libwebp/imageio/imageio_util.c \
  third_party/libwebp/imageio/jpegdec.c \
  third_party/libwebp/imageio/metadata.c \
  third_party/libwebp/imageio/pngdec.c \
  third_party/libwebp/imageio/pnmdec.c \
  third_party/libwebp/imageio/tiffdec.c \
  third_party/libwebp/imageio/webpdec.c

.PHONY: all build test fixtures install clean

all: build

build: $(TARGET)

# Used by LuaRocks' make build type (see lua-webp-scm-1.rockspec).
PREFIX ?= /usr/local
INSTALL_LIBDIR ?= $(PREFIX)/lib/lua/$(LUA_VERSION)

install: build
	mkdir -p $(DESTDIR)$(INSTALL_LIBDIR)
	cp $(TARGET) $(DESTDIR)$(INSTALL_LIBDIR)/

$(TARGET): $(MODULE_SOURCES) lua_webp.h $(IMAGEIO_SOURCES) $(WEBP_TARGETS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WEBP_CFLAGS) $(IMAGEIO_CFLAGS) $(SHARED_FLAGS) \
	  -o $@ $(MODULE_SOURCES) $(IMAGEIO_SOURCES) \
	  $(LDFLAGS) $(WEBP_LIBS) $(PNG_LIBS) $(JPEG_LIBS) $(TIFF_LIBS) $(LIBS)

test: build
	$(LUA_BIN) tests/test.lua

# Regenerates the committed test fixtures (tests/fixture.{png,webp}).
fixtures: tools/gen_fixtures
	./tools/gen_fixtures

tools/gen_fixtures: tools/gen_fixtures.c
	$(CC) $(CFLAGS) $(WEBP_CFLAGS) $(PNG_CFLAGS) -Ithird_party/libwebp \
	  -o $@ $< $(LDFLAGS) $(WEBP_LIBS) $(PNG_LIBS) $(LIBS)

clean:
	$(RM) $(TARGET)
	$(RM) -rf third_party/libwebp/build
	$(RM) tools/gen_fixtures tools/gen_fixtures.exe

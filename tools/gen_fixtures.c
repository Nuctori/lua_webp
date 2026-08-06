// Generates the tiny image fixtures used by tests/test.lua:
//
//   tests/fixture.png  - 16x16 RGBA checkerboard (R and G vary linearly,
//                         B is a checkerboard, A alternates 255/128)
//   tests/fixture.webp - lossless WebP encoding of the same pixels
//
// Build & run via `make fixtures` (requires libwebp + libpng dev packages).
//
// The pixel formula is intentionally unusual (odd RGB values, 128-alpha
// columns) so that tests can verify lossless round-trips pixel-exactly.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

#include <webp/encode.h>

#define W 16
#define H 16

static unsigned char pixels[W * H * 4];

static void build_pixels(void) {
  int x, y;
  for (y = 0; y < H; ++y) {
    for (x = 0; x < W; ++x) {
      unsigned char* const p = &pixels[(size_t)(y * W + x) * 4];
      p[0] = (unsigned char)(x * 16 + 3);               // R varies with x
      p[1] = (unsigned char)(y * 16 + 7);               // G varies with y
      p[2] = (unsigned char)(((x + y) % 2) ? 200 : 40); // checkerboard B
      p[3] = (unsigned char)(((x + y) % 3) ? 255 : 128);// alpha varies
    }
  }
}

static int write_png(const char* path) {
  FILE* const f = fopen(path, "wb");
  png_structp png;
  png_infop info;
  int y;
  if (f == NULL) return 0;
  png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png == NULL) { fclose(f); return 0; }
  info = png_create_info_struct(png);
  if (info == NULL) {
    png_destroy_write_struct(&png, NULL);
    fclose(f);
    return 0;
  }
  if (setjmp(png_jmpbuf(png))) {
    png_destroy_write_struct(&png, &info);
    fclose(f);
    return 0;
  }
  png_init_io(png, f);
  png_set_IHDR(png, info, W, H, 8, PNG_COLOR_TYPE_RGBA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png, info);
  for (y = 0; y < H; ++y) {
    png_write_row(png, &pixels[(size_t)y * W * 4]);
  }
  png_write_end(png, info);
  png_destroy_write_struct(&png, &info);
  fclose(f);
  return 1;
}

static int write_webp(const char* path) {
  FILE* const f = fopen(path, "wb");
  uint8_t* webp = NULL;
  const size_t size = WebPEncodeLosslessRGBA(pixels, W, H, W * 4, &webp);
  int ok = 0;
  if (f == NULL || webp == NULL || size == 0) {
    if (f != NULL) fclose(f);
    WebPFree(webp);
    return 0;
  }
  ok = (fwrite(webp, 1, size, f) == size);
  fclose(f);
  WebPFree(webp);
  return ok;
}

int main(void) {
  build_pixels();
  if (!write_png("tests/fixture.png")) {
    fprintf(stderr, "failed to write tests/fixture.png\n");
    return 1;
  }
  if (!write_webp("tests/fixture.webp")) {
    fprintf(stderr, "failed to write tests/fixture.webp\n");
    return 1;
  }
  printf("wrote tests/fixture.png and tests/fixture.webp (%dx%d RGBA)\n", W, H);
  return 0;
}

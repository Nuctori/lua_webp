// Generates the tiny image fixtures used by tests/test.lua:
//
//   tests/fixture.png  - 16x16 RGBA checkerboard (R and G vary linearly,
//                         B is a checkerboard, A alternates 255/128)
//   tests/fixture.webp - lossless WebP encoding of the same pixels
//   tests/fixture.tiff - TIFF encoding of the same pixels (RGBA)
//
// Build & run via `make fixtures` (requires libwebp + libpng + libtiff dev
// packages).
//
// The pixel formula is intentionally unusual (odd RGB values, 128-alpha
// columns) so that tests can verify lossless round-trips pixel-exactly.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>
#include <tiffio.h>

#include <webp/encode.h>
#include <webp/mux.h>

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

static int write_tiff(const char* path) {
  TIFF* const tiff = TIFFOpen(path, "w");
  static const uint16_t extra_samples = EXTRASAMPLE_ASSOCALPHA;
  int y;
  if (tiff == NULL) return 0;
  TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, W);
  TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, H);
  TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 4);
  TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8);
  TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  // declare the 4th channel as associated alpha (libwebp's tiffdec reads it)
  TIFFSetField(tiff, TIFFTAG_EXTRASAMPLES, 1, &extra_samples);
  TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff, 0));
  for (y = 0; y < H; ++y) {
    if (TIFFWriteScanline(tiff, &pixels[(size_t)y * W * 4], y, 0) < 0) {
      TIFFClose(tiff);
      return 0;
    }
  }
  TIFFClose(tiff);
  return 1;
}

// Writes a 2-frame animated WebP (tests/fixture-anim.webp): frame 1 at t=0
// is the checkerboard, frame 2 at t=100ms is a solid fill. Used to lock
// down the animation behavior (info().has_animation = true, decode errors).
static int write_anim_webp(const char* path) {
  WebPAnimEncoder* const enc = WebPAnimEncoderNew(W, H, NULL);
  WebPData data;
  FILE* f;
  int ok = 0;
  if (enc == NULL) return 0;
  WebPDataInit(&data);

  {
    WebPPicture pic;
    int i;
    WebPPictureInit(&pic);
    pic.width = W;
    pic.height = H;
    pic.use_argb = 1;
    if (!WebPPictureAlloc(&pic)) {
      WebPAnimEncoderDelete(enc);
      return 0;
    }
    // frame 1: same pixels as the still fixtures
    for (i = 0; i < W * H; ++i) {
      const int x = i % W, y = i / W;
      const unsigned char* const p = &pixels[(size_t)y * W * 4 + x * 4];
      pic.argb[i] = ((uint32_t)p[3] << 24) | ((uint32_t)p[0] << 16) |
                    ((uint32_t)p[1] << 8) | p[2];
    }
    if (!WebPAnimEncoderAdd(enc, &pic, 0, NULL)) {
      WebPPictureFree(&pic);
      WebPAnimEncoderDelete(enc);
      return 0;
    }
    // frame 2 (t=100ms): solid magenta
    for (i = 0; i < W * H; ++i) pic.argb[i] = 0xFFFF00FFu;
    if (!WebPAnimEncoderAdd(enc, &pic, 100, NULL)) {
      WebPPictureFree(&pic);
      WebPAnimEncoderDelete(enc);
      return 0;
    }
    WebPPictureFree(&pic);
  }

  if (!WebPAnimEncoderAssemble(enc, &data) || data.bytes == NULL) {
    WebPAnimEncoderDelete(enc);
    return 0;
  }
  f = fopen(path, "wb");
  if (f != NULL) {
    ok = (fwrite(data.bytes, 1, data.size, f) == data.size);
    fclose(f);
  }
  WebPDataClear(&data);
  WebPAnimEncoderDelete(enc);
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
  if (!write_tiff("tests/fixture.tiff")) {
    fprintf(stderr, "failed to write tests/fixture.tiff\n");
    return 1;
  }
  if (!write_anim_webp("tests/fixture-anim.webp")) {
    fprintf(stderr, "failed to write tests/fixture-anim.webp\n");
    return 1;
  }
  printf("wrote tests/fixture.{png,webp,tiff,anim.webp} (%dx%d RGBA)\n", W, H);
  return 0;
}

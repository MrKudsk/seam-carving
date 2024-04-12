#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "nob.h"
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef struct {
  uint32_t *pixels;
  int width, height, stride;
} Img;

#define IMG_AT(img, row, col) (img).pixels[(row) * (img).stride + (col)]

typedef struct {
  float *items;
  int width, height, stride;
} Mat;

#define MAT_AT(mat, row, col) (mat).items[(row) * (mat).stride + (col)]

static Mat mat_alloc(int width, int height) {
  Mat mat = {0};
  mat.items = malloc(sizeof(float) * width * height);
  assert(mat.items != NULL);
  mat.width = width;
  mat.height = height;
  mat.stride = width;
  return mat;
}

static float rgb_to_lum(uint32_t rgb) {
  float r = ((rgb >> (8 * 0)) & 0xFF) / 255.0;
  float g = ((rgb >> (8 * 1)) & 0xFF) / 255.0;
  float b = ((rgb >> (8 * 2)) & 0xFF) / 255.0;
  return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

void min_and_max(Mat mat, float *mn, float *mx) {
  *mn = FLT_MAX;
  *mx = FLT_MIN;
  for (int y = 0; y < mat.height; ++y) {
    for (int x = 0; x < mat.width; ++x) {
      float value = MAT_AT(mat, y, x);
      if (value < *mn)
        *mn = value;
      if (value > *mx)
        *mx = value;
    }
  }
}

void analyze_min_max(const char *prompt, Mat mat) {
  float mn, mx;
  min_and_max(mat, &mn, &mx);
  printf("%s: min = %f, max = %f\n", prompt, mn, mx);
}

bool dump_mat(const char *file_path, Mat mat) {
  float mn, mx;
  min_and_max(mat, &mn, &mx);

  uint32_t *pixels = NULL;
  bool result = true;

  pixels = malloc(sizeof(*pixels) * mat.width * mat.height);
  assert(pixels != NULL);

  for (int y = 0; y < mat.height; ++y) {
    for (int x = 0; x < mat.width; ++x) {
      int i = y * mat.width + x;
      float t = (MAT_AT(mat, y, x) - mn) / (mx - mn);
      uint32_t value = 255 * t;
      pixels[i] = 0xFF000000 | (value << (8 * 2) | (value << (8 * 1))) |
                  (value << (8 * 1));
    }
  }

  if (!stbi_write_png(file_path, mat.width, mat.height, 4, pixels,
                      mat.width * sizeof(*pixels))) {
    fprintf(stderr, "ERROR: could not save file %s", file_path);
    nob_return_defer(false);
  }

  printf("OK: generated %s\n", file_path);

defer:
  free(pixels);
  return result;
}

static void luminance(Img img, Mat lum) {
  assert(img.width == lum.width);
  assert(img.height == lum.height);
  for (int y = 0; y < lum.height; ++y) {
    for (int x = 0; x < lum.width; ++x) {
      MAT_AT(lum, y, x) = rgb_to_lum(IMG_AT(img, y, x));
    }
  }
}

void sobel_filter(Mat mat, Mat grad) {
  assert(mat.width == grad.width);
  assert(mat.height == grad.height);

  static float gx[3][3] = {
      {1.0, 0.0, -1.0},
      {2.0, 0.0, -2.0},
      {1.0, 0.0, -1.0},
  };

  static float gy[3][3] = {
      {1.0, 2.0, 1.0},
      {0.0, 0.0, 0.0},
      {-1.0, -2.0, -1.0},
  };

  for (int cy = 0; cy < mat.height; ++cy) {
    for (int cx = 0; cx < mat.width; ++cx) {
      float sx = 0.0;
      float sy = 0.0;
      for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
          int x = cx + dx;
          int y = cy + dy;
          float c = 0 <= x && x < mat.width && 0 <= y && y < mat.height
                        ? MAT_AT(mat, y, x)
                        : 0.0;
          sx += c * gx[dy + 1][dx + 1];
          sy += c * gy[dy + 1][dx + 1];
        }
      }
      MAT_AT(grad, cy, cx) = sqrtf(sx * sx + sy * sy);
    }
  }
}

int main() {
  const char *file_path = "images/Broadway_tower_edit.jpg";

  int width_, height_;
  uint32_t *pixels_ =
      (uint32_t *)stbi_load(file_path, &width_, &height_, NULL, 4);
  if (pixels_ == NULL) {
    fprintf(stderr, "ERROR: could not read %s\n", file_path);
    return 1;
  }

  printf("File path: %s\n", file_path);
  printf("Width: %d\n", width_);
  printf("Height: %d\n", height_);

  Img img = {
      .pixels = pixels_,
      .width = width_,
      .height = height_,
      .stride = width_,
  };

  Mat lum = mat_alloc(width_, height_);
  Mat grad = mat_alloc(width_, height_);
  Mat dp = mat_alloc(width_, height_);

  luminance(img, lum);
  analyze_min_max("lum", lum);
  dump_mat("lum.png", lum);

  sobel_filter(lum, grad);
  analyze_min_max("grad", grad);
  dump_mat("grad.png", grad);

  return 0;
}

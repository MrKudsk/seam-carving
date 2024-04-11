#include <stdint.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main() {
  const char *file_path = "images/Broadway_tower_edit.jpg";

  int width, height;
  uint32_t *pixels = (uint32_t *)stbi_load(file_path, &width, &height, NULL, 4);
  if (pixels == NULL) {
    fprintf(stderr, "ERROR: could not read %s\n", file_path);
    return 1;
  }

  printf("File path: %s\n", file_path);
  printf("Width: %d\n", width);
  printf("Height: %d\n", height);

  return 0;
}

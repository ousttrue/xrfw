#pragma once
#include <stdint.h>

int init_gles_scene();
void render_gles_scene(uint32_t colorTexture, int width, int height,
                      const float projection[16], const float view[16],
                      void *user);

#pragma once
#include <stdint.h>

int init_gles_scene();
int render_gles_scene(uint32_t colorTexture, int width, int height,
                      const float projection[16], const float view[16]);

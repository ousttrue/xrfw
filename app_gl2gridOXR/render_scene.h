#pragma once
#include <stdint.h>

int init_gles_scene();
void render_gles_scene(int view_width, int view_height,
                       const float projection[16], const float view[16]);

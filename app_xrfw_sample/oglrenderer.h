#pragma once
#include <stdint.h>

class OglRenderer {
  class OglRendererImpl *impl_ = nullptr;

public:
  OglRenderer();
  ~OglRenderer();
  OglRenderer(const OglRenderer &) = delete;
  OglRenderer &operator=(const OglRenderer &) = delete;
  void Render(uint32_t colorTexture, int width, int height,
              const float projection[16], const float view[16]);
};

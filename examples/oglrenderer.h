#pragma once
#include <array>
#include <memory>
#include <openxr/openxr.h>
#include <stdint.h>
#include <unordered_map>

class Drawable {
  uint32_t m_program{0};
  // GLint m_modelViewProjectionUniformLocation{0};
  // GLint m_vertexAttribCoords{0};
  // GLint m_vertexAttribColor{0};
  uint32_t m_vao{0};

public:
  void Render();
};

class OglRenderer {
  uint32_t m_swapchainFramebuffer{0};
  std::unordered_map<uint32_t, uint32_t> m_colorToDepthMap;
  std::array<float, 4> m_clearColor;

  std::shared_ptr<Drawable> m_drawable;

public:
  void RenderView(const XrSwapchainImageBaseHeader *swapchainImage, int x,
                  int y, int width, int height);

private:
  uint32_t GetDepthTexture(uint32_t colorTexture);
};

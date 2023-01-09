#pragma once
#include <array>
#include <memory>
// #include <openxr/openxr.h>
#include <stdint.h>
#include <unordered_map>

class OglRenderer {
  uint32_t m_swapchainFramebuffer{0};
  std::unordered_map<uint32_t, uint32_t> m_colorToDepthMap;
  std::array<float, 4> m_clearColor;

  std::shared_ptr<class OglDrawable> m_drawable;

public:
  OglRenderer();
  ~OglRenderer();
  void RenderView(uint32_t colorTexture, int x, int y, int width, int height);

private:
  uint32_t GetDepthTexture(uint32_t colorTexture);
};

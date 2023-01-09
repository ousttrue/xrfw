#pragma once
#include <array>
#include <stdint.h>
#include <unordered_map>

class OglRenderer {
  uint32_t m_swapchainFramebuffer{0};
  std::unordered_map<uint32_t, uint32_t> m_colorToDepthMap;
  std::array<float, 4> m_clearColor = {0, 0, 0, 0};

public:
  OglRenderer();
  ~OglRenderer();
  void BeginFbo(uint32_t colorTexture, int width, int height);
  void EndFbo();

private:
  uint32_t GetDepthTexture(uint32_t colorTexture);
};

void OglInitialize();

#pragma once
#ifdef XR_USE_PLATFORM_ANDROID
#include <GLES3/gl32.h>
#elif XR_USE_PLATFORM_WIN32
#include <gl/glew.h>
#else
error("no XR_USE")
#endif
#include <stdint.h>
#include <unordered_map>

class XrfwSwapchainFbo {
  uint32_t m_swapchainFramebuffer{0};
  std::unordered_map<uint32_t, uint32_t> m_colorToDepthMap;

public:
  XrfwSwapchainFbo() {
#if XR_USE_PLATFORM_WIN32
    glewInit();
#endif
    glGenFramebuffers(1, &m_swapchainFramebuffer);
  }
  ~XrfwSwapchainFbo() {
    glDeleteFramebuffers(1, &m_swapchainFramebuffer);
    for (auto &colorToDepth : m_colorToDepthMap) {
      if (colorToDepth.second != 0) {
        glDeleteTextures(1, &colorToDepth.second);
      }
    }
  }

  void Begin(uint32_t colorTexture, int width, int height,
                const float clearColor[4]) {
    if (colorTexture) {
      glBindFramebuffer(GL_FRAMEBUFFER, m_swapchainFramebuffer);
      const uint32_t depthTexture = GetDepthTexture(colorTexture);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, colorTexture, 0);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                             depthTexture, 0);
    }

    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    // Clear swapchain and depth buffer.
    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
#if XR_USE_PLATFORM_WIN32
    glClearDepth(1.0f);
#elif XR_USE_PLATFORM_ANDROID
    glClearDepthf(1.0f);
#else
    error("no XR_USE")
#endif
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
  }

  void End() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

private:
  uint32_t GetDepthTexture(uint32_t colorTexture) {
    // If a depth-stencil view has already been created for this back-buffer,
    // use it.
    auto depthBufferIt = m_colorToDepthMap.find(colorTexture);
    if (depthBufferIt != m_colorToDepthMap.end()) {
      return depthBufferIt->second;
    }

    // This back-buffer has no corresponding depth-stencil texture, so create
    // one with matching dimensions.

    GLint width;
    GLint height;
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    uint32_t depthTexture;
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    m_colorToDepthMap.insert(std::make_pair(colorTexture, depthTexture));

    return depthTexture;
  }
};

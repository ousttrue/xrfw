#include "oglrenderer.h"
#include "ogldrawable.h"
#include <Windows.h>

#include <gl/glew.h>
#include <memory>
#include <openxr/openxr_platform.h>

OglRenderer::OglRenderer() { glGenFramebuffers(1, &m_swapchainFramebuffer); }

OglRenderer::~OglRenderer()
{
  if (m_swapchainFramebuffer != 0) {
    glDeleteFramebuffers(1, &m_swapchainFramebuffer);
  }
  for (auto &colorToDepth : m_colorToDepthMap) {
    if (colorToDepth.second != 0) {
      glDeleteTextures(1, &colorToDepth.second);
    }
  }
}

void OglRenderer::RenderView(const XrSwapchainImageBaseHeader *swapchainImage,
                             int x, int y, int width, int height) {
  // CHECK(layerView.subImage.imageArrayIndex ==
  //       0);                     // Texture arrays not supported.
  // UNUSED_PARM(swapchainFormat); // Not used in this function for now.

  glBindFramebuffer(GL_FRAMEBUFFER, m_swapchainFramebuffer);

  const uint32_t colorTexture =
      reinterpret_cast<const XrSwapchainImageOpenGLKHR *>(swapchainImage)
          ->image;

  glViewport(static_cast<GLint>(x), static_cast<GLint>(y),
             static_cast<GLsizei>(width), static_cast<GLsizei>(height));

  glFrontFace(GL_CW);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  const uint32_t depthTexture = GetDepthTexture(colorTexture);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         colorTexture, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         depthTexture, 0);

  // Clear swapchain and depth buffer.
  glClearColor(m_clearColor[0], m_clearColor[1], m_clearColor[2],
               m_clearColor[3]);
  glClearDepth(1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  if (!m_drawable) {
    m_drawable = OglDrawable::Create();
  }
  m_drawable->Render();
}

uint32_t OglRenderer::GetDepthTexture(uint32_t colorTexture) {
  // If a depth-stencil view has already been created for this back-buffer, use
  // it.
  auto depthBufferIt = m_colorToDepthMap.find(colorTexture);
  if (depthBufferIt != m_colorToDepthMap.end()) {
    return depthBufferIt->second;
  }

  // This back-buffer has no corresponding depth-stencil texture, so create one
  // with matching dimensions.

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
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0,
               GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

  m_colorToDepthMap.insert(std::make_pair(colorTexture, depthTexture));

  return depthTexture;
}
#include "oglrenderer.h"
#include <Windows.h>

#include <gl/glew.h>
#include <iomanip>
#include <memory>
#include <openxr/openxr_platform.h>
#include <plog/Log.h>
#include <string_view>

std::unordered_map<uint64_t, const char *> g_glNameMap{
    {GL_RGBA8, "GL_RGBA8"},
    {GL_RGB16F, "GL_RGB16F"},
    {GL_R11F_G11F_B10F_EXT, "GL_R11F_G11F_B10F_EXT"},
    {GL_SRGB8_ALPHA8_EXT, "GL_SRGB8_ALPHA8_EXT"},
    {GL_DEPTH_COMPONENT16, "GL_DEPTH_COMPONENT16"},
    {GL_DEPTH_COMPONENT24, "GL_DEPTH_COMPONENT24"},
    {GL_DEPTH_COMPONENT32, "GL_DEPTH_COMPONENT32"},
    {GL_DEPTH24_STENCIL8, "GL_DEPTH24_STENCIL8"},
    {GL_DEPTH_COMPONENT32F, "GL_DEPTH_COMPONENT32F"},
    {GL_DEPTH32F_STENCIL8, "GL_DEPTH32F_STENCIL8"},
};

static std::string_view ToGLString(uint64_t value) {
  auto found = g_glNameMap.find(value);
  if (found != g_glNameMap.end()) {
    return found->second;
  }
  PLOG_FATAL << "0x" << std::hex << value;
  throw std::runtime_error("unknown");
}

OglRenderer::OglRenderer() { glGenFramebuffers(1, &m_swapchainFramebuffer); }

OglRenderer::~OglRenderer() {
  if (m_swapchainFramebuffer != 0) {
    glDeleteFramebuffers(1, &m_swapchainFramebuffer);
  }
  for (auto &colorToDepth : m_colorToDepthMap) {
    if (colorToDepth.second != 0) {
      glDeleteTextures(1, &colorToDepth.second);
    }
  }
}

void OglRenderer::BeginFbo(uint32_t colorTexture, int width, int height) {
  // CHECK(layerView.subImage.imageArrayIndex ==
  //       0);                     // Texture arrays not supported.
  // UNUSED_PARM(swapchainFormat); // Not used in this function for now.

  glBindFramebuffer(GL_FRAMEBUFFER, m_swapchainFramebuffer);

  glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));

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
}

void OglRenderer::EndFbo() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

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

void OglInitialize() { glewInit(); }

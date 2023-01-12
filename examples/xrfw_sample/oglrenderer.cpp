#ifdef XR_USE_PLATFORM_ANDROID
#include <GLES3/gl32.h>
#elif XR_USE_PLATFORM_WIN32
#include <gl/glew.h>
#else
error("no XR_USE")
#endif

#include "ogldrawable.h"
#include "oglrenderer.h"
#include <array>
#include <iomanip>
#include <memory>
#include <plog/Log.h>
#include <string_view>
#include <unordered_map>

std::unordered_map<uint64_t, const char *> g_glNameMap {
  {GL_RGBA8, "GL_RGBA8"}, {GL_RGB16F, "GL_RGB16F"},
      {GL_DEPTH_COMPONENT16, "GL_DEPTH_COMPONENT16"},
      {GL_DEPTH_COMPONENT24, "GL_DEPTH_COMPONENT24"},
      {GL_DEPTH24_STENCIL8, "GL_DEPTH24_STENCIL8"},
      {GL_DEPTH_COMPONENT32F, "GL_DEPTH_COMPONENT32F"},
      {GL_DEPTH32F_STENCIL8, "GL_DEPTH32F_STENCIL8"},
#if XR_USE_PLATFORM_WIN32
      {GL_R11F_G11F_B10F_EXT, "GL_R11F_G11F_B10F_EXT"},
      {GL_SRGB8_ALPHA8_EXT, "GL_SRGB8_ALPHA8_EXT"},
      {GL_DEPTH_COMPONENT32, "GL_DEPTH_COMPONENT32"},
#endif
};

static std::string_view ToGLString(uint64_t value) {
  auto found = g_glNameMap.find(value);
  if (found != g_glNameMap.end()) {
    return found->second;
  }
  PLOG_FATAL << "0x" << std::hex << value;
  throw std::runtime_error("unknown");
}

class OglRendererImpl {
  uint32_t m_swapchainFramebuffer{0};
  std::unordered_map<uint32_t, uint32_t> m_colorToDepthMap;
  std::array<float, 4> m_clearColor = {0, 0, 0, 0};
  std::shared_ptr<OglDrawable> m_drawable;
  std::vector<Cube> m_cubes;

public:
  OglRendererImpl() {
#if XR_USE_PLATFORM_WIN32
    glewInit();
#endif
    glGenFramebuffers(1, &m_swapchainFramebuffer);

    m_drawable = OglDrawable::Create();

    // For each locatable space that we want to visualize, render a 25cm
    m_cubes.push_back(Cube{
        {0, 0, 0, 1},
        {0, 0, 0},
        {0.25f, 0.25f, 0.25f},
    });
  }
  ~OglRendererImpl() {
    glDeleteFramebuffers(1, &m_swapchainFramebuffer);
    for (auto &colorToDepth : m_colorToDepthMap) {
      if (colorToDepth.second != 0) {
        glDeleteTextures(1, &colorToDepth.second);
      }
    }
  }
  void Render(uint32_t colorTexture, int width, int height,
              const float projection[16], const float view[16]) {
    BeginFbo(colorTexture, width, height);
    m_drawable->Render(projection, view, m_cubes);
    EndFbo();
  }

private:
  void BeginFbo(uint32_t colorTexture, int width, int height) {
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
    glClearColor(m_clearColor[0], m_clearColor[1], m_clearColor[2],
                 m_clearColor[3]);
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

  void EndFbo() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

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

//
// OglRenderer
//
OglRenderer::OglRenderer() : impl_(new OglRendererImpl) {}
OglRenderer::~OglRenderer() { delete (impl_); }
void OglRenderer::Render(uint32_t colorTexture, int width, int height,
                         const float projection[16], const float view[16]) {
  impl_->Render(colorTexture, width, height, projection, view);
}

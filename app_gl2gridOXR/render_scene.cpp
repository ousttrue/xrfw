#ifdef XR_USE_PLATFORM_ANDROID
#include <GLES3/gl32.h>
#elif XR_USE_PLATFORM_WIN32
#include <gl/glew.h>
#else
error("no XR_USE")
#endif

#include "render_scene.h"
#include "util_debugstr.h"
#include "util_render2d.h"
#include <algorithm>
#include <stdio.h>
#include <unordered_map>

static void draw_grid_lefttop(int win_w, int win_h) {
  float col_gray[] = {0.73f, 0.75f, 0.75f, 1.0f};
  float col_blue[] = {0.00f, 0.00f, 1.00f, 1.0f};
  float col_red[] = {1.00f, 0.00f, 0.00f, 1.0f};
  float col_cyan[] = {0.00f, 1.00f, 1.00f, 1.0f};
  float *col;
  int cx = win_w / 2;
  int cy = win_h / 2;

  set_2d_projection_matrix(win_w, win_h);

  col = col_gray;
  for (int y = 0; y < win_h; y += 10)
    draw_2d_line(0, y, win_w, y, col, 1.0f);

  for (int x = 0; x < win_w; x += 10)
    draw_2d_line(x, 0, x, win_h, col, 1.0f);

  col = col_blue;
  for (int y = 0; y < win_h; y += 100)
    draw_2d_line(0, y, win_w, y, col, 1.0f);

  for (int x = 0; x < win_w; x += 100)
    draw_2d_line(x, 0, x, win_h, col, 1.0f);

  col = col_red;
  for (int y = 0; y < win_h; y += 500)
    draw_2d_line(0, y, win_w, y, col, 1.0f);

  for (int x = 0; x < win_w; x += 500)
    draw_2d_line(x, 0, x, win_h, col, 1.0f);

  float col_green[] = {0.00f, 1.00f, 0.00f, 1.0f};
  draw_2d_line(0, cy, win_w, cy, col_green, 1.0f);
  draw_2d_line(cx, 0, cx, win_h, col_green, 1.0f);

  int radius = std::max(win_w, win_h);
  for (int r = 0; r < radius; r += 100)
    draw_2d_circle(cx, cy, r, 50, col_blue, 1.0f);

  for (int r = 0; r < radius; r += 500)
    draw_2d_circle(cx, cy, r, 50, col_red, 1.0f);
}

static void draw_grid(int win_w, int win_h) {
  float col_gray[] = {0.73f, 0.75f, 0.75f, 1.0f};
  float col_blue[] = {0.00f, 0.00f, 1.00f, 1.0f};
  float col_red[] = {1.00f, 0.00f, 0.00f, 1.0f};
  float col_cyan[] = {0.00f, 1.00f, 1.00f, 1.0f};
  float *col;
  int cx = win_w / 2;
  int cy = win_h / 2;

  set_2d_projection_matrix(win_w, win_h);

  col = col_gray;
  for (int y = 0; y < cy; y += 10) {
    draw_2d_line(0, cy + y, win_w, cy + y, col, 1.0f);
    draw_2d_line(0, cy - y, win_w, cy - y, col, 1.0f);
  }

  for (int x = 0; x < cx; x += 10) {
    draw_2d_line(cx + x, 0, cx + x, win_h, col, 1.0f);
    draw_2d_line(cx - x, 0, cx - x, win_h, col, 1.0f);
  }

  col = col_blue;
  for (int y = 0; y < cy; y += 100) {
    draw_2d_line(0, cy + y, win_w, cy + y, col, 1.0f);
    draw_2d_line(0, cy - y, win_w, cy - y, col, 1.0f);
  }

  for (int x = 0; x < cx; x += 100) {
    draw_2d_line(cx + x, 0, cx + x, win_h, col, 1.0f);
    draw_2d_line(cx - x, 0, cx - x, win_h, col, 1.0f);
  }

  col = col_red;
  for (int y = 0; y < cy; y += 500) {
    draw_2d_line(0, cy + y, win_w, cy + y, col, 1.0f);
    draw_2d_line(0, cy - y, win_w, cy - y, col, 1.0f);
  }

  for (int x = 0; x < cx; x += 500) {
    draw_2d_line(cx + x, 0, cx + x, win_h, col, 1.0f);
    draw_2d_line(cx - x, 0, cx - x, win_h, col, 1.0f);
  }

  /* circle */
  int radius = std::max(win_w, win_h);
  for (int r = 0; r < radius; r += 100)
    draw_2d_circle(cx, cy, r, 50, col_blue, 1.0f);

  for (int r = 0; r < radius; r += 500)
    draw_2d_circle(cx, cy, r, 50, col_red, 1.0f);

  /* strings */
  update_dbgstr_winsize(win_w, win_h);
  for (int x = 0; x < cx; x += 100) {
    char strbuf[128];
    sprintf(strbuf, "%d", x);
    draw_dbgstr(strbuf, cx + x, cy);

    sprintf(strbuf, "%d", -x);
    draw_dbgstr(strbuf, cx - x, cy);
  }

  for (int y = 0; y < cy; y += 100) {
    char strbuf[128];
    sprintf(strbuf, "%d", y);
    draw_dbgstr(strbuf, cx, cy + y);

    sprintf(strbuf, "%d", -y);
    draw_dbgstr(strbuf, cx, cy - y);
  }
}

int init_gles_scene() {
#if XR_USE_PLATFORM_WIN32
  glewInit();
#endif

  init_dbgstr(0, 0);
  init_2d_renderer(0, 0);

  return 0;
}

class FBO {
  uint32_t fbo_ = 0;
  std::unordered_map<uint32_t, uint32_t> m_colorToDepthMap;

public:
  void Bind(uint32_t colorTexture) {
    if (colorTexture) {
      if (fbo_ == 0) {
        glGenFramebuffers(1, &fbo_);
      }
      glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
      const uint32_t depthTexture = GetDepthTexture(colorTexture);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, colorTexture, 0);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                             depthTexture, 0);
    } else {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
  }

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
FBO g_fbo;

int render_gles_scene(uint32_t colorTexture, int width, int height,
                      const float projection[16], const float view[16]) {
  int view_x = 0;
  int view_y = 0;
  int view_w = width;
  int view_h = height;

  g_fbo.Bind(colorTexture);

  glViewport(view_x, view_y, view_w, view_h);

  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  /* ------------------------------------------- *
   *  Render
   * ------------------------------------------- */

  draw_grid(view_w, view_h);

  {
    int cx = view_w / 2;
    int cy = view_h / 2;

    char strbuf[128];
    sprintf(strbuf, "Viewport(%d, %d, %d, %d)", view_x, view_y, view_w, view_h);
    draw_dbgstr(strbuf, cx + 100, cy + 100);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return 0;
}

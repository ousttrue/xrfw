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

void render_gles_scene(int view_w, int view_h, const float projection[16],
                       const float view[16]) {
  draw_grid(view_w, view_h);
  {
    int cx = view_w / 2;
    int cy = view_h / 2;

    char strbuf[128];
    // sprintf(strbuf, "Viewport(%d, %d, %d, %d)", view_x, view_y, view_w,
    // view_h);
    draw_dbgstr(strbuf, cx + 100, cy + 100);
  }
}

#include <plog/Log.h>
#include <xrfw.h>

#include "pch.h"
#include "OpenXrProgram.h"

int main(int argc, char **argv) {
  XrfwPlatformWin32D3D11 platform;
  auto instance = platform.CreateInstance();
  if (!instance) {
    return 1;
  }

  if (!platform.InitializeGraphics()) {
    return 2;
  }

  auto graphics = sample::CreateCubeGraphics();
  // return xrfwSession(platform, &render_gles_scene, nullptr);

  xrfwDestroyInstance();
  return 0;
}

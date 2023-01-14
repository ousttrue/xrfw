#ifdef XR_USE_PLATFORM_WIN32
#ifdef XR_USE_GRAPHICS_API_D3D11
#include <xrfw.h>

struct PlatformWin32D3D11Impl {};

XrfwPlatformWin32D3D11::XrfwPlatformWin32D3D11(struct android_app *)
    : impl_(new PlatformWin32D3D11Impl) {}
XrfwPlatformWin32D3D11::~XrfwPlatformWin32D3D11() { delete impl_; }
XrInstance XrfwPlatformWin32D3D11::CreateInstance() {
  XrfwInitialization init;
  // xrfwPlatformWin32OpenGL(&init);
  return xrfwCreateInstance(&init);
}
bool XrfwPlatformWin32D3D11::InitializeGraphics() { return {}; }
XrSession XrfwPlatformWin32D3D11::CreateSession(XrfwSwapchains *swapchains) {
  return {};
}
bool XrfwPlatformWin32D3D11::BeginFrame() { return {}; }
void XrfwPlatformWin32D3D11::EndFrame(RenderFunc render, void *user) {}
uint32_t XrfwPlatformWin32D3D11::CastTexture(
    const XrSwapchainImageBaseHeader *swapchainImage) {
  return {};
}
void XrfwPlatformWin32D3D11::Sleep(std::chrono::milliseconds ms) { {}; }
#endif
#endif
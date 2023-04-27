#include "d3drenderer.h"
#include "xr_ext_hand_tracking.h"
#include <plog/Log.h>
#include <xrfw.h>
#include <xrfw_impl_win32_d3d11.h>

struct Context
{
  D3DRenderer m_d3d;
  ExtHandTracking m_ext;
  std::shared_ptr<ExtHandTracker> m_trackerL;
  std::shared_ptr<ExtHandTracker> m_trackerR;
  std::vector<cuber::Instance> m_instances;

  Context(XrInstance instance,
          XrSystemId system,
          const winrt::com_ptr<ID3D11Device>& device)
    : m_d3d(device)
    , m_ext(instance, system)
  {
  }

  void UpdateJoints(std::span<const XrHandJointLocationEXT> joints,
                    const DirectX::XMFLOAT4& color)
  {
    const XrSpaceLocationFlags isValid =
      XR_SPACE_LOCATION_ORIENTATION_VALID_BIT |
      XR_SPACE_LOCATION_POSITION_VALID_BIT;

    for (auto& joint : joints) {
      if ((joint.locationFlags & isValid) != 0) {
        auto size = joint.radius * 2;
        auto s = DirectX::XMMatrixScaling(size, size, size);
        auto r = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(
          (const DirectX::XMFLOAT4*)&joint.pose.orientation));
        auto t = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(
          (const DirectX::XMFLOAT3*)&joint.pose.position));

        m_instances.push_back({});
        DirectX::XMStoreFloat4x4(&m_instances.back().Matrix, s * r * t);
        m_instances.back().Color = color;
      }
    }
  }

  void Render(XrTime time,
              const XrSwapchainImageBaseHeader* swapchainImage,
              const XrfwSwapchains& info,
              const float leftProjection[16],
              const float leftView[16],
              const float rightProjection[16],
              const float rightView[16])
  {
    // update
    m_instances.clear();
    auto space = xrfwAppSpace();
    UpdateJoints(m_trackerL->Update(time, space), { 1, 0, 0, 1 });
    UpdateJoints(m_trackerR->Update(time, space), { 0, 0, 1, 1 });

    // render
    m_d3d.Render(swapchainImage,
                 info,
                 leftProjection,
                 leftView,
                 rightProjection,
                 rightView,
                 m_instances);
  }

  static const XrCompositionLayerBaseHeader* Render(
    XrTime time,
    const XrSwapchainImageBaseHeader* swapchainImage,
    const XrSwapchainImageBaseHeader*,
    const XrfwSwapchains& info,
    const float projection[16],
    const float view[16],
    const float rightProjection[16],
    const float rightView[16],
    void* user)
  {
    auto context = ((Context*)user);
    context->Render(
      time, swapchainImage, info, projection, view, rightProjection, rightView);
    return nullptr;
  }

  static void Begin(XrSession session, void* user)
  {
    auto context = ((Context*)user);
    context->m_trackerL =
      std::make_shared<ExtHandTracker>(context->m_ext, session, true);
    context->m_trackerR =
      std::make_shared<ExtHandTracker>(context->m_ext, session, false);
  }

  static void End(XrSession session, void* user)
  {
    auto context = ((Context*)user);
    context->m_trackerL = {};
    context->m_trackerR = {};
  }
};

int
main(int argc, char** argv)
{
  // instance
  XrfwPlatformWin32D3D11 platform;
  auto [instance, system] =
    platform.CreateInstance(ExtHandTracking::extensions());
  if (!instance) {
    return 1;
  }
  auto device = platform.InitializeGraphics();
  if (!device) {
    return 2;
  }

  // session
  Context context(instance, system, device);
  auto ret = xrfwSession(
    platform, &Context::Render, &context, &Context::Begin, &Context::End);

  xrfwDestroyInstance();
  return ret;
}

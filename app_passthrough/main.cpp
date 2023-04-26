#include <cuber/dx/DxCubeStereoRenderer.h>
#include <plog/Log.h>
#include <xrfw.h>
#include <xrfw_impl_win32_d3d11.h>

struct FBPassthrough
{
  static std::span<const char*> extensions()
  {
    static const char* s_extensions[] = {
      XR_FB_PASSTHROUGH_EXTENSION_NAME,
      XR_FB_TRIANGLE_MESH_EXTENSION_NAME // optional
    };
    return s_extensions;
  }
  static bool SystemSupportsPassthrough(XrInstance instance,
                                        XRFormFactor formFactor)
  {
    XrSystemPassthroughProperties2FB passthroughSystemProperties{
      XR_TYPE_SYSTEM_PASSTHROUGH_PROPERTIES2_FB
    };
    XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES,
                                         &passthroughSystemProperties };

    XrSystemGetInfo systemGetInfo{ XR_TYPE_SYSTEM_GET_INFO };
    systemGetInfo.formFactor = formFactor;

    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    xrGetSystem(instance, &systemGetInfo, &systemId);
    xrGetSystemProperties(instance, systemId, &systemProperties);

    // XR_PASSTHROUGH_CAPABILITY_COLOR_BIT_FB
    return (passthroughSystemProperties.capabilities &
            XR_PASSTHROUGH_CAPABILITY_BIT_FB) ==
           XR_PASSTHROUGH_CAPABILITY_BIT_FB;
  }

  PFN_xrCreatePassthroughFB pfnXrCreatePassthroughFBX = nullptr;

  FBPassthrough(XrInstance instance, XrSystemId system)
  {
    XrResult result =
      xrGetInstanceProcAddr(instance,
                            "xrCreatePassthroughFB",
                            (PFN_xrVoidFunction*)(&pfnXrCreatePassthroughFBX));
    if (XR_FAILED(result)) {
      PLOG_ERROR
        << "Failed to obtain the function pointer for xrCreatePassthroughFB.";
    }
  }
};

struct FBPassthroughFeature
{
  const FBPassthrough& m_ext;
  XrSession m_session;
  XrPassthroughFB passthroughFeature = XR_NULL_HANDLE;

  FBPassthroughFeature(const FBPassthrough& ext, XrSession session)
    : m_ext(ext)
    , m_session(session)
  {
    XrPassthroughCreateInfoFB passthroughCreateInfo = {
      .type = XR_TYPE_PASSTHROUGH_CREATE_INFO_FB,
      .flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB,
    };

    XrResult result = m_ext.pfnXrCreatePassthroughFBX(
      session, &passthroughCreateInfo, &passthroughFeature);
    if (XR_FAILED(result)) {
      PLOG_ERROR << "Failed to create a passthrough feature.";
    }
  }

  void CreateLayer()
  {
    // Create and run passthrough layer
    XrPassthroughLayerFB passthroughLayer = XR_NULL_HANDLE;

    XrPassthroughLayerCreateInfoFB layerCreateInfo = {
      .type = XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB,
      .passthrough = passthroughFeature,
      .flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB,
      .purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB,
    };

    XrResult result = m_ext.pfnXrCreatePassthroughLayerFBX(
      m_session, &layerCreateInfo, &passthroughLayer);
    if (XR_FAILED(result)) {
      PLOG_ERROR << "Failed to create and start a passthrough layer";
    }
  }

  // void Start()
  // {
  //   // Start the feature manually
  //   XrResult result = m_ext.pfnXrPassthroughStartFB(passthroughFeature);
  //   if (XR_FAILED(result)) {
  //     LOG("Failed to start a passthrough feature.\n");
  //   }
  // }
};

struct ExtHandTracking
{
  static std::span<const char*> extensions()
  {
    static const char* s_extensions[] = {
      XR_EXT_HAND_TRACKING_EXTENSION_NAME,
    };
    return s_extensions;
  }
  PFN_xrCreateHandTrackerEXT xrCreateHandTrackerEXT_ = nullptr;
  PFN_xrDestroyHandTrackerEXT xrDestroyHandTrackerEXT_ = nullptr;
  PFN_xrLocateHandJointsEXT xrLocateHandJointsEXT_ = nullptr;
  ExtHandTracking(XrInstance instance, XrSystemId system)
  {
    // Inspect hand tracking system properties
    XrSystemHandTrackingPropertiesEXT handTrackingSystemProperties{
      XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT
    };
    XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES,
                                         &handTrackingSystemProperties };
    if (FAILED(xrGetSystemProperties(instance, system, &systemProperties))) {
      PLOG_ERROR << "xrGetSystemProperties";
    }
    if (!handTrackingSystemProperties.supportsHandTracking) {
      // The system does not support hand tracking
      PLOG_ERROR << "xrGetSystemProperties "
                    "XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT FAILED.";
    } else {
      PLOG_INFO << "xrGetSystemProperties "
                   "XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT OK "
                   "- initiallizing hand tracking...";
    }

    if (FAILED(xrGetInstanceProcAddr(
          instance,
          "xrCreateHandTrackerEXT",
          (PFN_xrVoidFunction*)(&xrCreateHandTrackerEXT_)))) {
      PLOG_ERROR << "xrGetInstanceProcAddr: xrCreateHandTrackerEXT";
    }
    if (FAILED(xrGetInstanceProcAddr(
          instance,
          "xrDestroyHandTrackerEXT",
          (PFN_xrVoidFunction*)(&xrDestroyHandTrackerEXT_)))) {
      PLOG_ERROR << "xrGetInstanceProcAddr: xrDestroyHandTrackerEXT_";
    }
    if (FAILED(xrGetInstanceProcAddr(
          instance,
          "xrLocateHandJointsEXT",
          (PFN_xrVoidFunction*)(&xrLocateHandJointsEXT_)))) {
      PLOG_ERROR << "xrGetInstanceProcAddr: xrLocateHandJointsEXT_";
    }
  }
};

struct ExtHandTracker
{
  const ExtHandTracking& m_ext;
  XrHandTrackerEXT handTrackerL_ = XR_NULL_HANDLE;
  XrHandJointLocationEXT jointLocationsL_[XR_HAND_JOINT_COUNT_EXT];
  XrHandJointLocationsEXT locationsL_ = {};
  DirectX::XMFLOAT4 m_color;

  ExtHandTracker(const ExtHandTracking& ext, XrSession session, bool isLeft)
    : m_ext(ext)
  {
    XrHandTrackerCreateInfoEXT createInfo{
      .type = XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT,
      .hand = isLeft ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT,
      .handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT,
    };
    if (FAILED(m_ext.xrCreateHandTrackerEXT_(
          session, &createInfo, &handTrackerL_))) {
      PLOG_ERROR << "xrCreateHandTrackerEXT";
    }
    if (isLeft) {
      m_color = { 1, 0, 0, 1 };
    } else {
      m_color = { 0, 0, 1, 1 };
    }
  }

  ~ExtHandTracker()
  {
    if (FAILED(m_ext.xrDestroyHandTrackerEXT_(handTrackerL_))) {
      PLOG_ERROR << "xrDestroyHandTrackerEXT_";
    }
  }

  void Update(XrTime time,
              XrSpace space,
              std::vector<cuber::Instance>& instances)
  {
    locationsL_ = XrHandJointLocationsEXT{
      .type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT,
      .next = nullptr,
      .jointCount = XR_HAND_JOINT_COUNT_EXT,
      .jointLocations = jointLocationsL_,
    };

    XrHandJointsLocateInfoEXT locateInfoL{
      .type = XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT,
      .baseSpace = space,
      .time = time,
    };
    if (FAILED(m_ext.xrLocateHandJointsEXT_(
          handTrackerL_, &locateInfoL, &locationsL_))) {
      PLOG_ERROR << "xrLocateHandJointsEXT";
      return;
    }

    // Tracked joints and computed joints can all be valid
    const XrSpaceLocationFlags isValid =
      XR_SPACE_LOCATION_ORIENTATION_VALID_BIT |
      XR_SPACE_LOCATION_POSITION_VALID_BIT;

    if (locationsL_.isActive) {
      for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; ++i) {
        if ((jointLocationsL_[i].locationFlags & isValid) != 0) {
          instances.push_back({});
          auto size = jointLocationsL_[i].radius * 2;
          auto s = DirectX::XMMatrixScaling(size, size, size);
          auto r = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(
            (const DirectX::XMFLOAT4*)&jointLocationsL_[i].pose.orientation));
          auto t = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(
            (const DirectX::XMFLOAT3*)&jointLocationsL_[i].pose.position));
          DirectX::XMStoreFloat4x4(&instances.back().Matrix, s * r * t);
          instances.back().Color = m_color;
        }
      }
    }
  }
};

struct Context
{
  static const int MAX_NUM_EYES = 2;

  // d3d11
  winrt::com_ptr<ID3D11Device> m_device;
  winrt::com_ptr<ID3D11DeviceContext> m_deviceContext;
  winrt::com_ptr<ID3D11Texture2D> m_depthStencil;
  winrt::com_ptr<ID3D11DepthStencilView> m_dsv;
  cuber::dx11::DxCubeStereoRenderer graphics;
  std::vector<cuber::Instance> instances;

  ExtHandTracking m_ext;
  std::shared_ptr<ExtHandTracker> m_trackerL;
  std::shared_ptr<ExtHandTracker> m_trackerR;

  Context(XrInstance instance,
          XrSystemId system,
          const winrt::com_ptr<ID3D11Device>& device)
    : m_device(device)
    , graphics(device)
    , m_ext(instance, system)
  {
    m_device->GetImmediateContext(m_deviceContext.put());
  }

  bool CreateDepth(UINT width, UINT height)
  {
    if (m_dsv) {
      return true;
    }

    D3D11_TEXTURE2D_DESC depthDesc = {  
      .Width = width,
      .Height = height,
      .MipLevels = 1,
      .ArraySize = 2,
      .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
      .SampleDesc={
        .Count = 1,
        .Quality = 0,
      },
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_DEPTH_STENCIL,
      .CPUAccessFlags = 0,
      .MiscFlags = 0,
    };
    auto hr = m_device->CreateTexture2D(&depthDesc, NULL, m_depthStencil.put());
    if (FAILED(hr)) {
      return false;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {
      .Format = depthDesc.Format,
      .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY,
      .Texture2DArray = { 
        .MipSlice = 0, 
        .FirstArraySlice=0,
        .ArraySize=2,
      },
    };
    hr = m_device->CreateDepthStencilView(
      m_depthStencil.get(), &dsvDesc, m_dsv.put());
    if (FAILED(hr)) {
      return false;
    }

    return true;
  }

  void SetRTV(ID3D11Texture2D* colorTexture,
              int width,
              int height,
              DXGI_FORMAT colorSwapchainFormat)
  {
    CD3D11_VIEWPORT viewport{
      (float)0,
      (float)0,
      (float)width,
      (float)height,
    };
    m_deviceContext->RSSetViewports(1, &viewport);

    CreateDepth(width, height);

    // Create RenderTargetView with the original swapchain format (swapchain
    // image is typeless).
    winrt::com_ptr<ID3D11RenderTargetView> renderTargetView;
    const CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(
      D3D11_RTV_DIMENSION_TEXTURE2DARRAY, colorSwapchainFormat);
    m_device->CreateRenderTargetView(
      colorTexture, &renderTargetViewDesc, renderTargetView.put());

    // Clear swapchain and depth buffer. NOTE: This will clear the entire render
    // target view, not just the specified view.
    constexpr DirectX::XMFLOAT4 renderTargetClearColor = {
      0.184313729f,
      0.309803933f,
      0.309803933f,
      1.000000000f,
    };
    m_deviceContext->ClearRenderTargetView(renderTargetView.get(),
                                           &renderTargetClearColor.x);
    float depthClearValue = 1.0f;
    m_deviceContext->ClearDepthStencilView(
      m_dsv.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depthClearValue, 0);
    // m_deviceContext->OMSetDepthStencilState(nullptr, 0);

    ID3D11RenderTargetView* renderTargets[] = { renderTargetView.get() };
    m_deviceContext->OMSetRenderTargets(
      (UINT)std::size(renderTargets), renderTargets, m_dsv.get());
  }

  void Render(XrTime time,
              const XrSwapchainImageBaseHeader* swapchainImage,
              const XrfwSwapchains& info,
              const float projection[16],
              const float view[16],
              const float rightProjection[16],
              const float rightView[16])
  {
    // update
    instances.clear();
    auto space = xrfwAppSpace();
    m_trackerL->Update(time, space, instances);
    m_trackerR->Update(time, space, instances);

    // render
    SetRTV(xrfwCastTextureD3D11(swapchainImage),
           info.width,
           info.height,
           (DXGI_FORMAT)info.format);
    graphics.Render(projection,
                    view,
                    rightProjection,
                    rightView,
                    instances.data(),
                    instances.size());
  }

  static void Render(XrTime time,
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

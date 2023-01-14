// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

namespace sample {
struct Cube {
  xr::SpaceHandle Space{};
  std::optional<XrPosef>
      PoseInSpace{}; // Relative pose in cube Space. Default to identity.
  XrVector3f Scale{0.1f, 0.1f, 0.1f};

  XrPosef PoseInAppSpace =
      xr::math::Pose::Identity(); // Cube pose in app space that gets updated
                                  // every frame
};

struct IOpenXrProgram {
  virtual ~IOpenXrProgram() = default;
  virtual void Run() = 0;
};

struct IGraphicsPluginD3D11 {
  virtual ~IGraphicsPluginD3D11() = default;

  // Create an instance of this graphics api for the provided instance and
  // systemId.
  virtual ID3D11Device *
  InitializeDevice(LUID adapterLuid,
                   const std::vector<D3D_FEATURE_LEVEL> &featureLevels) = 0;

  // List of color pixel formats supported by this app.
  virtual const std::vector<DXGI_FORMAT> &SupportedColorFormats() const = 0;
  virtual const std::vector<DXGI_FORMAT> &SupportedDepthFormats() const = 0;

  // Render to swapchain images using stereo image array
  virtual void RenderView(ID3D11Texture2D *vrpt,
                          DXGI_FORMAT colorSwapchainFormat, int width,
                          int height, const float projection[16],
                          const float view[16], const float rightProjection[16],
                          const float rightView[16]) = 0;
};

std::unique_ptr<IGraphicsPluginD3D11> CreateCubeGraphics();
std::unique_ptr<IOpenXrProgram>
CreateOpenXrProgram(std::string applicationName,
                    std::unique_ptr<IGraphicsPluginD3D11> graphicsPlugin);

} // namespace sample

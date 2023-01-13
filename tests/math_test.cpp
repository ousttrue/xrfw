#include <catch2/catch_test_macros.hpp>

#include "../../src/xr_linear.h"
#include <corecrt_math.h>
#include <cstdint>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <numbers>
#include <openxr/openxr.h>
#include <span>

const float EPSILON = 1e-3f;
static bool CompareMatrix(std::span<const float> lhs,
                          std::span<const float> rhs) {
  if (lhs.size() != 16) {
    return false;
  }
  if (rhs.size() != 16) {
    return false;
  }
  for (int i = 0; i < 16; ++i) {
    auto delta = fabs(lhs[i] - rhs[i]);
    if (delta > EPSILON) {
      return false;
    }
  }
  return true;
}

static float deg2rad(float src) {
  return static_cast<float>(src / 180.0 * std::numbers::pi);
}

// From
// https://github.com/jherico/Vulkan/blob/experimental/openxr/examples/vr_openxr/vr_openxr.cpp
// (MIT License) Code written by Bradley Austin Davis ("jherico")
inline XrFovf toTanFovf(const XrFovf &fov) {
  return {tanf(fov.angleLeft), tanf(fov.angleRight), tanf(fov.angleUp),
          tanf(fov.angleDown)};
}

// https://github.com/jglrxavpok/Carrot/blob/55332acf03730d133a813bd2fc572c54ff955037/engine/engine/utils/conversions.h
inline glm::mat4 toGlm(const XrFovf &fov, float nearZ = 0.01f,
                       float farZ = 10000.0f) {
  auto tanFov = toTanFovf(fov);
  const auto &tanAngleRight = tanFov.angleRight;
  const auto &tanAngleLeft = tanFov.angleLeft;
  const auto &tanAngleUp = tanFov.angleUp;
  const auto &tanAngleDown = tanFov.angleDown;

  const float tanAngleWidth = tanAngleRight - tanAngleLeft;
  const float tanAngleHeight = (tanAngleUp - tanAngleDown);
  const float offsetZ = 0;

  glm::mat4 resultm{};
  float *result = &resultm[0][0];
  // normal projection
  result[0] = 2 / tanAngleWidth;
  result[4] = 0;
  result[8] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
  result[12] = 0;

  result[1] = 0;
  result[5] = 2 / tanAngleHeight;
  result[9] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
  result[13] = 0;

  result[2] = 0;
  result[6] = 0;
  result[10] = -(farZ + offsetZ) / (farZ - nearZ);
  result[14] = -2 * (farZ * (nearZ + offsetZ)) / (farZ - nearZ);

  result[3] = 0;
  result[7] = 0;
  result[11] = -1;
  result[15] = 0;

  return resultm;
}

TEST_CASE("projection", "[matrix]") {

  float identity[] = {
      1, 0, 0, 0, //
      0, 1, 0, 0, //
      0, 0, 1, 0, //
      0, 0, 0, 1, //
  };

  // const auto &pose = layerView.pose;
  glm::mat4 m(1);
  REQUIRE(CompareMatrix(std::span{identity, 16}, std::span{&m[0][0], 16}));

  XrFovf frustum = {
      .angleLeft = deg2rad(-15),
      .angleRight = deg2rad(15),
      .angleUp = deg2rad(15),
      .angleDown = deg2rad(-15),
  };
  XrMatrix4x4f proj;
  XrMatrix4x4f_CreateProjectionFov(&proj, GRAPHICS_OPENGL, frustum, 0.05f,
                                   100.0f);

  auto glm_proj = toGlm(frustum, 0.05f, 100.0f);

  REQUIRE(CompareMatrix(std::span{proj.m, 16}, std::span{&glm_proj[0][0], 16}));

  auto glm_proj2 = glm::frustumRH_NO(
      tanf(frustum.angleLeft) * 0.05f, tanf(frustum.angleRight) * 0.05f,
      tanf(frustum.angleDown) * 0.05f, tanf(frustum.angleUp) * 0.05f, 0.05f,
      100.0f);
  REQUIRE(
      CompareMatrix(std::span{proj.m, 16}, std::span{&glm_proj2[0][0], 16}));
}

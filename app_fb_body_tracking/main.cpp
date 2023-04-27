#include <asio.hpp>

#include "d3drenderer.h"
#include "xr_fb_body_tracking.h"
#include <plog/Log.h>
#include <vrm/scene.h>
#include <vrm/srht_sender.h>
#include <xrfw.h>
#include <xrfw_impl_win32_d3d11.h>

static std::optional<libvrm::vrm::HumanBones>
ToVrmBone(XrBodyJointFB joint)
{
  switch (joint) {
    case XR_BODY_JOINT_HIPS_FB:
      return libvrm::vrm::HumanBones::hips;
    case XR_BODY_JOINT_SPINE_LOWER_FB:
      return libvrm::vrm::HumanBones::spine;
    case XR_BODY_JOINT_SPINE_MIDDLE_FB:
      return libvrm::vrm::HumanBones::chest;
    // case XR_BODY_JOINT_SPINE_UPPER_FB:
    //   return libvrm::vrm::HumanBones::hips;
    case XR_BODY_JOINT_CHEST_FB:
      return libvrm::vrm::HumanBones::upperChest;
    case XR_BODY_JOINT_NECK_FB:
      return libvrm::vrm::HumanBones::neck;
    case XR_BODY_JOINT_HEAD_FB:
      return libvrm::vrm::HumanBones::head;

    case XR_BODY_JOINT_LEFT_SHOULDER_FB:
      return libvrm::vrm::HumanBones::leftShoulder;
    // case XR_BODY_JOINT_LEFT_SCAPULA_FB:
    //   return libvrm::vrm::HumanBones::hips;
    case XR_BODY_JOINT_LEFT_ARM_UPPER_FB:
      return libvrm::vrm::HumanBones::leftUpperArm;
    case XR_BODY_JOINT_LEFT_ARM_LOWER_FB:
      return libvrm::vrm::HumanBones::leftLowerArm;
    // case XR_BODY_JOINT_LEFT_HAND_WRIST_TWIST_FB:
    //   return libvrm::vrm::HumanBones::hips;
    case XR_BODY_JOINT_RIGHT_SHOULDER_FB:
      return libvrm::vrm::HumanBones::rightShoulder;
    // case XR_BODY_JOINT_RIGHT_SCAPULA_FB:
    //   return libvrm::vrm::HumanBones::hips;
    case XR_BODY_JOINT_RIGHT_ARM_UPPER_FB:
      return libvrm::vrm::HumanBones::rightUpperArm;
    case XR_BODY_JOINT_RIGHT_ARM_LOWER_FB:
      return libvrm::vrm::HumanBones::rightLowerArm;
    // case XR_BODY_JOINT_RIGHT_HAND_WRIST_TWIST_FB:
    //   return libvrm::vrm::HumanBones::hips;

    // case XR_BODY_JOINT_LEFT_HAND_PALM_FB:
    //   return libvrm::vrm::HumanBones::hips;
    case XR_BODY_JOINT_LEFT_HAND_WRIST_FB:
      return libvrm::vrm::HumanBones::leftHand;
    case XR_BODY_JOINT_LEFT_HAND_THUMB_METACARPAL_FB:
      return libvrm::vrm::HumanBones::leftThumbMetacarpal;
    case XR_BODY_JOINT_LEFT_HAND_THUMB_PROXIMAL_FB:
      return libvrm::vrm::HumanBones::leftThumbProximal;
    case XR_BODY_JOINT_LEFT_HAND_THUMB_DISTAL_FB:
      return libvrm::vrm::HumanBones::leftThumbDistal;
    // case XR_BODY_JOINT_LEFT_HAND_THUMB_TIP_FB:
    //   return libvrm::vrm::HumanBones::hips;
    // case XR_BODY_JOINT_LEFT_HAND_INDEX_METACARPAL_FB:
    //   return libvrm::vrm::HumanBones::hips;
    case XR_BODY_JOINT_LEFT_HAND_INDEX_PROXIMAL_FB:
      return libvrm::vrm::HumanBones::leftIndexProximal;
    case XR_BODY_JOINT_LEFT_HAND_INDEX_INTERMEDIATE_FB:
      return libvrm::vrm::HumanBones::leftIndexIntermediate;
    case XR_BODY_JOINT_LEFT_HAND_INDEX_DISTAL_FB:
      return libvrm::vrm::HumanBones::leftIndexDistal;
    // case XR_BODY_JOINT_LEFT_HAND_INDEX_TIP_FB:
    //   return libvrm::vrm::HumanBones::hips;
    // case XR_BODY_JOINT_LEFT_HAND_MIDDLE_METACARPAL_FB:
    //   return libvrm::vrm::HumanBones::hips;
    case XR_BODY_JOINT_LEFT_HAND_MIDDLE_PROXIMAL_FB:
      return libvrm::vrm::HumanBones::leftMiddleProximal;
    case XR_BODY_JOINT_LEFT_HAND_MIDDLE_INTERMEDIATE_FB:
      return libvrm::vrm::HumanBones::leftMiddleIntermediate;
    case XR_BODY_JOINT_LEFT_HAND_MIDDLE_DISTAL_FB:
      return libvrm::vrm::HumanBones::leftMiddleDistal;
    // case XR_BODY_JOINT_LEFT_HAND_MIDDLE_TIP_FB:
    //   return libvrm::vrm::HumanBones::hips;
    // case XR_BODY_JOINT_LEFT_HAND_RING_METACARPAL_FB:
    //   return libvrm::vrm::HumanBones::hips;
    case XR_BODY_JOINT_LEFT_HAND_RING_PROXIMAL_FB:
      return libvrm::vrm::HumanBones::leftRingProximal;
    case XR_BODY_JOINT_LEFT_HAND_RING_INTERMEDIATE_FB:
      return libvrm::vrm::HumanBones::leftRingIntermediate;
    case XR_BODY_JOINT_LEFT_HAND_RING_DISTAL_FB:
      return libvrm::vrm::HumanBones::leftRingDistal;
    // case XR_BODY_JOINT_LEFT_HAND_RING_TIP_FB:
    //   return libvrm::vrm::HumanBones::hips;
    // case XR_BODY_JOINT_LEFT_HAND_LITTLE_METACARPAL_FB:
    //   return libvrm::vrm::HumanBones::hips;
    case XR_BODY_JOINT_LEFT_HAND_LITTLE_PROXIMAL_FB:
      return libvrm::vrm::HumanBones::leftLittleProximal;
    case XR_BODY_JOINT_LEFT_HAND_LITTLE_INTERMEDIATE_FB:
      return libvrm::vrm::HumanBones::leftLittleIntermediate;
    case XR_BODY_JOINT_LEFT_HAND_LITTLE_DISTAL_FB:
      return libvrm::vrm::HumanBones::leftLittleDistal;
    // case XR_BODY_JOINT_LEFT_HAND_LITTLE_TIP_FB:
    //   return libvrm::vrm::HumanBones::hips;
    // case XR_BODY_JOINT_RIGHT_HAND_PALM_FB:
    //   return libvrm::vrm::HumanBones::hips;
    case XR_BODY_JOINT_RIGHT_HAND_WRIST_FB:
      return libvrm::vrm::HumanBones::rightHand;
    case XR_BODY_JOINT_RIGHT_HAND_THUMB_METACARPAL_FB:
      return libvrm::vrm::HumanBones::rightThumbMetacarpal;
    case XR_BODY_JOINT_RIGHT_HAND_THUMB_PROXIMAL_FB:
      return libvrm::vrm::HumanBones::rightThumbProximal;
    case XR_BODY_JOINT_RIGHT_HAND_THUMB_DISTAL_FB:
      return libvrm::vrm::HumanBones::rightThumbDistal;
    // case XR_BODY_JOINT_RIGHT_HAND_THUMB_TIP_FB:
    //   return libvrm::vrm::HumanBones::hips;
    // case XR_BODY_JOINT_RIGHT_HAND_INDEX_METACARPAL_FB:
    //   return libvrm::vrm::HumanBones::hips;
    case XR_BODY_JOINT_RIGHT_HAND_INDEX_PROXIMAL_FB:
      return libvrm::vrm::HumanBones::rightIndexProximal;
    case XR_BODY_JOINT_RIGHT_HAND_INDEX_INTERMEDIATE_FB:
      return libvrm::vrm::HumanBones::rightIndexIntermediate;
    case XR_BODY_JOINT_RIGHT_HAND_INDEX_DISTAL_FB:
      return libvrm::vrm::HumanBones::rightIndexDistal;
    // case XR_BODY_JOINT_RIGHT_HAND_INDEX_TIP_FB:
    //   return libvrm::vrm::HumanBones::hips;
    // case XR_BODY_JOINT_RIGHT_HAND_MIDDLE_METACARPAL_FB:
    //   return libvrm::vrm::HumanBones::hips;
    case XR_BODY_JOINT_RIGHT_HAND_MIDDLE_PROXIMAL_FB:
      return libvrm::vrm::HumanBones::rightMiddleProximal;
    case XR_BODY_JOINT_RIGHT_HAND_MIDDLE_INTERMEDIATE_FB:
      return libvrm::vrm::HumanBones::rightMiddleIntermediate;
    case XR_BODY_JOINT_RIGHT_HAND_MIDDLE_DISTAL_FB:
      return libvrm::vrm::HumanBones::rightMiddleDistal;
    // case XR_BODY_JOINT_RIGHT_HAND_MIDDLE_TIP_FB:
    //   return libvrm::vrm::HumanBones::hips;
    // case XR_BODY_JOINT_RIGHT_HAND_RING_METACARPAL_FB:
    //   return libvrm::vrm::HumanBones::hips;
    case XR_BODY_JOINT_RIGHT_HAND_RING_PROXIMAL_FB:
      return libvrm::vrm::HumanBones::rightRingProximal;
    case XR_BODY_JOINT_RIGHT_HAND_RING_INTERMEDIATE_FB:
      return libvrm::vrm::HumanBones::rightRingIntermediate;
    case XR_BODY_JOINT_RIGHT_HAND_RING_DISTAL_FB:
      return libvrm::vrm::HumanBones::rightRingDistal;
    // case XR_BODY_JOINT_RIGHT_HAND_RING_TIP_FB:
    //   return libvrm::vrm::HumanBones::hips;
    // case XR_BODY_JOINT_RIGHT_HAND_LITTLE_METACARPAL_FB:
    //   return libvrm::vrm::HumanBones::hips;
    case XR_BODY_JOINT_RIGHT_HAND_LITTLE_PROXIMAL_FB:
      return libvrm::vrm::HumanBones::rightLittleProximal;
    case XR_BODY_JOINT_RIGHT_HAND_LITTLE_INTERMEDIATE_FB:
      return libvrm::vrm::HumanBones::rightLittleIntermediate;
    case XR_BODY_JOINT_RIGHT_HAND_LITTLE_DISTAL_FB:
      return libvrm::vrm::HumanBones::rightLittleDistal;
    // case XR_BODY_JOINT_RIGHT_HAND_LITTLE_TIP_FB:
    //   return libvrm::vrm::HumanBones::hips;
    default:
      return {};
  }
}

struct Context
{
  D3DRenderer m_d3d;
  FbBodyTracking m_ext;
  std::shared_ptr<FbBodyTracker> m_tracker;
  std::vector<cuber::Instance> m_instances;

  std::shared_ptr<libvrm::gltf::Scene> m_scene;
  asio::io_context m_io;
  uint32_t m_skeletonId = 0;
  asio::ip::udp::endpoint m_ep;
  libvrm::srht::UdpSender m_sender;

  Context(XrInstance instance,
          XrSystemId system,
          const winrt::com_ptr<ID3D11Device>& device)
    : m_d3d(device)
    , m_ext(instance, system)
    , m_ep(asio::ip::address::from_string("127.0.0.1"), 54345)
    , m_sender(m_io)
  {
    m_scene = std::make_shared<libvrm::gltf::Scene>();
  }

  void UpdateSkeleton(std::span<const XrBodySkeletonJointFB> joints)
  {
    // updae scene
    m_scene->Clear();
    for (size_t i = 0; i < joints.size(); ++i) {
      // auto& joint = joints[i];
      char name[64];
      snprintf(name, sizeof(name), "joint_%zu", i);
      auto ptr = std::make_shared<libvrm::gltf::Node>(name);
      m_scene->m_nodes.push_back(ptr);
    }
    for (size_t i = 0; i < joints.size(); ++i) {
      auto& joint = joints[i];
      auto& node = m_scene->m_nodes[i];
      if (i == 0) {
        m_scene->m_roots.push_back(node);
      }
      if (joint.parentJoint >= 0 &&
          joint.parentJoint < m_scene->m_nodes.size()) {
        libvrm::gltf::Node::AddChild(m_scene->m_nodes[joint.parentJoint], node);
        if (auto bone = ToVrmBone((XrBodyJointFB)joint.joint)) {
          node->Humanoid = libvrm::gltf::NodeHumanoidInfo{
            .HumanBone = *bone,
          };
        }
      }
      auto r = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(
        (const DirectX::XMFLOAT4*)&joint.pose.orientation));
      auto t = DirectX::XMMatrixTranslationFromVector(
        DirectX::XMLoadFloat3((const DirectX::XMFLOAT3*)&joint.pose.position));
      node->SetWorldMatrix(r * t);
    }
    m_scene->InitializeNodes();

    m_sender.SendSkeleton(m_ep, ++m_skeletonId, m_scene);
  }

  void UpdateJoints(std::span<const XrBodyJointLocationFB> joints,
                    const DirectX::XMFLOAT4& color)
  {
    // update scene
    const XrSpaceLocationFlags isValid =
      XR_SPACE_LOCATION_ORIENTATION_VALID_BIT |
      XR_SPACE_LOCATION_POSITION_VALID_BIT;
    for (size_t i = 0; i < joints.size(); ++i) {
      auto& joint = joints[i];
      auto& node = m_scene->m_nodes[i];
      if ((joint.locationFlags & isValid) != 0) {
        // auto size = 0.01f; // joint.radius * 2;
        // auto s = DirectX::XMMatrixScaling(size, size, size);
        auto r = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(
          (const DirectX::XMFLOAT4*)&joint.pose.orientation));
        auto t = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(
          (const DirectX::XMFLOAT3*)&joint.pose.position));
        // m_instances.push_back({});
        // DirectX::XMStoreFloat4x4(&m_instances.back().Matrix, s * r * t);
        // m_instances.back().Color = color;
        node->SetWorldMatrix(r * t);
      }
    }

    m_instances.clear();
    m_scene->m_roots[0]->UpdateShapeInstanceRecursive(
      DirectX::XMMatrixIdentity(),
      [this](const libvrm::gltf::Instance& instance) {
        m_instances.push_back({
          .Matrix = instance.Matrix,
          .Color = { 1, 1, 1, 1 },
        });
      });

    m_sender.SendFrame(m_ep, m_skeletonId, m_scene);
  }

  void Render(XrTime time,
              const XrSwapchainImageBaseHeader* swapchainImage,
              const XrfwSwapchains& info,
              const float leftProjection[16],
              const float leftView[16],
              const float rightProjection[16],
              const float rightView[16])
  {
    m_io.poll();

    // update
    m_instances.clear();
    auto space = xrfwAppSpace();
    auto result = m_tracker->Update(time, space);
    if (result.SkeletonUpdated) {
      UpdateSkeleton(m_tracker->SkeletonJoints());
    }
    if (result.JointsIsActive) {
      UpdateJoints(m_tracker->Joints(), { 1, 1, 1, 1 });
    }

    // render
    m_d3d.Render(swapchainImage,
                 info,
                 leftProjection,
                 leftView,
                 rightProjection,
                 rightView,
                 m_instances);
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
    context->m_tracker =
      std::make_shared<FbBodyTracker>(context->m_ext, session);
  }

  static void End(XrSession session, void* user)
  {
    auto context = ((Context*)user);
    context->m_tracker = {};
  }
};

int
main(int argc, char** argv)
{
  // instance
  XrfwPlatformWin32D3D11 platform;
  auto [instance, system] =
    platform.CreateInstance(FbBodyTracking::extensions());
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

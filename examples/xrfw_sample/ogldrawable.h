#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <openxr/openxr.h>
#include <span>
#include <stdint.h>

struct Cube {
  glm::quat rotation;
  glm::vec3 translation;
  glm::vec3 scale;
};

struct Frustum {
  float left;
  float right;
  float top;
  float bottom;
};

class OglDrawable {
  uint32_t m_program{0};
  uint32_t m_modelViewProjectionUniformLocation{0};
  uint32_t m_vertexAttribCoords{0};
  uint32_t m_vertexAttribColor{0};
  uint32_t m_vao{0};
  uint32_t m_cubeVertexBuffer{0};
  uint32_t m_cubeIndexBuffer{0};

  OglDrawable();
  bool Load();

public:
  ~OglDrawable();
  OglDrawable(const OglDrawable &) = delete;
  OglDrawable &operator=(const OglDrawable &) = delete;
  static std::shared_ptr<OglDrawable> Create();
  void Render(const float projection[16], const float view[16],
              std::span<Cube> cubes);
};

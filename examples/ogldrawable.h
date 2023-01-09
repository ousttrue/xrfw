#pragma once
#include <memory>
#include <stdint.h>

class OglDrawable {
  uint32_t m_program{0};
  uint32_t m_modelViewProjectionUniformLocation{0};
  uint32_t m_vertexAttribCoords{0};
  uint32_t m_vertexAttribColor{0};
  uint32_t m_vao{0};
  uint32_t m_cubeVertexBuffer{0};
  uint32_t m_cubeIndexBuffer{0};

  OglDrawable();

public:
  ~OglDrawable();
  OglDrawable(const OglDrawable &) = delete;
  OglDrawable &operator=(const OglDrawable &) = delete;
  static std::shared_ptr<OglDrawable> Create();
  bool Load();
  void Render();
};

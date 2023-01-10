#include "ogldrawable.h"
#include <gl/glew.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <openxr/openxr.h>
#include <stdlib.h>

static const char *VertexShaderGlsl = R"_(
    #version 410

    in vec3 VertexPos;
    in vec3 VertexColor;

    out vec3 PSVertexColor;

    uniform mat4 ModelViewProjection;

    void main() {
       gl_Position = ModelViewProjection * vec4(VertexPos, 1.0);
       PSVertexColor = VertexColor;
    }
    )_";

static const char *FragmentShaderGlsl = R"_(
    #version 410

    in vec3 PSVertexColor;
    out vec4 FragColor;

    void main() {
       FragColor = vec4(PSVertexColor, 1);
    }
    )_";

namespace Geometry {

struct Vertex {
  glm::vec3 Position;
  glm::vec3 Color;
};

constexpr glm::vec3 Red{1, 0, 0};
constexpr glm::vec3 DarkRed{0.25f, 0, 0};
constexpr glm::vec3 Green{0, 1, 0};
constexpr glm::vec3 DarkGreen{0, 0.25f, 0};
constexpr glm::vec3 Blue{0, 0, 1};
constexpr glm::vec3 DarkBlue{0, 0, 0.25f};

// Vertices for a 1x1x1 meter cube. (Left/Right, Top/Bottom, Front/Back)
constexpr glm::vec3 LBB{-0.5f, -0.5f, -0.5f};
constexpr glm::vec3 LBF{-0.5f, -0.5f, 0.5f};
constexpr glm::vec3 LTB{-0.5f, 0.5f, -0.5f};
constexpr glm::vec3 LTF{-0.5f, 0.5f, 0.5f};
constexpr glm::vec3 RBB{0.5f, -0.5f, -0.5f};
constexpr glm::vec3 RBF{0.5f, -0.5f, 0.5f};
constexpr glm::vec3 RTB{0.5f, 0.5f, -0.5f};
constexpr glm::vec3 RTF{0.5f, 0.5f, 0.5f};

#define CUBE_SIDE(V1, V2, V3, V4, V5, V6, COLOR)                               \
  {V1, COLOR}, {V2, COLOR}, {V3, COLOR}, {V4, COLOR}, {V5, COLOR}, {V6, COLOR},

constexpr Vertex c_cubeVertices[] = {
    CUBE_SIDE(LTB, LBF, LBB, LTB, LTF, LBF, DarkRed)   // -X
    CUBE_SIDE(RTB, RBB, RBF, RTB, RBF, RTF, Red)       // +X
    CUBE_SIDE(LBB, LBF, RBF, LBB, RBF, RBB, DarkGreen) // -Y
    CUBE_SIDE(LTB, RTB, RTF, LTB, RTF, LTF, Green)     // +Y
    CUBE_SIDE(LBB, RBB, RTB, LBB, RTB, LTB, DarkBlue)  // -Z
    CUBE_SIDE(LBF, LTF, RTF, LBF, RTF, RBF, Blue)      // +Z
};

// Winding order is clockwise. Each side uses a different color.
constexpr unsigned short c_cubeIndices[] = {
    0,  1,  2,  3,  4,  5,  // -X
    6,  7,  8,  9,  10, 11, // +X
    12, 13, 14, 15, 16, 17, // -Y
    18, 19, 20, 21, 22, 23, // +Y
    24, 25, 26, 27, 28, 29, // -Z
    30, 31, 32, 33, 34, 35, // +Z
};

} // namespace Geometry

OglDrawable::OglDrawable() {}

OglDrawable::~OglDrawable() {
  if (m_program != 0) {
    glDeleteProgram(m_program);
  }
  if (m_vao != 0) {
    glDeleteVertexArrays(1, &m_vao);
  }
  if (m_cubeVertexBuffer != 0) {
    glDeleteBuffers(1, &m_cubeVertexBuffer);
  }
  if (m_cubeIndexBuffer != 0) {
    glDeleteBuffers(1, &m_cubeIndexBuffer);
  }
}

void OglDrawable::Render(const float projection[16], const float view[16],
                         std::span<Cube> cubes) {
  // Set shaders and uniform variables.
  glUseProgram(m_program);

  auto &v = *((glm::mat4 *)view);
  auto &p = *((glm::mat4 *)projection);
  auto vp = p * v;

  // Set cube primitive data.
  glBindVertexArray(m_vao);

  // Render each cube
  for (const Cube &cube : cubes) {
    // Compute the model-view-projection transform and set it..
    auto t = glm::translate(glm::mat4(1), cube.translation);
    auto r = glm::toMat4(cube.rotation);
    auto s = glm::scale(glm::mat4(1), cube.scale);
    auto model = t * r * s;
    auto mvp = vp * model;
    glUniformMatrix4fv(m_modelViewProjectionUniformLocation, 1, GL_FALSE,
                       reinterpret_cast<const GLfloat *>(&mvp));

    // Draw the cube.
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(_countof(Geometry::c_cubeIndices)),
                   GL_UNSIGNED_SHORT, nullptr);
  }

  glBindVertexArray(0);
  glUseProgram(0);
}

std::shared_ptr<OglDrawable> OglDrawable::Create() {
  auto ptr = std::shared_ptr<OglDrawable>(new OglDrawable);
  if (!ptr->Load()) {
    return nullptr;
  }
  return ptr;
}

bool OglDrawable::Load() {
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &VertexShaderGlsl, nullptr);
  glCompileShader(vertexShader);
  // CheckShader(vertexShader);

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &FragmentShaderGlsl, nullptr);
  glCompileShader(fragmentShader);
  // CheckShader(fragmentShader);

  m_program = glCreateProgram();
  glAttachShader(m_program, vertexShader);
  glAttachShader(m_program, fragmentShader);
  glLinkProgram(m_program);
  // CheckProgram(m_program);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  m_modelViewProjectionUniformLocation =
      glGetUniformLocation(m_program, "ModelViewProjection");

  m_vertexAttribCoords = glGetAttribLocation(m_program, "VertexPos");
  m_vertexAttribColor = glGetAttribLocation(m_program, "VertexColor");

  glGenBuffers(1, &m_cubeVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_cubeVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Geometry::c_cubeVertices),
               Geometry::c_cubeVertices, GL_STATIC_DRAW);

  glGenBuffers(1, &m_cubeIndexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeIndexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Geometry::c_cubeIndices),
               Geometry::c_cubeIndices, GL_STATIC_DRAW);

  glGenVertexArrays(1, &m_vao);
  glBindVertexArray(m_vao);
  glEnableVertexAttribArray(m_vertexAttribCoords);
  glEnableVertexAttribArray(m_vertexAttribColor);
  glBindBuffer(GL_ARRAY_BUFFER, m_cubeVertexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeIndexBuffer);
  glVertexAttribPointer(m_vertexAttribCoords, 3, GL_FLOAT, GL_FALSE,
                        sizeof(Geometry::Vertex), nullptr);
  glVertexAttribPointer(m_vertexAttribColor, 3, GL_FLOAT, GL_FALSE,
                        sizeof(Geometry::Vertex),
                        reinterpret_cast<const void *>(sizeof(XrVector3f)));
  return true;
}

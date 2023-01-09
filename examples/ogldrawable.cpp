#include "ogldrawable.h"
#include "geometry.h"
#include <gl/glew.h>

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

void OglDrawable::Render() {
  // Set shaders and uniform variables.
  glUseProgram(m_program);

  //   const auto &pose = layerView.pose;
  //   XrMatrix4x4f proj;
  //   XrMatrix4x4f_CreateProjectionFov(&proj, GRAPHICS_OPENGL, layerView.fov,
  //   0.05f,
  //                                    100.0f);
  //   XrMatrix4x4f toView;
  //   XrVector3f scale{1.f, 1.f, 1.f};
  //   XrMatrix4x4f_CreateTranslationRotationScale(&toView, &pose.position,
  //                                               &pose.orientation, &scale);
  //   XrMatrix4x4f view;
  //   XrMatrix4x4f_InvertRigidBody(&view, &toView);
  //   XrMatrix4x4f vp;
  //   XrMatrix4x4f_Multiply(&vp, &proj, &view);

  //   // Set cube primitive data.
  //   glBindVertexArray(m_vao);

  //   // Render each cube
  //   for (const Cube &cube : cubes) {
  //     // Compute the model-view-projection transform and set it..
  //     XrMatrix4x4f model;
  //     XrMatrix4x4f_CreateTranslationRotationScale(
  //         &model, &cube.Pose.position, &cube.Pose.orientation, &cube.Scale);
  //     XrMatrix4x4f mvp;
  //     XrMatrix4x4f_Multiply(&mvp, &vp, &model);
  //     glUniformMatrix4fv(m_modelViewProjectionUniformLocation, 1, GL_FALSE,
  //                        reinterpret_cast<const GLfloat *>(&mvp));

  //     // Draw the cube.
  //     glDrawElements(GL_TRIANGLES,
  //                    static_cast<GLsizei>(ArraySize(Geometry::c_cubeIndices)),
  //                    GL_UNSIGNED_SHORT, nullptr);
  //   }

  //   glBindVertexArray(0);
  //   glUseProgram(0);
  //   glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

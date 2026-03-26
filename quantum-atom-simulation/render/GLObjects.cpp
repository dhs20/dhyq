#include "quantum/render/GLObjects.h"

#include <utility>

namespace quantum::render {
namespace {

GLuint compileShader(GLenum type, const char* source, std::string& errorMessage) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        errorMessage.resize(static_cast<std::size_t>(length));
        glGetShaderInfoLog(shader, length, nullptr, errorMessage.data());
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

} // namespace

ShaderProgram::~ShaderProgram() {
    destroy();
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : program_(std::exchange(other.program_, 0)) {}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this != &other) {
        destroy();
        program_ = std::exchange(other.program_, 0);
    }
    return *this;
}

bool ShaderProgram::create(const char* vertexSource, const char* fragmentSource, std::string& errorMessage) {
    destroy();
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource, errorMessage);
    if (vertexShader == 0) {
        return false;
    }
    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource, errorMessage);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vertexShader);
    glAttachShader(program_, fragmentShader);
    glLinkProgram(program_);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint status = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length = 0;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &length);
        errorMessage.resize(static_cast<std::size_t>(length));
        glGetProgramInfoLog(program_, length, nullptr, errorMessage.data());
        destroy();
        return false;
    }
    return true;
}

void ShaderProgram::destroy() {
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
}

void ShaderProgram::use() const {
    glUseProgram(program_);
}

GLuint ShaderProgram::id() const {
    return program_;
}

Framebuffer::~Framebuffer() {
    destroy();
}

bool Framebuffer::create(int width, int height) {
    destroy();
    width_ = width;
    height_ = height;

    glGenFramebuffers(1, &framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

    glGenTextures(1, &colorTexture_);
    glBindTexture(GL_TEXTURE_2D, colorTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture_, 0);

    glGenRenderbuffers(1, &depthRenderbuffer_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width_, height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer_);

    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return complete;
}

void Framebuffer::destroy() {
    if (depthRenderbuffer_ != 0) {
        glDeleteRenderbuffers(1, &depthRenderbuffer_);
        depthRenderbuffer_ = 0;
    }
    if (colorTexture_ != 0) {
        glDeleteTextures(1, &colorTexture_);
        colorTexture_ = 0;
    }
    if (framebuffer_ != 0) {
        glDeleteFramebuffers(1, &framebuffer_);
        framebuffer_ = 0;
    }
}

void Framebuffer::resize(int width, int height) {
    if (width == width_ && height == height_) {
        return;
    }
    create(width, height);
}

void Framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
}

void Framebuffer::bindDefault() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint Framebuffer::colorTexture() const {
    return colorTexture_;
}

int Framebuffer::width() const {
    return width_;
}

int Framebuffer::height() const {
    return height_;
}

Mesh::~Mesh() {
    destroy();
}

Mesh::Mesh(Mesh&& other) noexcept
    : vao_(std::exchange(other.vao_, 0)),
      vbo_(std::exchange(other.vbo_, 0)),
      ebo_(std::exchange(other.ebo_, 0)),
      vertexCount_(std::exchange(other.vertexCount_, 0)),
      indexCount_(std::exchange(other.indexCount_, 0)),
      floatsPerVertex_(std::exchange(other.floatsPerVertex_, 0)),
      primitiveType_(std::exchange(other.primitiveType_, GL_TRIANGLES)),
      indexed_(std::exchange(other.indexed_, false)) {}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        destroy();
        vao_ = std::exchange(other.vao_, 0);
        vbo_ = std::exchange(other.vbo_, 0);
        ebo_ = std::exchange(other.ebo_, 0);
        vertexCount_ = std::exchange(other.vertexCount_, 0);
        indexCount_ = std::exchange(other.indexCount_, 0);
        floatsPerVertex_ = std::exchange(other.floatsPerVertex_, 0);
        primitiveType_ = std::exchange(other.primitiveType_, GL_TRIANGLES);
        indexed_ = std::exchange(other.indexed_, false);
    }
    return *this;
}

void Mesh::upload(const std::vector<float>& vertices,
                  int floatsPerVertex,
                  GLenum primitiveType,
                  const std::vector<unsigned int>& indices) {
    destroy();
    floatsPerVertex_ = floatsPerVertex;
    primitiveType_ = primitiveType;
    indexed_ = !indices.empty();
    vertexCount_ = static_cast<GLsizei>(vertices.size() / static_cast<std::size_t>(floatsPerVertex_));
    indexCount_ = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, floatsPerVertex_ * static_cast<GLsizei>(sizeof(float)), nullptr);

    if (floatsPerVertex_ >= 6) {
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,
                              3,
                              GL_FLOAT,
                              GL_FALSE,
                              floatsPerVertex_ * static_cast<GLsizei>(sizeof(float)),
                              reinterpret_cast<void*>(3 * sizeof(float)));
    }

    if (indexed_) {
        glGenBuffers(1, &ebo_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                     indices.data(),
                     GL_STATIC_DRAW);
    }

    glBindVertexArray(0);
}

void Mesh::destroy() {
    if (ebo_ != 0) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
}

void Mesh::draw() const {
    glBindVertexArray(vao_);
    if (indexed_) {
        glDrawElements(primitiveType_, indexCount_, GL_UNSIGNED_INT, nullptr);
    } else {
        glDrawArrays(primitiveType_, 0, vertexCount_);
    }
    glBindVertexArray(0);
}

Texture3D::~Texture3D() {
    destroy();
}

void Texture3D::destroy() {
    if (texture_ != 0) {
        glDeleteTextures(1, &texture_);
        texture_ = 0;
    }
}

bool Texture3D::uploadDensityPhase(int resolution, const std::vector<float>& voxels) {
    if (texture_ == 0) {
        glGenTextures(1, &texture_);
    }
    resolution_ = resolution;
    glBindTexture(GL_TEXTURE_3D, texture_);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D,
                 0,
                 GL_RG32F,
                 resolution_,
                 resolution_,
                 resolution_,
                 0,
                 GL_RG,
                 GL_FLOAT,
                 voxels.data());
    glBindTexture(GL_TEXTURE_3D, 0);
    return true;
}

void Texture3D::bind(GLenum textureUnit) const {
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_3D, texture_);
}

GLuint Texture3D::id() const {
    return texture_;
}

} // namespace quantum::render

#pragma once

#include <GL/glew.h>

#include <string>
#include <vector>

namespace quantum::render {

class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    bool create(const char* vertexSource, const char* fragmentSource, std::string& errorMessage);
    void destroy();
    void use() const;
    [[nodiscard]] GLuint id() const;

private:
    GLuint program_ = 0;
};

class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer();

    bool create(int width, int height);
    void destroy();
    void resize(int width, int height);
    void bind() const;
    static void bindDefault();

    [[nodiscard]] GLuint colorTexture() const;
    [[nodiscard]] int width() const;
    [[nodiscard]] int height() const;

private:
    GLuint framebuffer_ = 0;
    GLuint colorTexture_ = 0;
    GLuint depthRenderbuffer_ = 0;
    int width_ = 0;
    int height_ = 0;
};

class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void upload(const std::vector<float>& vertices,
                int floatsPerVertex,
                GLenum primitiveType,
                const std::vector<unsigned int>& indices = {});
    void destroy();
    void draw() const;

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;
    GLsizei vertexCount_ = 0;
    GLsizei indexCount_ = 0;
    int floatsPerVertex_ = 0;
    GLenum primitiveType_ = GL_TRIANGLES;
    bool indexed_ = false;
};

class Texture3D {
public:
    Texture3D() = default;
    ~Texture3D();

    void destroy();
    bool uploadDensityPhase(int resolution, const std::vector<float>& voxels);
    void bind(GLenum textureUnit) const;
    [[nodiscard]] GLuint id() const;

private:
    GLuint texture_ = 0;
    int resolution_ = 0;
};

} // namespace quantum::render

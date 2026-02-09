#ifndef SHADER_H
#define SHADER_H
#define GLM_ENABALE_EXPERIMENTAL

#include <glad/include/glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
private:
    std::string loadSourceFromFile(const std::string& path) {
        std::ifstream file(path);
        std::stringstream buffer;

        if (!file.is_open()) {
            std::cerr << "[ShaderProgram] Failed to open file: " << path << std::endl;
            return "";
        }

        buffer << file.rdbuf();
        file.close();
        return buffer.str();
    }

    GLuint compileStage(GLenum type, const std::string& source) {
        GLuint shader = glCreateShader(type);
        const char* src = source.c_str();

        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (!success) {
            char log[1024];
            glGetShaderInfoLog(shader, 1024, nullptr, log);
            std::cerr << "[ShaderProgram] Compilation error:\n" << log << std::endl;
        }

        return shader;
    }

    void linkProgram(GLuint vertex, GLuint fragment) {
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);

        GLint success;
        glGetProgramiv(ID, GL_LINK_STATUS, &success);

        if (!success) {
            char log[1024];
            glGetProgramInfoLog(ID, 1024, nullptr, log);
            std::cerr << "[ShaderProgram] Linking error:\n" << log << std::endl;
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
public:
	GLuint ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const std::string& vertexPath, const std::string& fragmentPath) {
        std::string vertexSource = loadSourceFromFile(vertexPath);
        std::string fragmentSource = loadSourceFromFile(fragmentPath);

        GLuint vertexShader = compileStage(GL_VERTEX_SHADER, vertexSource);
        GLuint fragmentShader = compileStage(GL_FRAGMENT_SHADER, fragmentSource);

        linkProgram(vertexShader, fragmentShader);
    }
    // activate the shader
    // ------------------------------------------------------------------------
    void use()
    {
        glUseProgram(ID);
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setVec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const std::string& name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string& name, const glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
};


#endif

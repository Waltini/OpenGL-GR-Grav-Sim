#pragma once

#include "shaders_c.h"
#include <glad/glad.h>
#include "stb_image.h"
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <filesystem>

namespace fs = std::filesystem;

class Skybox {
public:
    Skybox()
        : gen(rd())
    {
        setupBuffers();
        regenerate(); // pick a random skybox on creation
    }

    ~Skybox()
    {
        glDeleteVertexArrays(1, &skyboxVAO);
        glDeleteBuffers(1, &skyboxVBO);
        glDeleteTextures(1, &textureID);
    }

    /// Call this whenever you want a new random skybox
    void regenerate(bool flag = false)
    {
        if (textureID != 0)
            glDeleteTextures(1, &textureID);

        index = randomIndex();
        auto faces = buildFaces(index);
        textureID = loadCubemap(faces);

        lightPos = randomVectorWithMagnitude(30.0f, 1000.0f);

        std::cout << "Loaded skybox #" << index << std::endl;
    }

    void draw(Shader& skyboxShader) {
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default
    }

    GLuint getTextureID() const { return textureID; }
    GLuint getVAO() const { return skyboxVAO; }
    glm::vec3 getLightPos() const { return lightPos; }
    int getIndex() const { return index; }

private:
    GLuint skyboxVAO = 0;
    GLuint skyboxVBO = 0;
    GLuint textureID = 0;

    glm::vec3 lightPos;

    int index;

    std::random_device rd;
    std::mt19937 gen;

    static constexpr int SKYBOX_COUNT = 31;

    const float vertices[108] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    void setupBuffers()
    {
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);

        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    }

    int randomIndex()
    {
        std::uniform_int_distribution<int> dist(0, SKYBOX_COUNT - 1);
        return dist(gen);
    }

    glm::vec3 randomVectorWithMagnitude(float minMag, float maxMag)
    {
        std::uniform_real_distribution<float> magDist(minMag, maxMag);
        std::uniform_real_distribution<float> unitDist(-1.0f, 1.0f);
        std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());

        float magnitude = magDist(gen);

        // Spherical coordinates (uniform on sphere)
        float z = unitDist(gen);
        float theta = angleDist(gen);

        float r = std::sqrt(1.0f - z * z);

        return magnitude * glm::vec3(
            r * std::cos(theta),
            z,
            r * std::sin(theta)
        );
    }

    std::vector<fs::path> buildFaces(int index)
    {
        std::cout << fs::current_path() << std::endl;

        fs::path base = fs::path("assets") / "textures" / "skyboxes" / std::to_string(index);

        return {
            base / "right.png",
            base / "left.png",
            base / "top.png",
            base / "bottom.png",
            base / "front.png",
            base / "back.png"
        };
    }

    unsigned int loadCubemap(const std::vector<fs::path>& faces)
    {
        unsigned int texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

        int width, height, channels;
        stbi_set_flip_vertically_on_load(false);

        for (unsigned int i = 0; i < faces.size(); i++)
        {
            unsigned char* data =
                stbi_load(faces[i].string().c_str(), &width, &height, &channels, 0);

            if (data)
            {
                GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
                glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0,
                    format,
                    width,
                    height,
                    0,
                    format,
                    GL_UNSIGNED_BYTE,
                    data
                );
            }
            else
            {
                std::cerr << "Failed to load cubemap face: "
                    << faces[i] << std::endl;
            }
            stbi_image_free(data);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return texID;
    }
};

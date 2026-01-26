#pragma once

#define M_PI        3.14159265358979323846264338327950288   /* pi */

#include "shaders_c.h"
#include <vector>
#include <cmath>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace objects {

    namespace detail {}

    class cube {
    public:
        cube(
            float length,
            glm::vec3 colour,
            glm::vec3 position
        )
        {
            m_colour = glm::vec4(colour, 1.0f);
            setTransform(position, length);
            setupBuffers();
        }
        ~cube()
        {
            glDeleteVertexArrays(1, &c_VAO);
            glDeleteBuffers(1, &c_VBO);
        }

        void setTransform(const glm::vec3& pos, float length) {
            m_model = glm::translate(glm::mat4(1.0f), pos);
            m_model = glm::scale(m_model, glm::vec3{ length });
        }

        void draw(Shader* shader) {
            shader->use();

            shader->setMat4("model", m_model);
            shader->setVec3("colour", m_colour);

            glBindVertexArray(c_VAO);

            // Cone sides
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

    private:
        GLuint c_VBO, c_VAO;

        glm::mat4 m_model{ 1.0f };
        glm::vec4 m_colour{ 1.0f };

        void setupBuffers()
        {
            float vertices[] = { // Creates a cube
            -0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f, -0.5f,  0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,

            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,

             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,

            -0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f,  0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f
            };

            // Graphical Buffers and Arrays
            glGenVertexArrays(1, &c_VAO);
            glGenBuffers(1, &c_VBO);

            glBindVertexArray(c_VAO);

            glBindBuffer(GL_ARRAY_BUFFER, c_VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


            // position attribute
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
        }
    };

    class arrow {
    public:
        arrow(
            const glm::vec3& position,
            const glm::vec3& magnitude,
            const glm::vec3& color,
            float body_radius,
            const int sections
        )
            : colour(color),
            radius_offset(body_radius),
            a_position(position),
            a_direction(glm::normalize(magnitude)),
            neck_baseHeight((glm::length(magnitude) * vectorScalar) + radius_offset)
        {
            if (glm::length2(magnitude) < 0.0625) { draw_f = false; }
            generateMesh(sections);

            setupBuffers();

            if (0.0625f < glm::length2(magnitude) < 1.0f) {
                neck_radialScale = 1.0f / 3.0f;
                head_lengthScale = 1.0f / 3.0f;
                head_radialScale = 1.0f / 3.0f;
            }
            else if (glm::length2(magnitude) < 4.0f) {
                float val = glm::length(magnitude);
                neck_radialScale = (2.0f / 3.0f) * val - (1.0f / 3.0f);
                head_lengthScale = (2.0f / 3.0f) * val - (1.0f / 3.0f);
                head_radialScale = (2.0f / 3.0f) * val - (1.0f / 3.0f);
            }

            updateModelMatrices();
        }
        ~arrow()
        {
            glDeleteVertexArrays(1, &m_VAO);
            glDeleteBuffers(1, &m_VBO);
        }

        void transform(const glm::vec3& position, const glm::vec3& magnitude, const float radius, const float thickness = 0.0f) {
            if (glm::length2(magnitude) < 0.0625) { draw_f = false; }
            else { draw_f = true; }

            radius_offset = radius;
            a_position = position;
            a_direction = glm::normalize(magnitude);

            neck_lengthScale = ((glm::length(magnitude) * vectorScalar) + radius) / neck_baseHeight;
            neck_radialScale = 1.0f;
            head_lengthScale = 1.0f;
            head_radialScale = 1.0f;

            if (0.0625f < glm::length2(magnitude) < 1.0f) {
                neck_radialScale = 2.0f / 3.0f;
                head_lengthScale = 2.0f / 3.0f;
                head_radialScale = 2.0f / 3.0f;
            }
            else if (glm::length2(magnitude) < 4.0f) {
                float val = glm::length(magnitude);
                neck_radialScale = (1.0f / 3.0f) * val + (1.0f / 3.0f);
                head_lengthScale = (1.0f / 3.0f) * val + (1.0f / 3.0f);
                head_radialScale = (1.0f / 3.0f) * val + (1.0f / 3.0f);
            }

            if (thickness > 0.0f) {
                neck_radialScale = thickness;
                head_lengthScale = thickness;
                head_radialScale = thickness;
            }

            updateModelMatrices();
        }

        void draw(Shader* shader) {
            if (draw_f) {
                shader->use();

                shader->setVec3("colour", colour);

                glBindVertexArray(m_VAO);

                // Head
                shader->setMat4("model", head_model);

                // Cone sides
                glDrawArrays(GL_TRIANGLES, 0, head_sideVertexCount);

                // Base
                glDrawArrays(
                    GL_TRIANGLE_FAN,
                    head_sideVertexCount,
                    head_baseVertexCount
                );

                // Neck
                shader->setMat4("model", neck_model);

                // Sides
                glDrawArrays(GL_TRIANGLES, neck_offset, neck_sideVertexCount);
            }
        }


    private:
        GLuint m_VBO, m_VAO;
        bool draw_f = true;

        // --------------------------------------------------
        // Matrices and Vertices
        // --------------------------------------------------
        glm::mat4 neck_model{ 1.0f };
        glm::mat4 head_model{ 1.0f };
        std::vector<glm::vec3> m_vertices;

        // --------------------------------------------------
        // Characteristic Vectors
        // --------------------------------------------------
        glm::vec3 a_position{ 0.0f };
        glm::vec3 a_direction{ 0.0f, 1.0f, 0.0f };
        glm::vec3 colour{ 1.0f };

        // --------------------------------------------------
        // Conversion
        // --------------------------------------------------
        float radius_offset;
        const float vectorScalar = 0.005f; // 1.5f

        // --------------------------------------------------
        // Ranges
        // --------------------------------------------------
        int neck_sideVertexCount{ 0 };
        int head_sideVertexCount{ 0 };
        int head_baseVertexCount{ 0 };
        int neck_offset{ 0 };

        // --------------------------------------------------
        // Scaling
        // --------------------------------------------------
        // Neck
        float neck_baseHeight{ 1.0f };
        float neck_lengthScale{ 1.0f };
        float neck_radialScale{ 1.0f };
        // Head
        float head_lengthScale{ 1.0f };
        float head_radialScale{ 1.0f };

        void updateModelMatrices()
        {
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::quat q = glm::rotation(up, a_direction);

            glm::mat4 base = glm::translate(glm::mat4(1.0f), a_position) * glm::toMat4(q);

            neck_model = base * glm::scale(glm::mat4{ 1.0f }, glm::vec3{ neck_radialScale, neck_lengthScale, neck_radialScale });

            head_model = base * glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 0.0f, neck_baseHeight * neck_lengthScale, 0.0f }) * glm::scale(glm::mat4{ 1.0f }, glm::vec3{ head_radialScale, head_lengthScale, head_radialScale });
        }

        void generateMesh(const int sections) {

            m_vertices.clear();

            // =======================
            // Cone sides (TRIANGLES)
            // =======================
            for (int i = 0; i <= sections; i++)
            {
                float a0 = 2.0f * M_PI * i / sections;
                float a1 = 2.0f * M_PI * (i + 1) / sections;

                glm::vec3 p0(0.025 * cosf(a0), 0.0f, 0.025 * sinf(a0));
                glm::vec3 p1(0.025 * cosf(a1), 0.0f, 0.025 * sinf(a1));
                glm::vec3 apex(0.0f, 0.0667, 0.0f);

                // One triangle per section
                m_vertices.push_back(apex);
                m_vertices.push_back(p0);
                m_vertices.push_back(p1);
            }

            head_sideVertexCount = static_cast<int>(m_vertices.size());

            // =======================
            // Base disk (TRIANGLE_FAN)
            // =======================

            // center
            m_vertices.emplace_back(0.0f, 0.0f, 0.0f);

            for (int i = 0; i <= sections; i++)
            {
                float a = 2.0f * M_PI * i / sections;
                m_vertices.emplace_back(
                    0.025 * cosf(a),
                    0.0f,
                    0.025 * sinf(a)
                );
            }

            head_baseVertexCount =
                static_cast<int>(m_vertices.size()) - head_sideVertexCount;

            neck_offset = head_baseVertexCount + head_sideVertexCount;

            for (int i = 0; i <= sections; i++) {
                float a0 = 2.0f * M_PI * i / sections;
                float a1 = 2.0f * M_PI * (i + 1) / sections;

                glm::vec3 p0(0.0125 * cosf(a0), neck_baseHeight, 0.0125 * sinf(a0));
                glm::vec3 p1(0.0125 * cosf(a1), neck_baseHeight, 0.0125 * sinf(a1));
                glm::vec3 p2(0.0125 * cosf(a0), 0.0f, 0.0125 * sinf(a0));
                glm::vec3 p3(0.0125 * cosf(a1), 0.0f, 0.0125 * sinf(a1));

                // Vertices
                m_vertices.push_back(p0);
                m_vertices.push_back(p2);
                m_vertices.push_back(p1);

                m_vertices.push_back(p2);
                m_vertices.push_back(p1);
                m_vertices.push_back(p3);
            }

            neck_sideVertexCount = static_cast<int>(m_vertices.size()) - neck_offset;
        }

        void setupBuffers() {
            glGenVertexArrays(1, &m_VAO);
            glGenBuffers(1, &m_VBO);

            glBindVertexArray(m_VAO);
            glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

            glBufferData(
                GL_ARRAY_BUFFER,
                m_vertices.size() * sizeof(glm::vec3),
                m_vertices.data(),
                GL_STATIC_DRAW
            );

            glVertexAttribPointer(
                0, 3, GL_FLOAT, GL_FALSE,
                sizeof(glm::vec3),
                (void*)0
            );
            glEnableVertexAttribArray(0);

            glBindVertexArray(0);
        }
    };

    class sphere {
    public:
        sphere(
            int sectors,
            int stacks,
            glm::vec3 color,
            glm::vec3 pos
        )
            : colour(color)
        {
            generateMesh(sectors, stacks); // unit sphere
            setupBuffers();
            transform(pos, m_radius);
        }
        ~sphere()
        {
            glDeleteVertexArrays(1, &s_VAO);
            glDeleteBuffers(1, &s_VBO);
            glDeleteBuffers(1, &s_EBO);
        }

        void transform(const glm::vec3& pos, float radius) {
            m_position = pos;
            m_radius = radius;
            updateModelMatrix();
        }

        void draw(Shader* shader) {
            shader->use();
            shader->setVec3("colour", colour);
            shader->setMat4("model", s_model);

            glBindVertexArray(s_VAO);
            glDrawElements(GL_TRIANGLES,
                static_cast<GLsizei>(m_indices.size()),
                GL_UNSIGNED_INT,
                nullptr);
            glBindVertexArray(0);
        }

    private:
        GLuint s_VAO{}, s_VBO{}, s_EBO{};

        std::vector<glm::vec3> m_vertices;
        std::vector<glm::vec3> m_normals;
        std::vector<unsigned int> m_indices;

        glm::mat4 s_model{ 1.0f };
        glm::vec3 colour;

        glm::vec3 m_position{ 0.0f };
        float m_radius{ 1.0f };

        void updateModelMatrix() {
            s_model = glm::translate(glm::mat4(1.0f), m_position);
            s_model = glm::scale(s_model, glm::vec3(m_radius));
        }

        void generateMesh(int sectorCount, int stackCount) {
            m_vertices.clear();
            m_normals.clear();
            m_indices.clear();

            float sectorStep = 2.0f * M_PI / sectorCount;
            float stackStep = M_PI / stackCount;

            for (int i = 0; i <= stackCount; ++i) {
                float phi = M_PI / 2.0f - i * stackStep;
                float xy = cosf(phi);
                float z = sinf(phi);

                for (int j = 0; j <= sectorCount; ++j) {
                    float theta = j * sectorStep;

                    float x = xy * cosf(theta);
                    float y = xy * sinf(theta);

                    glm::vec3 pos{ x, y, z };
                    m_vertices.push_back(pos);
                    m_normals.push_back(glm::normalize(pos));
                }
            }

            for (int i = 0; i < stackCount; ++i) {
                int k1 = i * (sectorCount + 1);
                int k2 = k1 + sectorCount + 1;

                for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
                    if (i != 0) {
                        m_indices.push_back(k1);
                        m_indices.push_back(k2);
                        m_indices.push_back(k1 + 1);
                    }
                    if (i != (stackCount - 1)) {
                        m_indices.push_back(k1 + 1);
                        m_indices.push_back(k2);
                        m_indices.push_back(k2 + 1);
                    }
                }
            }
        }

        void setupBuffers() {
            glGenVertexArrays(1, &s_VAO);
            glGenBuffers(1, &s_VBO);
            glGenBuffers(1, &s_EBO);

            glBindVertexArray(s_VAO);

            glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
            glBufferData(GL_ARRAY_BUFFER,
                m_vertices.size() * sizeof(glm::vec3),
                m_vertices.data(),
                GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                m_indices.size() * sizeof(unsigned int),
                m_indices.data(),
                GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
            glEnableVertexAttribArray(0);

            glBindVertexArray(0);
        }
    };
}


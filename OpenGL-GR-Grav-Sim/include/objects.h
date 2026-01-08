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

    class Cube {
    public:
        Cube(
            float length,
            glm::vec3 colour,
            glm::vec3 position
        )
        {
            m_colour = glm::vec4(colour, 1.0f);
            setTransform(position, length);
            setupBuffers();
        }
        ~Cube()
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
            int sections
        )
            : colour(color),
            radius_offset(body_radius),
            a_position(position),
            a_direction(glm::normalize(magnitude)),
            neck_baseHeight((glm::length(magnitude) * velocityScalar) + radius_offset)
        {
            generateHeadMesh(sections);
            generateNeckMesh(sections);

            setupBuffers(head_VAO, head_VBO, head_vertices);
            setupBuffers(neck_VAO, neck_VBO, neck_vertices);

            updateModelMatrices();
        }

        void transform(const glm::vec3& position, const glm::vec3& magnitude) {
            a_position = position;
            a_direction = glm::normalize(magnitude);

            neck_lengthScale = ((glm::length(magnitude) * velocityScalar) + radius_offset) / neck_baseHeight;

            updateModelMatrices();
        }

        void resize(float head_newLength, float head_newRadius, float neck_newLength, float neck_newRadius) {
            head_lengthScale = head_newLength / head_baseHeight;
            neck_lengthScale = neck_newLength / neck_baseHeight;

            head_radialScale = head_newRadius / head_baseRadius;
            neck_radialScale = neck_newRadius / neck_baseRadius;
        }

        void draw(Shader* shader) {
            shader->use();

            shader->setVec3("colour", colour);

            // Neck
            shader->setMat4("model", neck_model);

            glBindVertexArray(neck_VAO);

            // Sides
            glDrawArrays(GL_TRIANGLES, 0, neck_vertices.size());

            // Head
            shader->setMat4("model", head_model);

            glBindVertexArray(head_VAO);

            // Cone sides
            glDrawArrays(GL_TRIANGLES, 0, head_sideVertexCount);

            // Base
            glDrawArrays(
                GL_TRIANGLE_FAN,
                head_baseOffset,
                head_baseVertexCount
            );
        }

    private:
        GLuint neck_VBO, neck_VAO;
        GLuint head_VBO, head_VAO;

        std::vector<glm::vec3> neck_vertices;
        std::vector<glm::vec3> head_vertices;

        glm::vec3 colour{ 1.0f };

        // --------------------------------------------------
        // Conversion
        // --------------------------------------------------
        float radius_offset;
        const float velocityScalar = 0.5f; // 1.5f

        // --------------------------------------------------
        // Ranges
        // --------------------------------------------------
        int neck_sideVertexCount{ 0 };
        int head_sideVertexCount{ 0 };
        int head_baseOffset{ 0 };
        int head_baseVertexCount{ 0 };

        // --------------------------------------------------
        // Scaling
        // --------------------------------------------------
        // Neck
        float neck_baseHeight{ 1.0f };
        float neck_lengthScale{ 1.0f };
        const float neck_baseRadius{ 0.125 };
        float neck_radialScale{ 1.0f };
        // Head
        const float head_baseHeight{ 0.667f };
        float head_lengthScale{ 1.0f };
        const float head_baseRadius{ 0.25f };
        float head_radialScale{ 1.0f };


        glm::vec3 a_position{ 0.0f };
        glm::vec3 a_direction{ 0.0f, 1.0f, 0.0f };

        glm::mat4 neck_model{ 1.0f };
        glm::mat4 head_model{ 1.0f };

        void updateModelMatrices()
        {
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::quat q = glm::rotation(up, a_direction);

            glm::mat4 base = glm::translate(glm::mat4(1.0f), a_position) * glm::toMat4(q);

            neck_model = base * glm::scale(glm::mat4{ 1.0f }, glm::vec3{ neck_radialScale, neck_lengthScale, neck_radialScale });

            head_model = base * glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 0.0f, neck_baseHeight * neck_lengthScale, 0.0f }) * glm::scale(glm::mat4{ 1.0f }, glm::vec3{ head_radialScale, head_lengthScale, head_radialScale });
        }

        void generateNeckMesh(int sections) {

            neck_vertices.clear();

            for (int i = 0; i <= sections; i++) {
                float a0 = 2.0f * M_PI * i / sections;
                float a1 = 2.0f * M_PI * (i + 1) / sections;

                glm::vec3 p0(neck_baseRadius * cos(a0), neck_baseHeight, neck_baseRadius * sin(a0));
                glm::vec3 p1(neck_baseRadius * cos(a1), neck_baseHeight, neck_baseRadius * sin(a1));
                glm::vec3 p2(neck_baseRadius * cos(a0), 0.0f, neck_baseRadius * sin(a0));
                glm::vec3 p3(neck_baseRadius * cos(a1), 0.0f, neck_baseRadius * sin(a1));

                // Vertices
                neck_vertices.push_back(p0);
                neck_vertices.push_back(p2);
                neck_vertices.push_back(p1);

                neck_vertices.push_back(p2);
                neck_vertices.push_back(p1);
                neck_vertices.push_back(p3);
            }

            neck_sideVertexCount = static_cast<int>(neck_vertices.size());
        }

        void generateHeadMesh(int sections) {
            head_vertices.clear();

            // =======================
            // Cone sides (TRIANGLES)
            // =======================
            for (int i = 0; i <= sections; i++)
            {
                float a0 = 2.0f * M_PI * i / sections;
                float a1 = 2.0f * M_PI * (i + 1) / sections;

                glm::vec3 p0(head_baseRadius * cos(a0), 0.0f, head_baseRadius * sin(a0));
                glm::vec3 p1(head_baseRadius * cos(a1), 0.0f, head_baseRadius * sin(a1));
                glm::vec3 apex(0.0f, head_baseHeight, 0.0f);

                // One triangle per section
                head_vertices.push_back(apex);
                head_vertices.push_back(p0);
                head_vertices.push_back(p1);
            }

            head_sideVertexCount = static_cast<int>(head_vertices.size());

            // =======================
            // Base disk (TRIANGLE_FAN)
            // =======================
            head_baseOffset = head_sideVertexCount;

            // center
            head_vertices.emplace_back(0.0f, 0.0f, 0.0f);

            for (int i = 0; i <= sections; i++)
            {
                float a = 2.0f * M_PI * i / sections;
                head_vertices.emplace_back(
                    head_baseRadius * cos(a),
                    0.0f,
                    head_baseRadius * sin(a)
                );
            }

            head_baseVertexCount =
                static_cast<int>(head_vertices.size()) - head_baseOffset;
        }

        void setupBuffers(GLuint &VAO, GLuint &VBO, std::vector<glm::vec3> &vertices) {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);

            glBufferData(
                GL_ARRAY_BUFFER,
                vertices.size() * sizeof(glm::vec3),
                vertices.data(),
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
}


#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


glm::vec3 PN_acceleration(glm::vec3 pos1, glm::vec3 pos2, glm::vec3 v1, glm::vec3 v2, float m1, float m2, bool years);
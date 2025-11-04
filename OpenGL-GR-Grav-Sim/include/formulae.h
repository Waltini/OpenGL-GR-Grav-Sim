#pragma once

#ifndef PN_ACCELERATION_H_INCLUDED
#define PN_ACCELERATION_H_INCLUDED

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "celestial_body_class.h"

using dvec3 = glm::dvec3;

dvec3 PN_acceleration(dvec3 pos1, dvec3 pos2, dvec3 v1, dvec3 v2, double m1, double m2);

void resolve_rel_accel(dvec3& a_rel, dvec3& a1, dvec3& a2, double m1, double m2);

#endif
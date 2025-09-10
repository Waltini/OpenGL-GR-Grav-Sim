#pragma once

#ifndef PN_ACCELERATION_H_INCLUDED
#define PN_ACCELERATION_H_INCLUDED

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "celestial_body_class.h"

using ldvec3 = glm::tvec3<long double>;

ldvec3 PN_accel_cons(ldvec3 pos1, ldvec3 pos2, ldvec3 v1, ldvec3 v2, long double m1, long double m2, bool years);

ldvec3 PN_accel_diss(ldvec3 pos1, ldvec3 pos2, ldvec3 v1, ldvec3 v2, long double m1, long double m2, bool years);

#endif
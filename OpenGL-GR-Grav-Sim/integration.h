#pragma once

#ifndef STEPRK45_H
#define STEPRK45_H_INCLUDED

#include <iostream>
#include <iomanip>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "celestial_body_class.h"

// Type Definitions for custom GLM maths objects
using ldvec3 = glm::tvec3<long double>;
using ldmat43 = glm::mat<4, 3, long double>;

// mathState structure for communication with the Buffer Box
struct mathState {
	ldmat43 y;
	long double m1, m2, physics_time;
};

struct result_values {
	ldmat43 state_y;
	long double time_update;
	bool accepted;
};

struct debug_values {
	result_values result;
	long double attempt_h;
	long double err_norm_passed;
	long double next_h;
};

class RK45_integration {
public:
	RK45_integration(long double atol, long double rtol, long double initial_dt);

	debug_values step(mathState backbuf);

	bool getDebug();

	void setDebug(bool update);
private:
	long double atol;
	long double rtol;
	long double h;
	bool debug = false;

	ldmat43 derivatives(long double t, ldmat43& y, long double m1, long double m2);

	debug_values RK45_step(ldmat43 y, long double t, long double& h, long double m1, long double m2);

	long double calc_err_norm(const ldmat43& y, const ldmat43& y4, const ldmat43& y5, const long double atol, const long double rtol);
};

#endif
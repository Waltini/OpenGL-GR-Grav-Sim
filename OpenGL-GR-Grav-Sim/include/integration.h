#pragma once

#ifndef STEPRK45_H
#define STEPRK45_H_INCLUDED

#include <iostream>
#include <iomanip>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "celestial_body_class.h"

using dvec3 = glm::dvec3;
using dmat43 = glm::mat<4, 3, double>;

struct mathState {
	dmat43 y;
	double m1, m2, physics_time;
};

struct substep_values {
	dmat43 state_y;
	double err_norm;
};

struct integrate_result {
	dmat43 state_y;
	double slip;
	int count;
	int accepts;
	int rejects;
	double avg_h;
	bool crash_f;
	char report[512];

	integrate_result(dmat43 state, double slip, int count, int accepts, int rejects, double avg_h, bool crash) :
		state_y(state), slip(slip), count(count), accepts(accepts), rejects(rejects), avg_h(avg_h), crash_f(crash) {
	}
};

class RK45_integration {
public:
	RK45_integration(double atol, double rtol, double initial_dt);

	integrate_result step(mathState backbuf, double physics_dt);

	bool getDebug();

	void setDebug(bool update);
private:
	double atol;
	double rtol;
	double timestep;
	bool debug = false;

	dmat43 derivatives(dmat43& y, double m1, double m2);

	integrate_result RK45_integrate(dmat43 y, double total_dt, double tol, double m1, double m2);

	substep_values RK45_substep(dmat43 y, double& h, double m1, double m2);

	double calc_err_norm(const dmat43& y, const dmat43& y4, const dmat43& y5, const double atol, const double rtol);
};

#endif
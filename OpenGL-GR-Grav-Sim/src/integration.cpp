#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "formulae.h"
#include "integration.h"

#include <iostream>
#include <iomanip>
#include <cmath>

using dvec3 = glm::tvec3<double>;
using dmat43 = glm::mat<4, 3, double>;

// Dormand–Prince coefficients
// -------------------------------------------------------------------------------------------
// State Step Constants
static const double a21_const = 0.2;
static const double a31_const = 3.0 / 40.0, a32_const = 9.0 / 40.0;
static const double a41_const = 44.0 / 45.0, a42_const = -56.0 / 15.0, a43_const = 32.0 / 9.0;
static const double a51_const = 19372.0 / 6561.0, a52_const = -25360.0 / 2187.0, a53_const = 64448.0 / 6561.0, a54_const = -212.0 / 729.0;
static const double a61_const = 9017.0 / 3168.0, a62_const = -355.0 / 33.0, a63_const = 46732.0 / 5247.0, a64_const = 49.0 / 176.0, a65_const = -5103.0 / 18656.0;

// Order Constants
static const double b1_const = 35.0 / 384.0, b3_const = 500.0 / 1113.0, b4_const = 125.0 / 192.0, b5_const = -2187.0 / 6784.0, b6_const = 11.0 / 84.0;
static const double b1s_const = 5179.0 / 57600.0, b3s_const = 7571.0 / 16695.0, b4s_const = 393.0 / 640.0, b5s_const = -92097.0 / 339200.0, b6s_const = 187.0 / 2100.0, b7s_const = 1.0 / 40.0;


//Constructor
RK45_integration::RK45_integration(double atol, double rtol, double initial_dt)
	: atol(atol), rtol(rtol), timestep(initial_dt) {}

integrate_result RK45_integration::step(mathState backbuf, double physics_dt) {
	const double tol = 1.0;
	
	integrate_result result = RK45_integrate(backbuf.y, physics_dt, tol, backbuf.m1, backbuf.m2);

	backbuf.physics_time += physics_dt;

	return result;
}

bool RK45_integration::getDebug() { return RK45_integration::debug; } // Used for debugging

void RK45_integration::setDebug(bool update) { RK45_integration::debug = update; } // Used to turn into debugging mode (Currently unsetup)

dmat43 RK45_integration::derivatives(dmat43& state, double m1, double m2) {
	// Propertries Unpacking
	dvec3 pos1 = state[0]; 
	dvec3 v1 = state[1];
	dvec3 pos2 = state[2];
	dvec3 v2 = state[3];

	// Relative acceleration of the current state
	dvec3 a_rel = PN_acceleration(pos1, pos2, v1, v2, m1, m2);
	dvec3 a1, a2;
	resolve_rel_accel(a_rel, a1, a2, m1, m2); // Seperates the individual accelerations of each body given the mass ratio

	dmat43 dydt; // Packs a new derivative state
	dydt[0] = v1;
	dydt[1] = a1;
	dydt[2] = v2;
	dydt[3] = a2;

	return dydt;
}

// sqrt(1/N * sum((y5_ij - y4_ij)/(atol + rtol * max(y_ij, y4_ij))))
double RK45_integration::calc_err_norm(const dmat43& y, const dmat43& y4, const dmat43& y5, const double atol, const double rtol) {
	// Formulae Variables
	double sum = 0.0; // sum(diff^2)
	int count = 0; // 1 / N

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 3; j++) {
			double scale = atol + rtol * std::max(std::abs(y[i][j]), std::abs(y4[i][j])); // atol + rtol * max(y_ij, y4_ij)
			double diff = (y5[i][j] - y4[i][j]) / scale; // (y5_ij - y4_ij) / scale
			sum += diff * diff;
			count++;
		}
	}

	return std::sqrt(sum / count);
}

substep_values RK45_integration::RK45_substep(dmat43 y, double& h, double m1, double m2) {
	// RK45 Stages
	// -------------------------------------------------------------------------------------
	dmat43 k1, k2, k3, k4, k5, k6, k7, staged_y;
	// Stage 1
	k1 = derivatives(y, m1, m2);

	// Stage 2
	staged_y = y + h * (a21_const * k1);
	k2 = derivatives(staged_y, m1, m2);

	// Stage 3
	staged_y = y + h * ((a31_const * k1) + (a32_const * k2));
	k3 = derivatives(staged_y, m1, m2);

	// Stage 4
	staged_y = y + h * ((a41_const * k1) + (a42_const * k2) + (a43_const * k3));
	k4 = derivatives(staged_y, m1, m2);

	// Stage 5
	staged_y = y + h * ((a51_const * k1) + (a52_const * k2) + (a53_const * k3) + (a54_const * k4));
	k5 = derivatives(staged_y, m1, m2);

	// Stage 6
	staged_y = y + h * ((a61_const * k1) + (a62_const * k2) + (a63_const * k3) + (a64_const * k4) + (a65_const * k5));
	k6 = derivatives(staged_y, m1, m2);

	// Fourth order solution
	dmat43 y4 = y + h * (b1_const * k1 + b3_const * k3 + b4_const * k4 + b5_const * k5 + b6_const * k6);

	// Stage 7
	k7 = derivatives(y4, m1, m2); // y4 is the coincidental staged_y for stage 7

	// Fifth order solution
	dmat43 y5 = y + h * (b1s_const * k1 + b3s_const * k3 + b4s_const * k4 + b5s_const * k5 + b6s_const * k6 + b7s_const * k7);

	// Error checking
	double err_norm = calc_err_norm(y, y4, y5, RK45_integration::atol, RK45_integration::rtol);

	substep_values result{
		y5, err_norm
	};

	return result;
}

integrate_result RK45_integration::RK45_integrate(dmat43 y, double total_dt, double tol, double m1, double m2) {
	const double safety = 0.9;
	const double minAdapt = 0.1, maxAdapt = 5.0;

	double intg_t = 0.0;
	double overhang = 0.0;

	dmat43 state = y;

	int accepts = 0, rejects = 0, count = 0;
	double tot_h = 0.0;

	int since_last_accept = 0;

	double h = RK45_integration::timestep;

	bool no_crash = true;

	while (intg_t < total_dt && no_crash) {
		if (intg_t + h > total_dt) {
			h = total_dt - intg_t;
		}

		substep_values RK45_values = RK45_substep(state, h, m1, m2);

		if (RK45_values.err_norm < tol) {
			intg_t += h;
			state = RK45_values.state_y;
			accepts++;
			since_last_accept = 0;
		}
		else {
			rejects++;
			since_last_accept++;
			if (since_last_accept >= 50) {no_crash = false;}
		}

		double adapt = safety * std::pow(1.0 / (RK45_values.err_norm + 1e-16), 0.2);
		adapt = std::min(std::max(adapt, minAdapt), maxAdapt);

		h *= adapt;

		count++;
	}

	RK45_integration::timestep = h;

	integrate_result result{
		state, 0.0, count, accepts, rejects, (tot_h / count), !no_crash
	};

	return result;
}
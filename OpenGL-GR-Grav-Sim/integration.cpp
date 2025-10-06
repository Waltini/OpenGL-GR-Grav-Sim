#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "formulae.h"
#include "celestial_body_class.h"
#include "integration.h"

#include <iostream>
#include <iomanip>
#include <cmath>

// Dormand–Prince coefficients
// -------------------------------------------------------------------------------------------
// t Step Constants
static const long double c2_const = 0.2L;
static const long double c3_const = 0.3L;
static const long double c4_const = 0.8L;
static const long double c5_const = 8.0L / 9.0L;

// State Step Constants
static const long double a21_const = 0.2L;
static const long double a31_const = 3.0L / 40.0L, a32_const = 9.0L / 40.0L;
static const long double a41_const = 44.0L / 45.0L, a42_const = -56.0L / 15.0L, a43_const = 32.0L / 9.0L;
static const long double a51_const = 19372.0L / 6561.0L, a52_const = -25360.0L / 2187.0L, a53_const = 64448.0L / 6561.0L, a54_const = -212.0L / 729.0L;
static const long double a61_const = 9017.0L / 3168.0L, a62_const = -355.0L / 33.0L, a63_const = 46732.0L / 5247.0L, a64_const = 49.0L / 176.0L, a65_const = -5103.0L / 18656.0L;

// Order Constants
static const long double b1_const = 35.0L / 384.0L, b3_const = 500.0L / 1113.0L, b4_const = 125.0L / 192.0L, b5_const = -2187.0L / 6784.0L, b6_const = 11.0L / 84.0L;
static const long double b1s_const = 5179.0L / 57600.0L, b3s_const = 7571.0L / 16695.0L, b4s_const = 393.0L / 640.0L, b5s_const = -92097.0L / 339200.0L, b6s_const = 187.0L / 2100.0L, b7s_const = 1.0L / 40.0L;


	//Constructor
RK45_integration::RK45_integration(long double atol, long double rtol, long double initial_dt) 
	: atol(atol), rtol(rtol), h(initial_dt) {}

debug_values RK45_integration::step(mathState backbuf) {
	debug_values result = RK45_step(backbuf.y, backbuf.physics_time, RK45_integration::h, backbuf.m1, backbuf.m2);

	return result;
}

bool RK45_integration::getDebug() { return RK45_integration::debug; }

void RK45_integration::setDebug(bool update) { RK45_integration::debug = update; }


ldmat43 RK45_integration::derivatives(long double t, ldmat43& state, long double m1, long double m2) {
	ldvec3 pos1 = state[0];
	ldvec3 v1 = state[1];
	ldvec3 pos2 = state[2];
	ldvec3 v2 = state[3];

	ldvec3 a_rel = PN_acceleration(pos1, pos2, v1, v2, m1, m2);
	ldvec3 a1, a2;
	resolve_rel_accel(a_rel, a1, a2, m1, m2);

	ldmat43 dydt;
	dydt[0] = v1;
	dydt[1] = a1;
	dydt[2] = v2;
	dydt[3] = a2;

	return dydt;
}

// sqrt(1/N * ? (y5_ij - y4_ij)/(atol + rtol * max(y_ij, y4_ij)))
long double RK45_integration::calc_err_norm(const ldmat43& y, const ldmat43& y4, const ldmat43& y5, const long double atol, const long double rtol) {
	// Formulae Variables
	long double sum = 0.0L; // ? diff^2
	int count = 0; // 1 / N

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 3; j++) {
			long double scale = atol + rtol * std::max(std::abs(y[i][j]), std::abs(y4[i][j])); // atol + rtol * max(y_ij, y4_ij)
			long double diff = (y5[i][j] - y4[i][j]) / scale; // (y5_ij - y4_ij) / scale
			sum += diff * diff;
			count++;
		}
	}

	return std::sqrt(sum / count);
}

debug_values RK45_integration::RK45_step(ldmat43 y, long double t, long double& h, long double m1, long double m2) {
	const long double safety = 0.9L;
	const long double minAdapt = 0.1L, maxAdapt = 5.0L;

	// RK45 Stages
	// -------------------------------------------------------------------------------------
	ldmat43 k1, k2, k3, k4, k5, k6, k7, staged_y;
	// Stage 1
	k1 = derivatives(t, y, m1, m2);

	// Stage 2
	staged_y = y + h * (a21_const * k1);
	k2 = derivatives(t + c2_const * h, staged_y, m1, m2);

	// Stage 3
	staged_y = y + h * ((a31_const * k1) + (a32_const * k2));
	k3 = derivatives(t + c3_const * h, staged_y, m1, m2);

	// Stage 4
	staged_y = y + h * ((a41_const * k1) + (a42_const * k2) + (a43_const * k3));
	k4 = derivatives(t + c4_const * h, staged_y, m1, m2);

	// Stage 5
	staged_y = y + h * ((a51_const * k1) + (a52_const * k2) + (a53_const * k3) + (a54_const * k4));
	k5 = derivatives(t + c5_const * h, staged_y, m1, m2);

	// Stage 6
	staged_y = y + h * ((a61_const * k1) + (a62_const * k2) + (a63_const * k3) + (a64_const * k4) + (a65_const * k5));
	k6 = derivatives(t + h, staged_y, m1, m2);

	// Fourth order solution
	ldmat43 y4 = y + h * (b1_const * k1 + b3_const * k3 + b4_const * k4 + b5_const * k5 + b6_const * k6);

	// Stage 7
	k7 = derivatives(t + h, y4, m1, m2); // y4 is the coincidental staged_y for stage 7

	// Fifth order solution
	ldmat43 y5 = y + h * (b1s_const * k1 + b3s_const * k3 + b4s_const * k4 + b5s_const * k5 + b6s_const * k6 + b7s_const * k7);

	// Error checking
	long double err_norm = calc_err_norm(y, y4, y5, atol, rtol);

	if (err_norm <= 1.0L) {
		y = y5;
		t += h;

		long double adapt = safety * std::pow(1.0L / (err_norm + 1e-16L), 0.2);
		adapt = std::min(std::max(adapt, minAdapt), maxAdapt);

		result_values result = {
			y, t, true
		};

		debug_values result_debug = {
			result, h, err_norm, h * adapt
		};

		h *= adapt;

		return result_debug;

	}
	else {
		long double adapt = safety * std::pow(1.0L / (err_norm + 1e-16L), 0.2);
		adapt = std::min(std::max(adapt, minAdapt), maxAdapt);

		result_values result = {
			y, t, false
		};

		debug_values result_debug = {
			result, h, err_norm, h * adapt
		};

		h *= adapt;

		return result_debug;
	}
}
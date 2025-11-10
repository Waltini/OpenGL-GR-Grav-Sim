#define M_PI        3.14159265358979323846264338327950288   /* pi */
#define GLM_ENABLE_EXPERIMENTAL

#include <iostream>
#include <iomanip>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "formulae.h"

using dvec3 = glm::dvec3;

// Mathematical Constants
// -----------------------------------------------------------------------------------------
constexpr double G = 4.0 * M_PI * M_PI; // Newtonian Gravitational Constant in AU^3 / (Msun * yr^2)
constexpr double c = 63241.0771; // Speed of light in AU / yr

// Function
// -----------------------------------------------------------------------------------------
dvec3 PN_acceleration(dvec3 pos1, dvec3 pos2, dvec3 v1, dvec3 v2, double m1, double m2) {
	// Terms
	// -------------------------------------------------------------------------------------
	// Mass Terms
	const double m = m1 + m2; // total mass MAKE ZERO MASS INVALID OR IMPOSSIBLE
	const double mu = G * m; // standard gravitational parameter (AU^3 / {day^2 : yr^2})
	const double n_smr = (m1 * m2) / (m * m); // symmetric mass ratio
	// Relative Terms
	dvec3 v_bold = v1 - v2; // vecotr velocity difference 
	dvec3 sep = pos1 - pos2; // vector seperation 
	const double r = glm::length(sep); // scalar seperation 
	const double v_2 = glm::dot(v_bold, v_bold); // scalar velocity difference 
	const double inv_r = 1.0 / r;

	dvec3 n_hat = sep * inv_r; // unit vector
	const double r_dot = glm::dot(v_bold, n_hat); // radial velocity

	const double r_dot2 = r_dot * r_dot;
	const double mu_r = mu * inv_r;
	
	
	//std::cout << std::setprecision(20) << "m = " << m << std::endl;
	//std::cout << std::setprecision(20) << "n_smr = " << n_smr << std::endl;
	//std::cout << std::setprecision(20) << "r = " << r << std::endl;
	//std::cout << std::setprecision(20) << "v = " << v << std::endl;
	//std::cout << std::setprecision(20) << "r_dot = " << r_dot << std::endl;

	//std::cout << std::setprecision(20) << "v_bold = " << v_bold.x << " " << v_bold.y << " " << v_bold.z << std::endl;
	//std::cout << std::setprecision(20) << "sep = " << sep.x << " " << sep.y << " " << sep.z << std::endl;
	//std::cout << std::setprecision(20) << "n_hat = " << n_hat.x << " " << n_hat.y << " " << n_hat.z << std::endl;

	// Formulaes
	// -------------------------------------------------------------------------------------
	// More understandable formulae will be commented above the overly bracketed c++ formulaes
	// -------------------------------------------------------------------------------------

	// 1PN Terms
	// ((4 + 2 * n_smr)Gm/r - (1 + 3 * n_smr) * v^2 + 3/2 * n_smr * r_dot^2) * n_hat + (4 - 2 * n_smr) * r_dot * v_bold
	dvec3 A_1PN = (( ((4.0 + (2.0 * n_smr)) * mu_r) 
			- ((1.0 + (3.0 * n_smr)) * v_2)
			+ (1.5 * (n_smr * r_dot2))) * n_hat) 
			+ (((4.0 - (2.0 * n_smr)) * r_dot) * v_bold);
	//std::cout << std::setprecision(20) << "A_1PN = " << A_1PN.x << " " << A_1PN.y << " " << A_1PN.z << std::endl;

	// 2PN Terms
	const double v_4 = v_2 * v_2;
	const double r_dot4 = r_dot2 * r_dot2;
	const double mu_r2 = mu_r * mu_r;

	//std::cout << std::setprecision(20) << "A_2PN_n_1 = " << A_2PN_n_1 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_n_2 = " << A_2PN_n_2 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_n_3 = " << A_2PN_n_3 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_n_4 = " << A_2PN_n_4 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_n_5 = " << A_2PN_n_5 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_n_6 = " << A_2PN_n_6 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_n = " << A_2PN_n.x << " " << A_2PN_n.y << " " << A_2PN_n.z << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_v_1 = " << A_2PN_v_1 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_v_2 = " << A_2PN_v_2 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_v_3 = " << A_2PN_v_3 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_v = " << A_2PN_v.x << " " << A_2PN_v.y << " " << A_2PN_v.z << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN = " << A_2PN.x << " " << A_2PN.y << " " << A_2PN.z << std::endl;

	dvec3 A_2PN = ((0.75 * (12.0 + 29.0 * n_smr) * (mu_r2))
		+ (n_smr * (3.0 - 4.0 * n_smr) * v_4)
		+ ((15.0 / 8.0) * n_smr * (1.0 - 3.0 * n_smr) * r_dot4)
		- (1.5 * n_smr * (3.0 - 4.0 * n_smr) * v_2 * r_dot2)
		- (0.5 * n_smr * (13.0 - 4.0 * n_smr) * mu_r * v_2)
		- (2.0 + (25.0 * n_smr) + 2.0 * (n_smr * n_smr)) * mu_r * r_dot2) * n_hat 
		+ ((n_smr * (15.0 + 4.0 * n_smr) * v_2 * r_dot)
		- (1.5 * n_smr * (3.0 + 2.0 * n_smr) * r_dot2 * r_dot)
		- (0.5 * (4.0 + (41.0 * n_smr) + 8.0 * (n_smr * n_smr)) * mu_r * r_dot)) * v_bold;


	
	// 2.5PN
	// Responsible for the orbital decay caused by graviational radiation
	// -8/15 * n_smr * Gm/r * ((9v^2 + 17Gm/r) * r_dot * n_hat - (3v^2 + 9Gm/r) * v_bold)
	dvec3 A_25PN = ((((9.0 * v_2) + (17.0 * mu_r)) * r_dot * n_hat)
		+ (((3.0 * v_2) + (9.0 * mu_r)) * v_bold)) * (-(8.0 / 15.0) * n_smr * mu_r);

	//std::cout << std::setprecision(20) << "A_2PN_n_1 = " << A_2PN_n_1 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_n_2 = " << A_2PN_n_2 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_n_3 = " << A_2PN_n_3 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_n_4 = " << A_2PN_n_4 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_n_5 = " << A_2PN_n_5 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_n_6 = " << A_2PN_n_6 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_n = " << A_2PN_n.x << " " << A_2PN_n.y << " " << A_2PN_n.z << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_v_1 = " << A_2PN_v_1 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_v_2 = " << A_2PN_v_2 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_v_3 = " << A_2PN_v_3 << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN_v = " << A_2PN_v.x << " " << A_2PN_v.y << " " << A_2PN_v.z << std::endl;
	//std::cout << std::setprecision(20) << "A_2PN = " << A_2PN.x << " " << A_2PN.y << " " << A_2PN.z << std::endl;


	const double c2 = (1 / (c * c));
	const double c4 = (1 / (c * c * c * c));
	const double c5 = (1 / (c * c * c * c * c));

	// dv_bold/dt ~ a
	// Gm/r^2(-n_hat + (1/c^2)(A_1PN) + (1/c^4)(A_2PN))
	dvec3 a = (mu / (r * r)) * (-n_hat + (c2 * A_1PN) + (c4 * A_2PN) + (c5 * A_25PN));
	//std::cout << std::setprecision(20) << "a = " << a_cons.x << " " << a_cons.y << " " << a_cons.z << std::endl;

	return a;
}

void resolve_rel_accel(dvec3& a_rel, dvec3& a1, dvec3& a2, double m1, double m2) {
	// Because I've only included up to the 2.5PN term currently the individual accelerations can be seperated from the relative acceleration using the mass ratio of the two objects
	double m = m1 + m2;
	a1 = (m2 / m) * a_rel;
	a2 = -((m1 / m) * a_rel);
	//std::cout << std::setprecision(20) << "a1 = " << a1.x << " " << a1.y << " " << a1.z << std::endl;
	//std::cout << std::setprecision(20) << "a2 = " << a2.x << " " << a2.y << " " << a2.z << std::endl;
	//std::cout << std::setprecision(20) << "a_rel = " << a_rel.x << " " << a_rel.y << " " << a_rel.z << std::endl;
}

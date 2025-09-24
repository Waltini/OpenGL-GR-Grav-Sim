#define M_PI        3.14159265358979323846264338327950288   /* pi */
#define GLM_ENABLE_EXPERIMENTAL

#include <iostream>
#include <iomanip>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "celestial_body_class.h"
#include "formulae.h"

// Mathematical Constants
// -----------------------------------------------------------------------------------------
constexpr double G_d = 2.9592338593516714e-4; // Newtonian Gravitational Constant in AU^3 / (Msun * day^2)
constexpr double c_d = 173.2632249; // Speed of light in AU / day

constexpr double G = 4.0 * M_PI * M_PI; // Newtonian Gravitational Constant in AU^3 / (Msun * yr^2)
constexpr double c = 63241.0771; // Speed of light in AU / yr

using ldvec3 = glm::tvec3<long double>;

// Function
// -----------------------------------------------------------------------------------------
ldvec3 PN_acceleration(ldvec3 pos1, ldvec3 pos2, ldvec3 v1, ldvec3 v2, long double m1, long double m2) {
	// Terms
	// -------------------------------------------------------------------------------------
	// Mass Terms
	const long double m = m1 + m2; // total mass MAKE ZERO MASS INVALID OR IMPOSSIBLE
	const long double mu = G * m; // standard gravitational parameter (AU^3 / {day^2 : yr^2})
	const long double n_smr = (m1 * m2) / (m * m); // symmetric mass ratio
	// Relative Terms
	ldvec3 v_bold = v1 - v2; // vecotr velocity difference 
	ldvec3 sep = pos1 - pos2; // vector seperation 
	const long double r = glm::length(sep); // scalar seperation 
	const long double v_2 = glm::dot(v_bold, v_bold); // scalar velocity difference 
	const long double inv_r = 1.0L / r;

	ldvec3 n_hat = sep * inv_r; // unit vector
	const long double r_dot = glm::dot(v_bold, n_hat); // radial velocity

	const long double r_dot2 = r_dot * r_dot;
	const long double mu_r = mu * inv_r;
	
	
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
	ldvec3 A_1PN = (( ((4.0L + (2.0L * n_smr)) * mu_r) - ((1.0L + (3.0L * n_smr)) * v_2) + (1.5L * (n_smr * r_dot2))) * n_hat) + (((4.0L - (2.0L * n_smr)) * r_dot) * v_bold);
	//std::cout << std::setprecision(20) << "A_1PN = " << A_1PN.x << " " << A_1PN.y << " " << A_1PN.z << std::endl;

	// 2PN Terms
	const long double v_4 = v_2 * v_2;
	const long double r_dot4 = r_dot2 * r_dot2;
	const long double mu_r2 = mu_r * mu_r;

	ldvec3 A_2PN_n = ((0.75L * (12.0L + 29.0L * n_smr) * (mu_r2))
			+ (n_smr * (3.0L - 4.0L * n_smr) * v_4)
			+ ((15.0L / 8.0L) * n_smr * (1.0L - 3.0L * n_smr) * r_dot4)
			- (1.5L * n_smr * (3.0L - 4.0L * n_smr) * v_2 * r_dot2)
			- (0.5L * n_smr * (13.0L - 4.0L * n_smr) * mu_r * v_2)
			- (2.0L + (25.0L * n_smr) + 2.0L * (n_smr * n_smr)) * mu_r * r_dot2) * n_hat;

	ldvec3 A_2PN_v = ((n_smr * (15.0L + 4.0L * n_smr) * v_2 * r_dot)
		- (1.5L * n_smr * (3.0L + 2.0L * n_smr) * r_dot2 * r_dot)
		- (0.5L * (4.0L + (41.0L * n_smr) + 8.0L * (n_smr * n_smr)) * mu_r * r_dot)) * v_bold;

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

	ldvec3 A_2PN = A_2PN_n + A_2PN_v;
	
	// 2.5PN
	// Responsible for the orbital decay caused by graviational radiation
	// -8/15 * n_smr * Gm/r * ((9v^2 + 17Gm/r) * r_dot * n_hat - (3v^2 + 9Gm/r) * v_bold)
	ldvec3 A_25PN = ((((9.0L * v_2) + (17.0L * mu_r)) * r_dot * n_hat)
		+ (((3.0L * v_2) + (9.0L * mu_r)) * v_bold)) * (-(8.0L / 15.0L) * n_smr * mu_r);

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


	const long double c2 = (1 / (c * c));
	const long double c4 = (1 / (c * c * c * c));
	const long double c5 = (1 / (c * c * c * c * c));

	// dv_bold/dt ~ a
	// Gm/r^2(-n_hat + (1/c^2)(A_1PN) + (1/c^4)(A_2PN))
	ldvec3 a = (mu / (r * r)) * (-n_hat + (c2 * A_1PN) + (c4 * A_2PN) + (c5 * A_25PN));
	//std::cout << std::setprecision(20) << "a = " << a_cons.x << " " << a_cons.y << " " << a_cons.z << std::endl;

	return a;
}

void resolve_rel_accel(ldvec3& a_rel, ldvec3& a1, ldvec3& a2, long double m1, long double m2) {
	long double m = m1 + m2;
	a1 = (m2 / m) * a_rel;
	a2 = -((m1 / m) * a_rel);
	//std::cout << std::setprecision(20) << "a1 = " << a1.x << " " << a1.y << " " << a1.z << std::endl;
	//std::cout << std::setprecision(20) << "a2 = " << a2.x << " " << a2.y << " " << a2.z << std::endl;
	//std::cout << std::setprecision(20) << "a_rel = " << a_rel.x << " " << a_rel.y << " " << a_rel.z << std::endl;
}

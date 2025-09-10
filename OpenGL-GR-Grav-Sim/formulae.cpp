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

constexpr double G_yr = 4.0 * M_PI * M_PI; // Newtonian Gravitational Constant in AU^3 / (Msun * yr^2)
constexpr double c_yr = 63241.0771; // Speed of light in AU / yr

constexpr long double G = 6.6743015e-11L;
constexpr unsigned int c = 299792458;

using ldvec3 = glm::tvec3<long double>;

// Function
// -----------------------------------------------------------------------------------------
ldvec3 PN_accel_cons(ldvec3 pos1, ldvec3 pos2, ldvec3 v1, ldvec3 v2, long double m1, long double m2, bool years) {
	// Time
	//  ------------------------------------------------------------------------------------
	//const double G = years ? G_yr : G_d;
	//const double c = years ? c_yr : c_d;
	//std::cout << "G: " << G << std::endl;
	//std::cout << "c: " << c << std::endl;
	// Terms
	// -------------------------------------------------------------------------------------
	const long double m = m1 + m2; // total mass MAKE ZERO MASS INVALID OR IMPOSSIBLE
	const long double mu = G * m; // standard gravitational parameter (AU^3 / {day^2 : yr^2})
	const long double n_smr = (m1 * m2) / (m * m); // symmetric mass ratio
	ldvec3 v_bold = v1 - v2; // vecotr velocity difference 
	ldvec3 sep = pos1 - pos2; // vector seperation 

	const long double r = glm::length(sep); // scalar seperation 
	const long double v = glm::length(v_bold); // scalar velocity difference 

	ldvec3 n_hat = sep / r; // unit vector
	const long double r_dot = (v_bold.x * n_hat.x) + (v_bold.y * n_hat.y) + (v_bold.z * n_hat.z); // radial velocity
	
	
	std::cout << std::setprecision(20) << "m = " << m << std::endl;
	std::cout << std::setprecision(20) << "n_smr = " << n_smr << std::endl;
	std::cout << std::setprecision(20) << "r = " << r << std::endl;
	std::cout << std::setprecision(20) << "v = " << v << std::endl;
	std::cout << std::setprecision(20) << "r_dot = " << r_dot << std::endl;

	std::cout << std::setprecision(20) << "v_bold = " << v_bold.x << " " << v_bold.y << " " << v_bold.z << std::endl;
	std::cout << std::setprecision(20) << "sep = " << sep.x << " " << sep.y << " " << sep.z << std::endl;
	std::cout << std::setprecision(20) << "n_hat = " << n_hat.x << " " << n_hat.y << " " << n_hat.z << std::endl;

	// Formulaes
	// -------------------------------------------------------------------------------------
	// More understandable formulae will be commented above the overly bracketed c++ formulaes
	// -------------------------------------------------------------------------------------

	// 1PN
	// ((4 + 2 * n_smr)Gm/r - (1 + 3 * n_smr) * v^2 + 3/2 * n_smr * r_dot^2) * n_hat + (4 - 2 * n_smr) * r_dot * v_bold
	ldvec3 A_1PN = (( (4.0L + (2.0L * n_smr)) * (mu / r) - ((1.0L + (3.0L * n_smr)) * ((v_bold.x * v_bold.x) + (v_bold.y * v_bold.y) + (v_bold.z * v_bold.z))) + (1.5L * (n_smr * r_dot * r_dot))) * n_hat) + (((4.0L - (2.0L * n_smr)) * r_dot) * v_bold);
	std::cout << std::setprecision(20) << "A_1PN = " << A_1PN.x << " " << A_1PN.y << " " << A_1PN.z << std::endl;

	// 2PN
	long double A_2PN_n_1 = (0.75L * (12.0L + 29.0L * n_smr) * ((mu / r) * (mu / r))); // 3/4(12 + 29 * n_smr)(Gm/r)^2
	long double A_2PN_n_2 = (n_smr * (3.0L - 4.0L * n_smr) * (((v_bold.x * v_bold.x) + (v_bold.y * v_bold.y) + (v_bold.z * v_bold.z)) * ((v_bold.x * v_bold.x) + (v_bold.y * v_bold.y) + (v_bold.z * v_bold.z)))); // n_smr(3 - 4 * n_smr) * v^4
	long double A_2PN_n_3 = ((15.0L / 8.0L) * n_smr * (1.0L - 3.0L * n_smr) * (r_dot * r_dot * r_dot * r_dot)); // 15/8 * n_smr(1 - 3 * n_smr) * r_dot^4
	long double A_2PN_n_4 = -(1.5L * n_smr * (3.0L - 4.0L * n_smr) * ((v_bold.x * v_bold.x) + (v_bold.y * v_bold.y) + (v_bold.z * v_bold.z)) * (r_dot * r_dot)); // -(3/2 * n_smr(3 - 4 * n_smr) * v^2 * r_dot^2)
	long double A_2PN_n_5 = -(0.5L * n_smr * (13.0L - 4.0L * n_smr) * (mu / r) * ((v_bold.x * v_bold.x) + (v_bold.y * v_bold.y) + (v_bold.z * v_bold.z))); // -(1/2 * n_smr(13 - 4 * n_smr)Gm/r * v^2
	long double A_2PN_n_6 = -(2.0L + (25.0L * n_smr) + 2.0L * (n_smr * n_smr)) * (mu / r) * (r_dot * r_dot); // -(2 + 25 * n_smr + 2 * (n_smr)^2)Gm/r * r_dot^2)
	ldvec3 A_2PN_n = (A_2PN_n_1 + A_2PN_n_2 + A_2PN_n_3 + A_2PN_n_4 + A_2PN_n_5 + A_2PN_n_6) * n_hat;

	long double A_2PN_v_1 = (n_smr * (15.0L + 4.0L * n_smr) * ((v_bold.x * v_bold.x) + (v_bold.y * v_bold.y) + (v_bold.z * v_bold.z)) * r_dot); // n_smr(15 + 4 * n_smr) * v^2 * r_dot
	long double A_2PN_v_2 = -(1.5L * n_smr * (3.0L + 2.0L * n_smr) * (r_dot * r_dot * r_dot)); // -(3/2 * n_smr(3 + 2n_smr) * r_dot^3)
	long double A_2PN_v_3 = -(0.5L * (4.0L + (41.0L * n_smr) + 8.0L * (n_smr * n_smr)) * (mu / r) * r_dot); // -(1/2 * (4 + 41 * n_smr + 8 * n_smr^2)Gm/r * r_dot)
	ldvec3 A_2PN_v = (A_2PN_v_1 + A_2PN_v_2 + A_2PN_v_3) * v_bold;

	ldvec3 A_2PN = A_2PN_n + A_2PN_v;
	
	std::cout << std::setprecision(20) << "A_2PN_n_1 = " << A_2PN_n_1 << std::endl;
	std::cout << std::setprecision(20) << "A_2PN_n_2 = " << A_2PN_n_2 << std::endl;
	std::cout << std::setprecision(20) << "A_2PN_n_3 = " << A_2PN_n_3 << std::endl;
	std::cout << std::setprecision(20) << "A_2PN_n_4 = " << A_2PN_n_4 << std::endl;
	std::cout << std::setprecision(20) << "A_2PN_n_5 = " << A_2PN_n_5 << std::endl;
	std::cout << std::setprecision(20) << "A_2PN_n_6 = " << A_2PN_n_6 << std::endl;
	std::cout << std::setprecision(20) << "A_2PN_n = " << A_2PN_n.x << " " << A_2PN_n.y << " " << A_2PN_n.z << std::endl;
	std::cout << std::setprecision(20) << "A_2PN_v_1 = " << A_2PN_v_1 << std::endl;
	std::cout << std::setprecision(20) << "A_2PN_v_2 = " << A_2PN_v_2 << std::endl;
	std::cout << std::setprecision(20) << "A_2PN_v_3 = " << A_2PN_v_3 << std::endl;
	std::cout << std::setprecision(20) << "A_2PN_v = " << A_2PN_v.x << " " << A_2PN_v.y << " " << A_2PN_v.z << std::endl;
	std::cout << std::setprecision(20) << "A_2PN = " << A_2PN.x << " " << A_2PN.y << " " << A_2PN.z << std::endl;


	const long double c2 = (1 / (c * c));
	const long double c4 = (1 / (c * c * c * c));

	// dv_bold/dt ~ a
	// Gm/r^2(-n_hat + (1/c^2)(A_1PN) + (1/c^4)(A_2PN))
	ldvec3 a_cons = (mu / (r * r)) * (-n_hat + (c2 * A_1PN) + (c4 * A_2PN));
	std::cout << std::setprecision(20) << "a = " << a_cons.x << " " << a_cons.y << " " << a_cons.z << std::endl;

	return a_cons;
}

ldvec3 PN_accel_diss(ldvec3 pos1, ldvec3 pos2, ldvec3 v1, ldvec3 v2, long double m1, long double m2, bool years) {
	// Terms
	// ------------------------------------------------------------------------------------
	const long double c5 = (1 / (c * c * c * c * c));
	const long double m = m1 + m2; // total mass MAKE ZERO MASS INVALID OR IMPOSSIBLE
	const long double mu = G * m; // standard gravitational parameter (AU^3 / {day^2 : yr^2})
	const long double n_smr = (m1 * m2) / (m * m); // symmetric mass ratio
	ldvec3 v_bold = v1 - v2; // vecotr velocity difference 
	ldvec3 sep = pos1 - pos2; // vector seperation 

	const long double r = glm::length(sep); // scalar seperation 
	const long double v = glm::length(v_bold); // scalar velocity difference 

	ldvec3 n_hat = sep / r; // unit vector
	const long double r_dot = (v_bold.x * n_hat.x) + (v_bold.y * n_hat.y) + (v_bold.z * n_hat.z); // radial velocity

	// 2.5PN
	// ------------------------------------------------------------------------------------
	long double A_25PN_1 = (9.0L * ((v_bold.x * v_bold.x) + (v_bold.y * v_bold.y) + (v_bold.z * v_bold.z))); // (9 * v^2)
	long double A_25PN_2 = (17.0L * (mu / r)); // (17 * Gm/r)
	ldvec3 A_25PN_1_2 = (A_25PN_1 + A_25PN_2) * r_dot * n_hat; // (9v^2 + 17Gm/r)
	long double A_25PN_3 = (3.0L * ((v_bold.x * v_bold.x) + (v_bold.y * v_bold.y) + (v_bold.z * v_bold.z))); // (3 * v^2)
	long double A_25PN_4 = (9.0L * (mu / r)); // (9 * Gm/r)
	ldvec3 A_25PN_3_4 = -((A_25PN_3 + A_25PN_4) * v_bold); // (3v^2 + 9Gm/r)
	long double A_25PN_5 = -(8.0L / 15.0L) * n_smr * (mu / r); // -(8/15 * n_smr * Gm/r)

	// 2.5PN
	// Responsible for the orbital decay caused by graviational radiation
	// -8/15 * n_smr * Gm/r * ((9v^2 + 17Gm/r) * r_dot * n_hat - (3v^2 + 9Gm/r) * v_bold)
	ldvec3 A_25PN = A_25PN_5 * (A_25PN_1_2 + A_25PN_3_4);

	std::cout << std::setprecision(20) << "A_25PN_1 = " << A_25PN_1 << std::endl;
	std::cout << std::setprecision(20) << "A_25PN_2 = " << A_25PN_2 << std::endl;
	std::cout << std::setprecision(20) << "A_25PN_1_2 = " << A_25PN_1_2.x << " " << A_25PN_1_2.y << " " << A_25PN_1_2.z << std::endl;
	std::cout << std::setprecision(20) << "A_25PN_3 = " << A_25PN_3 << std::endl;
	std::cout << std::setprecision(20) << "A_25PN_4 = " << A_25PN_4 << std::endl;
	std::cout << std::setprecision(20) << "A_25PN_3_4 = " << A_25PN_3_4.x << " " << A_25PN_3_4.y << " " << A_25PN_3_4.z << std::endl;
	std::cout << std::setprecision(20) << "A_25PN_5 = " << A_25PN_5 << std::endl;
	std::cout << std::setprecision(20) << "A_25PN = " << A_25PN.x << " " << A_25PN.y << " " << A_25PN.z << std::endl;

	// dv_bold/dt ~ a
	// (Gm/r^2)(1/c^5 * A_25PN) 
	ldvec3 a_diss = (mu / (r * r)) * c5 * A_25PN;

	return a_diss;
}

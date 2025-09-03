#define M_PI        3.14159265358979323846264338327950288   /* pi */
#define GLM_ENABLE_EXPERIMENTAL

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Mathematical Constants
// -----------------------------------------------------------------------------------------
constexpr double G_d = 2.9592338593516714e-4; // Newtonian Gravitational Constant in AU^3 / (Msun * day^2)
constexpr double c_d = 173.2632249; // Speed of light in AU / day

constexpr double G_yr = 4.0 * M_PI * M_PI; // Newtonian Gravitational Constant in AU^3 / (Msun * yr^2)
constexpr double c_yr = 63241.0771; // Speed of light in AU / yr

// Function
// -----------------------------------------------------------------------------------------
glm::vec3 PN_acceleration(glm::vec3 pos1, glm::vec3 pos2, glm::vec3 v1, glm::vec3 v2, float m1, float m2, bool years) {
	// Time
	//  ------------------------------------------------------------------------------------
	double G = years ? G_yr : G_d;
	double c = years ? c_yr : c_d;
	std::cout << "G: " << G << std::endl;
	std::cout << "c: " << c << std::endl;
	// Terms
	// -------------------------------------------------------------------------------------
	float m = m1 + m2; // total mass (Msun)
	float mu = G * m; // standard gravitational parameter (AU^3 / {day^2 : yr^2})
	float n_smr = (m1 * m2) / (m * m); // symmetric mass ratio
	glm::dvec3 v_bold = v1 - v2; // vecotr velocity difference (AU / {day^2 : yr^2})
	glm::vec3 sep = pos1 - pos2; // vector seperation (AU)

	double r = glm::length(sep); // scalar seperation (AU)
	double v = glm::length(v_bold); // scalar velocity difference (AU / {day^2 : yr^2})

	glm::dvec3 n_hat = sep / r; // unit vector
	double r_dot = glm::dot(v_bold, n_hat); // radial velocity
	
	
	std::cout << "m = " << m << std::endl;
	std::cout << "n_smr = " << n_smr << std::endl;
	std::cout << "r = " << r << std::endl;
	std::cout << "v = " << v << std::endl;
	std::cout << "r_dot = " << r_dot << std::endl;

	std::cout << "v_bold = " << glm::to_string(v_bold) << std::endl;
	std::cout << "sep = " << glm::to_string(sep) << std::endl;
	std::cout << "n_hat = " << glm::to_string(n_hat) << std::endl;

	// Formulaes
	// -------------------------------------------------------------------------------------
	// More understandable formulae will be commented below the overly bracketed c++ formulaes
	// -------------------------------------------------------------------------------------
	// 1PN
	glm::dvec3 A_1PN = (((4 + 2 * n_smr) * (mu / r) - ((1 + 3 * n_smr) * (v * v)) + ((3 / 2) * (n_smr * r_dot * r_dot))) * n_hat) + (((4 - 2 * n_smr) * r_dot) * v_bold);
	// ((4 + 2 * n_smr)Gm/r - (1 + 3 * n_smr) * v^2 + 3/2 * n_smr * r_dot^2) * n_hat + (4 - 2 * n_smr) * r_dot * v_bold
	std::cout << "A_1PN = " << A_1PN.x << " " << A_1PN.y << " " << A_1PN.z << std::endl;

	// 2PN
	double A_2PN_n_1 = ((3 / 4) * (12 + 29 * n_smr) * ((mu / r) * (mu / r))); // 3/4(12 + 29 * n_smr)(Gm/r)^2
	double A_2PN_n_2 = (n_smr * (3 - 4 * n_smr) * (v * v * v * v)); // n_smr(3 - 4 * n_smr) * v^4
	double A_2PN_n_3 = ((15 / 8) * n_smr * (1 - 3 * n_smr) * (r_dot * r_dot * r_dot * r_dot)); // 15/8 * n_smr(1 - 3 * n_smr) * r_dot^4
	double A_2PN_n_4 = -((3 / 2) * n_smr * (3 - 4 * n_smr) * (v * v) * (r_dot * r_dot)); // -(3/2 * n_smr(3 - 4 * n_smr) * v^2 * r_dot^2)
	double A_2PN_n_5 = -((1 / 2) * n_smr * (13 - 4 * n_smr) * (mu / r) * (v * v)); // -(1/2 * n_smr(13 - 4 * n_smr)Gm/r * v^2
	double A_2PN_n_6 = -(2 + (25 * n_smr) + 2 * (n_smr * n_smr)) * (mu / r) * (r_dot * r_dot); // -(2 + 25 * n_smr + 2 * (n_smr)^2)Gm/r * r_dot^2)
	glm::dvec3 A_2PN_n = (A_2PN_n_1 + A_2PN_n_2 + A_2PN_n_3 + A_2PN_n_4 + A_2PN_n_5 + A_2PN_n_6) * n_hat;

	double A_2PN_v_1 = (n_smr * (15 + 4 * n_smr) * (v * v) * r_dot); // n_smr(15 + 4 * n_smr) * v^2 * r_dot
	double A_2PN_v_2 = -((3 / 2) * n_smr * (3 + 2 * n_smr) * (r_dot * r_dot * r_dot)); // -(3/2 * n_smr(3 + 2n_smr) * r_dot^3)
	double A_2PN_v_3 = -((1 / 2) * (4 + (41 * n_smr) + 8 * (n_smr * n_smr)) * (mu / r) * r_dot); // -(1/2 * (4 + 41 * n_smr + 8 * n_smr^2)Gm/r * r_dot)
	glm::dvec3 A_2PN_v = (A_2PN_v_1 + A_2PN_v_2 + A_2PN_v_3) * v_bold;

	glm::dvec3 A_2PN = A_2PN_n + A_2PN_v;

	std::cout << "A_2PN = " << A_2PN.x << " " << A_2PN.y << " " << A_2PN.z << std::endl;

	// 2.5PN
	// Responsible for the orbital decay caused by graviational radiation
	// -8/15 * n_smr * Gm/r * ((9v^2 + 17Gm/r) * r_dot * n_hat - (3v^2 + 9Gm/r) * v_bold)
	glm::dvec3 A_25PN = -(8 / 15) * n_smr * (mu / r) * ((((9 * v * v) + (17 * (mu / r))) * r_dot * n_hat) - ((3 * v * v) + 9 * (mu / r)) * v_bold);
	std::cout << "A_25PN = " << " " << A_25PN.x << " " << A_25PN.y << " " << A_25PN.z << std::endl;

	const double c2 = (1 / (c * c));
	const double c4 = (1 / (c * c * c * c));
	const double c5 = (1 / (c * c * c * c * c));
	// dv_bold/dt ~ a
	// Gm/r^2(-n_hat + (1/c^2)(A_1PN) + (1/c^4)(A_2PN) + (1/c^5)(A_2PN))
	glm::dvec3 a = (mu / (r * r)) * (-n_hat + (c2 * A_1PN) + (c4 * A_2PN) + (c5 * A_25PN));
	std::cout << "a = " << a.x << " " << a.y << " " << a.z << std::endl;

	return a;
}

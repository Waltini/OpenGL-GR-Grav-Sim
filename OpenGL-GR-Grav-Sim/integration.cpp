#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "formulae.h"
#include "celestial_body_class.h"
#include "integration.h"

#include <iostream>
#include <iomanip>

using ldvec3 = glm::tvec3<long double>;

void step(celestial_body& b1, celestial_body& b2, long double& dt, bool years) {
	const long double m1 = b1.getMass();
	const long double m2 = b2.getMass();

	// Half-Step ~ Dissipative
	ldvec3 a_diss_1_rel = PN_accel_diss(b1.getPos(), b2.getPos(), b1.getVel(), b2.getVel(), m1, m2, years);
	ldvec3 a_diss_1_b1, a_diss_1_b2;
	resolve_rel_accel(a_diss_1_rel, a_diss_1_b1, a_diss_1_b2, m1, m2);
	b1.updateVel(0.5L * dt * a_diss_1_b1);
	b2.updateVel(0.5L * dt * a_diss_1_b2);

	// Full Step ~ Conservative 
	ldvec3 a_cons_1_rel = PN_accel_cons(b1.getPos(), b2.getPos(), b1.getVel(), b2.getVel(), m1, m2, years);
	ldvec3 a_cons_1_b1, a_cons_1_b2;
	resolve_rel_accel(a_cons_1_rel, a_cons_1_b1, a_cons_1_b2, m1, m2);
	b1.updatePos(b1.getVel() * dt + 0.5L * (dt * dt) * a_cons_1_b1);
	b2.updatePos(b2.getVel() * dt + 0.5L * (dt * dt) * a_cons_1_b2);

	ldvec3 a_cons_2_rel = PN_accel_cons(b1.getPos(), b2.getPos(), b1.getVel(), b2.getVel(), m1, m2, years);
	ldvec3 a_cons_2_b1, a_cons_2_b2;
	resolve_rel_accel(a_cons_2_rel, a_cons_2_b1, a_cons_2_b2, m1, m2);
	b1.updateVel(0.5L * dt * (a_cons_1_b1 + a_cons_2_b1));
	b2.updateVel(0.5L * dt * (a_cons_1_b2 + a_cons_2_b2));

	// Half-step ~ Dissipative
	ldvec3 a_diss_2_rel = PN_accel_diss(b1.getPos(), b2.getPos(), b1.getVel(), b2.getVel(), m1, m2, years);
	ldvec3 a_diss_2_b1, a_diss_2_b2;
	resolve_rel_accel(a_diss_2_rel, a_diss_2_b1, a_diss_2_b2, m1, m2);
	b1.updateVel(0.5L * dt * a_diss_2_b1);
	b2.updateVel(0.5L * dt * a_diss_2_b2);
}


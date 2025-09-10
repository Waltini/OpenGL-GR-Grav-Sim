#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "formulae.h"
#include "celestial_body_class.h"
#include "integration.h"

#include <iostream>

using ldvec3 = glm::tvec3<long double>;

void step(celestial_body& b1, celestial_body& b2, long double& dt, bool years) {
	const long double m1 = b1.getMass();
	const long double m2 = b2.getMass();

	// Half-Step ~ Dissipative
	ldvec3 a_diss_1 = PN_accel_diss(b1.getPos(), b2.getPos(), b1.getVel(), b2.getVel(), m1, m2, years);
	ldvec3 a_diss_2 = PN_accel_diss(b2.getPos(), b1.getPos(), b2.getVel(), b1.getVel(), m1, m2, years);
	b1.updateVel(0.5L * dt * a_diss_1);
	b2.updateVel(0.5L * dt * a_diss_2);

	// Full Step ~ Conservative 
	ldvec3 a_cons_1a = PN_accel_cons(b1.getPos(), b2.getPos(), b1.getVel(), b2.getVel(), m1, m2, years);
	ldvec3 a_cons_2a = PN_accel_cons(b2.getPos(), b1.getPos(), b2.getVel(), b1.getVel(), m1, m2, years);
	b1.updatePos(b1.getVel() * dt + 0.5L * (dt * dt) * a_cons_1a);
	b2.updatePos(b2.getVel() * dt + 0.5L * (dt * dt) * a_cons_2a);

	ldvec3 a_cons_1b = PN_accel_cons(b1.getPos(), b2.getPos(), b1.getVel(), b2.getVel(), m1, m2, years);
	ldvec3 a_cons_2b = PN_accel_cons(b2.getPos(), b1.getPos(), b2.getVel(), b1.getVel(), m1, m2, years);
	b1.updateVel(0.5L * dt * (a_cons_1a + a_cons_1b));
	b2.updateVel(0.5L * dt * (a_cons_2a + a_cons_2b));

	// Half-step ~ Dissipative
	a_diss_1 = PN_accel_diss(b1.getPos(), b2.getPos(), b1.getVel(), b2.getVel(), m1, m2, years);
	a_diss_2 = PN_accel_diss(b2.getPos(), b1.getPos(), b2.getVel(), b1.getVel(), m1, m2, years);
	b1.updateVel(0.5L * dt * a_diss_1);
	b2.updateVel(0.5L * dt * a_diss_2);
}


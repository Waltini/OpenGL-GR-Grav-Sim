#ifndef celestial_body_class_H
#define celestial_body_class_H

#include <glm/glm.hpp>
#include <iostream>
#include <iomanip>

using ldvec3 = glm::tvec3<long double>;

class celestial_body {
private:
    ldvec3 pos;
    ldvec3 vel;
    long double mass;
public:
    // Constructor
    celestial_body(ldvec3 position, ldvec3 velocity, long double m)
        : pos(position), vel(velocity), mass(m) {
        if (m <= 0) {
            mass = 1.0L;
        }
    }

    // Getters
    ldvec3 getPos() const { return pos; }
    ldvec3 getVel() const { return vel; }
    long double getMass() const { return mass; }

    // Setters
    void setPos(const ldvec3& p) { pos = p; }
    void setVel(const ldvec3& v) { vel = v; }

    // Update
    void updatePos(ldvec3 p) {
        pos += p;
    }
    void updateVel(ldvec3 v) {
        vel += v;
    }

    // Debug
    void print() const {
        std::cout << std::setprecision(20) << "pos = " << pos.x << " " << pos.y << " " << pos.z << std::endl;
        std::cout << std::setprecision(20) << "vel = " << vel.x << " " << vel.y << " " << vel.z << std::endl;
        std::cout << std::setprecision(20) << "mass = " << mass << std::endl;
    }
};

#endif // MYCLASS_H

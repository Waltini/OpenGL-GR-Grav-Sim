#ifndef celestial_body_class_H
#define celestial_body_class_H
#define M_PI        3.14159265358979323846264338327950288   /* pi */

#include <glm/glm.hpp>
#include <iostream>
#include <iomanip>
#include <cmath>

using dvec3 = glm::dvec3;

class celestial_body {
private:
    dvec3 pos;
    dvec3 vel;
    double mass;
    double radius = calcSphereRadius(mass);

    double calcSphereRadius(double M) {
        if (M > 3.003e-6) {
            return (10e2 * std::cbrt((3 * (M - 3.003e-6)) / (4 * M_PI * 9247304))) + 0.5;
        }
        else {
            return 0.5;
        }
    }

public:
    // Constructor
    celestial_body(dvec3 position, dvec3 velocity, double m)
        : pos(position), vel(velocity), mass(m) {
        if (m <= 0) {
            mass = 1.0;
        }
    }

    // Getters
    dvec3 getPos() const { return pos; } // Returns position of the object
    dvec3 getVel() const { return vel; } // Returns velocity of the object
    double getMass() const { return mass; } // Returns mass of the object
    double getRadius() const { return radius; } // Returns radius of the object

    // Setters
    void setPos(const dvec3& p) { pos = p; } // Sets the position of the object (used by the buffer box front buffer updating, and ImGUI edit applying)
    void setVel(const dvec3& v) { vel = v; } // Sets the velocity of the object (used by the buffer box front buffer updating, and ImGUI edit applying)
    void setMass(const double& m) { 
        mass = m;
        radius = calcSphereRadius(m);
    } // Sets the mass of the object and updates the radius with the new mass (used only by ImGUI edit applying)

    // Debug
    void print() const {
        std::cout << std::setprecision(20) << "pos = " << pos.x << " " << pos.y << " " << pos.z << std::endl;
        std::cout << std::setprecision(20) << "vel = " << vel.x << " " << vel.y << " " << vel.z << std::endl;
        std::cout << std::setprecision(20) << "mass = " << mass << std::endl;
        std::cout << std::setprecision(20) << "radius = " << radius << std::endl;
    }
};

#endif // MYCLASS_H

#ifndef celestial_body_class_H
#define celestial_body_class_H

#include <glm/glm.hpp>
#include <iostream>

class celestial_body {
private:
    glm::dvec3 pos;
    glm::dvec3 vec;
    double mass;
public:
    // Constructor
    celestial_body(glm::dvec3 position, glm::dvec3 velocity, double m)
        : pos(position), vec(velocity), mass(m) {
    }

    // Getters
    glm::dvec3 getPos() const { return pos; }
    glm::dvec3 getVec() const { return vec; }
    double getMass() const { return mass; }

    // Setters
    void setPos(const glm::dvec3& p) { pos = p; }
    void setVec(const glm::dvec3& v) { vec = v; }

    // Update
    //
    // Debug
    void print() const {
        std::cout << "pos = " << pos.x << " " << pos.y << " " << pos.z << std::endl;
        std::cout << "vec = " << vec.x << " " << vec.y << " " << vec.z << std::endl;
        std::cout << "mass = " << mass << std::endl;
    }
};

#endif // MYCLASS_H

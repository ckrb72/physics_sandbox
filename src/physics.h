#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

/*
    Important Assumptions:
    Right now just treating gravity like it is on earth.
    If we wanted to simulate gravitational force between two objects (not just earth and any object)
    then we would need to use the classic f = G(m1m2)/r^2 equation.
*/

#define grav 9.8

struct PhysicsParticle
{
    glm::vec3 position = glm::vec3(0.0);
    glm::vec3 velocity = glm::vec3(0.0);
    glm::vec3 acceleration = glm::vec3(0.0, -grav, 0.0);
    float damping = 1.0;    // Might want to make this less than 1 to account for accuracy issues that might add more to v than we want
    float mass_inv = 0;     // store the inverse of the math so we can easily represent infinite mass (mass_inv = 0);
};


void phys_integrate(PhysicsParticle& particle, float delta);
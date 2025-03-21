#include "physics.h"
#include <iostream>

void phys_integrate(PhysicsParticle& p, float delta)
{
    // Since delta is so small we can ignore the acceleration part of this equation
    // In certain cases we might want to use the whole formula:
    // position = init_position + velocity * delta + acceleration * delta * delta
    p.position += (p.velocity * delta);

    // update acceleration when we add forces
    // p.acceleration += mass_inv * force_accumulation;     // a = f/m;

    p.velocity += (p.acceleration * delta);

    // apply drag
    p.velocity *= powf(p.damping, delta);
}
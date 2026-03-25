#include "ParticleSystem.h"
#include <GL/glut.h>

// Particle class implementation
Particle::Particle(double x, double y, double z, double vx, double vy, double vz, double lifetime, double size, double r, double g, double b) {
    this->x = x;
    this->y = y;
    this->z = z;
    this->vx = vx;
    this->vy = vy;
    this->vz = vz;
    this->lifetime = lifetime;
    this->maxLifetime = lifetime;
    this->size = size;
    this->color[0] = r;
    this->color[1] = g;
    this->color[2] = b;
}

void Particle::update(double dt) {
    x += vx * dt;
    y += vy * dt;
    z += vz * dt;
    lifetime -= dt;
}

bool Particle::isAlive() const {
    return lifetime > 0;
}

// ParticleSystem class implementation
ParticleSystem::ParticleSystem(int maxParticles) {
    this->maxParticles = maxParticles;
    particles.reserve(maxParticles);
}

void ParticleSystem::addParticle(double x, double y, double z, double vx, double vy, double vz, double lifetime, double size, double r, double g, double b) {
    if (particles.size() < maxParticles) {
        particles.emplace_back(x, y, z, vx, vy, vz, lifetime, size, r, g, b);
    }
}

void ParticleSystem::update(double dt) {
    for (int i = particles.size() - 1; i >= 0; --i) {
        particles[i].update(dt);
        if (!particles[i].isAlive()) {
            particles.erase(particles.begin() + i);
        }
    }
}

void ParticleSystem::render() {
    glEnable(GL_POINT_SMOOTH);
    glBegin(GL_POINTS);
    
    for (const auto& particle : particles) {
        double alpha = particle.lifetime / particle.maxLifetime;
        glColor4f(particle.color[0], particle.color[1], particle.color[2], alpha);
        glPointSize(particle.size * alpha);
        glVertex3f(particle.x, particle.y, particle.z);
    }
    
    glEnd();
    glDisable(GL_POINT_SMOOTH);
}

void ParticleSystem::clear() {
    particles.clear();
}

int ParticleSystem::getParticleCount() const {
    return particles.size();
}

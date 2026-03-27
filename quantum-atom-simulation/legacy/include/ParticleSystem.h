#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <vector>

class Particle {
public:
    double x, y, z;
    double vx, vy, vz;
    double lifetime;
    double maxLifetime;
    double size;
    double color[3];
    
    Particle(double x, double y, double z, double vx, double vy, double vz, double lifetime, double size, double r, double g, double b);
    void update(double dt);
    bool isAlive() const;
};

class ParticleSystem {
private:
    std::vector<Particle> particles;
    int maxParticles;
    
public:
    ParticleSystem(int maxParticles = 1000);
    
    void addParticle(double x, double y, double z, double vx, double vy, double vz, double lifetime, double size, double r, double g, double b);
    void update(double dt);
    void render();
    void clear();
    int getParticleCount() const;
};

#endif // PARTICLE_SYSTEM_H

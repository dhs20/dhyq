#include "WaveParticleModel.h"
#include <GL/glut.h>
#include <cmath>

// Define M_PI if not defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

WaveParticleModel::WaveParticleModel(int atomicNumber) : AtomModel(atomicNumber) {
    waveAmplitude = 0.5;
    waveFrequency = 2.0;
    waveSpeed = 1.0;
    showWave = true;
    showParticle = true;
    
    // Initialize particle position and velocity
    particlePosition[0] = 0.0;
    particlePosition[1] = 0.0;
    particlePosition[2] = 0.0;
    particleVelocity[0] = 0.1;
    particleVelocity[1] = 0.1;
    particleVelocity[2] = 0.0;
}

void WaveParticleModel::update(double dt) {
    AtomModel::update(dt);
    
    double deltaTime = animationController.getDeltaTime();
    
    // Update particle position
    particlePosition[0] += particleVelocity[0] * deltaTime;
    particlePosition[1] += particleVelocity[1] * deltaTime;
    particlePosition[2] += particleVelocity[2] * deltaTime;
    
    // Boundary check
    if (fabs(particlePosition[0]) > 5.0) particleVelocity[0] = -particleVelocity[0];
    if (fabs(particlePosition[1]) > 5.0) particleVelocity[1] = -particleVelocity[1];
    if (fabs(particlePosition[2]) > 5.0) particleVelocity[2] = -particleVelocity[2];
}

void WaveParticleModel::render() {
    double time = animationController.getTime();
    
    // Render wave function
    if (showWave) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glBegin(GL_POINTS);
        for (double x = -5.0; x <= 5.0; x += 0.1) {
            for (double y = -5.0; y <= 5.0; y += 0.1) {
                for (double z = -5.0; z <= 5.0; z += 0.1) {
                    double value = calculateWaveFunction(x, y, z, time);
                    double intensity = fabs(value);
                    
                    if (intensity > 0.1) {
                        glColor4f(0.0, 0.0, 1.0, intensity);
                        glVertex3f(x, y, z);
                    }
                }
            }
        }
        glEnd();
        
        glDisable(GL_BLEND);
    }
    
    // Render particle
    if (showParticle) {
        glColor3f(1.0, 0.0, 0.0);
        glPushMatrix();
        glTranslatef(particlePosition[0], particlePosition[1], particlePosition[2]);
        glutSolidSphere(electronRadius * 2, 16, 16);
        glPopMatrix();
    }
    
    // Render particle system
    particleSystem.render();
}

void WaveParticleModel::reset() {
    AtomModel::reset();
    
    // Reset particle position and velocity
    particlePosition[0] = 0.0;
    particlePosition[1] = 0.0;
    particlePosition[2] = 0.0;
    particleVelocity[0] = 0.1;
    particleVelocity[1] = 0.1;
    particleVelocity[2] = 0.0;
}

void WaveParticleModel::setShowWave(bool show) {
    showWave = show;
}

void WaveParticleModel::setShowParticle(bool show) {
    showParticle = show;
}

double WaveParticleModel::calculateWaveFunction(double x, double y, double z, double t) const {
    double r = sqrt(x*x + y*y + z*z);
    double k = 2 * M_PI * waveFrequency / waveSpeed;
    double omega = 2 * M_PI * waveFrequency;
    
    return waveAmplitude * sin(k * r - omega * t) / r;
}

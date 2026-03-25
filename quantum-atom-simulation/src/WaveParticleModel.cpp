#include "WaveParticleModel.h"
#include "Utils.h"
#include <GL/glut.h>
#include <cmath>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

WaveParticleModel::WaveParticleModel(int atomicNumber, int neutronNumber) : AtomModel(atomicNumber, neutronNumber) {
    waveAmplitude = 0.5;
    waveFrequency = 2.0;
    waveSpeed = 1.0;
    showWave = true;
    showParticle = true;
    deBroglieLambda = 0.0;
    standingWaveRatio = 0.0;

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

    particlePosition[0] += particleVelocity[0] * deltaTime;
    particlePosition[1] += particleVelocity[1] * deltaTime;
    particlePosition[2] += particleVelocity[2] * deltaTime;

    if (fabs(particlePosition[0]) > 5.0) particleVelocity[0] = -particleVelocity[0];
    if (fabs(particlePosition[1]) > 5.0) particleVelocity[1] = -particleVelocity[1];
    if (fabs(particlePosition[2]) > 5.0) particleVelocity[2] = -particleVelocity[2];

    double velocity = std::sqrt(
        particleVelocity[0] * particleVelocity[0] +
        particleVelocity[1] * particleVelocity[1] +
        particleVelocity[2] * particleVelocity[2]);

    deBroglieLambda = Utils::deBroglieWavelength(Utils::ELECTRON_MASS, velocity);
    double orbitCircumference = 2.0 * M_PI * std::max(1.0, std::sqrt(
        particlePosition[0] * particlePosition[0] +
        particlePosition[1] * particlePosition[1]));

    if (deBroglieLambda > 0.0) {
        standingWaveRatio = orbitCircumference / deBroglieLambda;
    } else {
        standingWaveRatio = 0.0;
    }
}

void WaveParticleModel::render() {
    double time = animationController.getTime();

    if (showWave) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBegin(GL_POINTS);
        for (double x = -5.0; x <= 5.0; x += 0.15) {
            for (double y = -5.0; y <= 5.0; y += 0.15) {
                for (double z = -5.0; z <= 5.0; z += 0.15) {
                    double value = calculateWaveFunction(x, y, z, time);
                    double intensity = fabs(value);

                    if (intensity > 0.1) {
                        glColor4f(0.0, 0.0, 1.0, std::min(1.0, intensity));
                        glVertex3f(x, y, z);
                    }
                }
            }
        }
        glEnd();

        glDisable(GL_BLEND);
    }

    if (showParticle) {
        glColor3f(1.0, 0.0, 0.0);
        glPushMatrix();
        glTranslatef(particlePosition[0], particlePosition[1], particlePosition[2]);
        glutSolidSphere(electronRadius * 2, 16, 16);
        glPopMatrix();
    }

    renderInfoPanel();
    particleSystem.render();
}

void WaveParticleModel::reset() {
    AtomModel::reset();
    particlePosition[0] = 0.0;
    particlePosition[1] = 0.0;
    particlePosition[2] = 0.0;
    particleVelocity[0] = 0.1;
    particleVelocity[1] = 0.1;
    particleVelocity[2] = 0.0;
    deBroglieLambda = 0.0;
    standingWaveRatio = 0.0;
}

void WaveParticleModel::setShowWave(bool show) {
    showWave = show;
}

void WaveParticleModel::setShowParticle(bool show) {
    showParticle = show;
}

double WaveParticleModel::calculateWaveFunction(double x, double y, double z, double t) const {
    double r = sqrt(x*x + y*y + z*z);
    if (r < 1e-8) {
        return 0.0;
    }

    double k = 2 * M_PI * waveFrequency / waveSpeed;
    double omega = 2 * M_PI * waveFrequency;

    return waveAmplitude * sin(k * r - omega * t) / r;
}

void WaveParticleModel::renderInfoPanel() const {
    char info[200];
    std::snprintf(info, sizeof(info), "de Broglie lambda=%.2e m | standing-wave ratio=%.2f", deBroglieLambda, standingWaveRatio);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(-0.95f, 0.9f);
    for (const char* c = info; *c != '\0'; ++c) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

#include "ThreeDRenderModel.h"
#include "SchrodingerSolver.h"
#include "Utils.h"
#include <GL/glut.h>
#include <cmath>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ThreeDRenderModel::ThreeDRenderModel(int atomicNumber, int neutronNumber) : AtomModel(atomicNumber, neutronNumber) {
    quantumNumbers[0] = 1;
    quantumNumbers[1] = 0;
    quantumNumbers[2] = 0;
    resolution = 50;
    cloudRadius = 5.0;
    showOrbitals = true;
    probabilityThreshold = 0.08;
    quantumStatePresets = SchrodingerSolver::supportedQuantumStates();
    activePresetIndex = 0;
    generateProbabilityCloud();
}

void ThreeDRenderModel::update(double dt) {
    AtomModel::update(dt);
}

void ThreeDRenderModel::render() {
    renderNucleus();

    if (showOrbitals) {
        renderOrbitals();
    }

    particleSystem.render();
    renderInfoPanel();
}

void ThreeDRenderModel::reset() {
    AtomModel::reset();
    generateProbabilityCloud();
}

void ThreeDRenderModel::setQuantumNumbers(int n, int l, int m) {
    quantumNumbers[0] = n;
    quantumNumbers[1] = l;
    quantumNumbers[2] = m;
    generateProbabilityCloud();
}

void ThreeDRenderModel::setShowOrbitals(bool show) {
    showOrbitals = show;
}

void ThreeDRenderModel::setProbabilityThreshold(double threshold) {
    probabilityThreshold = std::max(0.01, std::min(0.9, threshold));
    generateProbabilityCloud();
}

void ThreeDRenderModel::nextQuantumPreset() {
    if (quantumStatePresets.empty()) {
        return;
    }

    activePresetIndex = (activePresetIndex + 1) % quantumStatePresets.size();
    const auto& state = quantumStatePresets[activePresetIndex];
    setQuantumNumbers(state[0], state[1], state[2]);
}

void ThreeDRenderModel::generateProbabilityCloud() {
    particleSystem.clear();

    int n = quantumNumbers[0];
    int l = quantumNumbers[1];
    int m = quantumNumbers[2];

    for (int i = 0; i < 25000; ++i) {
        double r = Utils::random(0, cloudRadius);
        double theta = Utils::random(0, M_PI);
        double phi = Utils::random(0, 2 * M_PI);

        double probability = Utils::probabilityDensity(n, l, m, r, theta, phi);
        double gate = Utils::random(0, 1.0);

        if (probability > probabilityThreshold * gate) {
            auto cartesian = Utils::sphericalToCartesian(r, theta, phi);
            particleSystem.addParticle(cartesian[0], cartesian[1], cartesian[2], 0, 0, 0, 1000, 0.05, 0.2, 0.6, 1.0);
        }
    }
}

void ThreeDRenderModel::renderNucleus() {
    glColor3f(1.0, 0.0, 0.0);
    glutSolidSphere(nucleusRadius, 32, 32);
}

void ThreeDRenderModel::renderOrbitals() {
    int l = quantumNumbers[1];

    glColor3f(0.5, 0.5, 0.5);
    glLineWidth(1.0);

    if (l == 0) {
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 100; ++i) {
            double angle = 2 * M_PI * i / 100;
            glVertex3f(cloudRadius * cos(angle), cloudRadius * sin(angle), 0.0);
        }
        glEnd();
    } else if (l == 1) {
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 100; ++i) {
            double angle = 2 * M_PI * i / 100;
            glVertex3f(cloudRadius * cos(angle) / sqrt(2), cloudRadius * sin(angle) / sqrt(2), 0.0);
        }
        glEnd();
    } else if (l == 2) {
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 100; ++i) {
            double angle = 2 * M_PI * i / 100;
            double r = cloudRadius * sin(2 * angle) / 2;
            glVertex3f(r * cos(angle), r * sin(angle), 0.0);
        }
        glEnd();
    }
}

void ThreeDRenderModel::renderInfoPanel() {
    char info[200];
    std::snprintf(info, sizeof(info), "3D cloud n=%d l=%d m=%d | threshold=%.2f | presets=%zu",
        quantumNumbers[0], quantumNumbers[1], quantumNumbers[2], probabilityThreshold, quantumStatePresets.size());

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

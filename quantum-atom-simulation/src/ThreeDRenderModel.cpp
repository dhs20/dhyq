#include "ThreeDRenderModel.h"
#include "Utils.h"
#include <GL/glut.h>
#include <cmath>

// Define M_PI if not defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ThreeDRenderModel::ThreeDRenderModel(int atomicNumber) : AtomModel(atomicNumber) {
    quantumNumbers[0] = 1; // n
    quantumNumbers[1] = 0; // l
    quantumNumbers[2] = 0; // m
    resolution = 50;
    cloudRadius = 5.0;
    showOrbitals = true;
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

void ThreeDRenderModel::generateProbabilityCloud() {
    particleSystem.clear();
    
    int n = quantumNumbers[0];
    int l = quantumNumbers[1];
    int m = quantumNumbers[2];
    
    for (int i = 0; i < 10000; ++i) {
        double r = Utils::random(0, cloudRadius);
        double theta = Utils::random(0, M_PI);
        double phi = Utils::random(0, 2 * M_PI);
        
        double probability = Utils::probabilityDensity(n, l, m, r, theta, phi);
        
        if (probability > Utils::random(0, 1.0)) {
            auto cartesian = Utils::sphericalToCartesian(r, theta, phi);
            double x = cartesian[0];
            double y = cartesian[1];
            double z = cartesian[2];
            
            particleSystem.addParticle(x, y, z, 0, 0, 0, 1000, 0.05, 0.0, 0.0, 1.0);
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
    
    if (l == 0) { // s轨道
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 100; ++i) {
            double angle = 2 * M_PI * i / 100;
            double x = cloudRadius * cos(angle);
            double y = cloudRadius * sin(angle);
            glVertex3f(x, y, 0.0);
        }
        glEnd();
        
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 100; ++i) {
            double angle = 2 * M_PI * i / 100;
            double x = cloudRadius * cos(angle);
            double z = cloudRadius * sin(angle);
            glVertex3f(x, 0.0, z);
        }
        glEnd();
        
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 100; ++i) {
            double angle = 2 * M_PI * i / 100;
            double y = cloudRadius * cos(angle);
            double z = cloudRadius * sin(angle);
            glVertex3f(0.0, y, z);
        }
        glEnd();
    } else if (l == 1) { // p轨道
        // pz轨道
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 100; ++i) {
            double angle = 2 * M_PI * i / 100;
            double x = cloudRadius * cos(angle) / sqrt(2);
            double y = cloudRadius * sin(angle) / sqrt(2);
            glVertex3f(x, y, 0.0);
        }
        glEnd();
        
        // px轨道
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 100; ++i) {
            double angle = 2 * M_PI * i / 100;
            double y = cloudRadius * cos(angle) / sqrt(2);
            double z = cloudRadius * sin(angle) / sqrt(2);
            glVertex3f(0.0, y, z);
        }
        glEnd();
        
        // py轨道
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 100; ++i) {
            double angle = 2 * M_PI * i / 100;
            double x = cloudRadius * cos(angle) / sqrt(2);
            double z = cloudRadius * sin(angle) / sqrt(2);
            glVertex3f(x, 0.0, z);
        }
        glEnd();
    } else if (l == 2) { // d轨道
        // dxy轨道
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 100; ++i) {
            double angle = 2 * M_PI * i / 100;
            double r = cloudRadius * sin(2 * angle) / 2;
            double x = r * cos(angle);
            double y = r * sin(angle);
            glVertex3f(x, y, 0.0);
        }
        glEnd();
        
        // dxz轨道
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 100; ++i) {
            double angle = 2 * M_PI * i / 100;
            double r = cloudRadius * sin(2 * angle) / 2;
            double x = r * cos(angle);
            double z = r * sin(angle);
            glVertex3f(x, 0.0, z);
        }
        glEnd();
        
        // dyz轨道
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 100; ++i) {
            double angle = 2 * M_PI * i / 100;
            double r = cloudRadius * sin(2 * angle) / 2;
            double y = r * cos(angle);
            double z = r * sin(angle);
            glVertex3f(0.0, y, z);
        }
        glEnd();
    }
}

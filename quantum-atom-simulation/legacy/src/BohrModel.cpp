#include "BohrModel.h"
#include <GL/glut.h>
#include <cmath>

// Define M_PI if not defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

BohrModel::BohrModel(int atomicNumber) : AtomModel(atomicNumber) {
    nucleusX = 0.0;
    nucleusY = 0.0;
    nucleusZ = 0.0;
    initializeElectrons();
}

void BohrModel::update(double dt) {
    AtomModel::update(dt);
    
    double deltaTime = animationController.getDeltaTime();
    
    for (auto& electron : electrons) {
        electron.angle += electron.speed * deltaTime;
        if (electron.angle >= 2 * M_PI) {
            electron.angle -= 2 * M_PI;
        }
    }
}

void BohrModel::render() {
    // Render nucleus
    glPushMatrix();
    glTranslatef(nucleusX, nucleusY, nucleusZ);
    glColor3f(1.0, 0.0, 0.0);
    glutSolidSphere(nucleusRadius, 32, 32);
    glPopMatrix();
    
    // Render orbits
    glColor3f(0.5, 0.5, 0.5);
    glLineWidth(1.0);
    
    for (const auto& electron : electrons) {
        glPushMatrix();
        glTranslatef(nucleusX, nucleusY, nucleusZ);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 100; ++i) {
            double angle = 2 * M_PI * i / 100;
            double x = electron.radius * cos(angle);
            double y = electron.radius * sin(angle);
            glVertex3f(x, y, 0.0);
        }
        glEnd();
        glPopMatrix();
    }
    
    // Render electrons
    glColor3f(0.0, 0.0, 1.0);
    
    for (const auto& electron : electrons) {
        double x = nucleusX + electron.radius * cos(electron.angle);
        double y = nucleusY + electron.radius * sin(electron.angle);
        double z = nucleusZ;
        
        glPushMatrix();
        glTranslatef(x, y, z);
        glutSolidSphere(electronRadius, 16, 16);
        glPopMatrix();
    }
    
    // Render particle system
    particleSystem.render();
}

void BohrModel::reset() {
    AtomModel::reset();
    initializeElectrons();
}

void BohrModel::initializeElectrons() {
    electrons.clear();
    
    int n = 1;
    int electronsAdded = 0;
    
    while (electronsAdded < atomicNumber) {
        int maxElectrons = 2 * n * n;
        for (int i = 0; i < maxElectrons && electronsAdded < atomicNumber; ++i) {
            Electron electron;
            electron.energyLevel = n;
            electron.angle = 2 * M_PI * i / maxElectrons;
            electron.radius = calculateOrbitalRadius(n);
            electron.speed = calculateOrbitalSpeed(n);
            electrons.push_back(electron);
            electronsAdded++;
        }
        n++;
    }
}

double BohrModel::calculateOrbitalRadius(int n) const {
    return 0.529 * n * n; // Bohr radius unit: angstrom
}

double BohrModel::calculateOrbitalSpeed(int n) const {
    return 2.188e6 / n; // Unit: m/s, normalized
}

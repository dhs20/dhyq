#include "BohrModel.h"
#include "Utils.h"
#include <algorithm>
#include <GL/glut.h>
#include <cmath>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

BohrModel::BohrModel(int atomicNumber, int neutronNumber) : AtomModel(atomicNumber, neutronNumber) {
    nucleusX = 0.0;
    nucleusY = 0.0;
    nucleusZ = 0.0;
    selectedElectronIndex = 0;
    transitionActive = false;
    autoTransition = false;
    transitionTimer = 0.0;
    transitionDuration = 1.0;
    transitionFrom = 1;
    transitionTo = 1;
    transitionEnergyEV = 0.0;
    photonFrequencyHz = 0.0;
    photonWavelengthNm = 0.0;
    entropyDelta = 0.0;
    initializeElectrons();
}

void BohrModel::update(double dt) {
    AtomModel::update(dt);

    double deltaTime = animationController.getDeltaTime();
    transitionTimer += deltaTime;

    if (autoTransition && transitionTimer > 2.5 && !electrons.empty()) {
        triggerTransition(((int)(transitionTimer * 10) % 2) == 0);
        transitionTimer = 0.0;
    }

    for (auto& electron : electrons) {
        electron.angle += electron.speed * deltaTime;
        if (electron.angle >= 2 * M_PI) {
            electron.angle -= 2 * M_PI;
        }
    }

    if (transitionActive && !electrons.empty()) {
        Electron& electron = electrons[selectedElectronIndex % electrons.size()];
        double progress = deltaTime / transitionDuration;
        double targetRadius = calculateOrbitalRadius(transitionTo);
        electron.radius += (targetRadius - electron.radius) * progress * 5.0;

        if (std::fabs(electron.radius - targetRadius) < 0.01) {
            electron.radius = targetRadius;
            electron.energyLevel = transitionTo;
            electron.speed = calculateOrbitalSpeed(transitionTo);
            transitionActive = false;
        }
    }
}

void BohrModel::render() {
    glPushMatrix();
    glTranslatef(nucleusX, nucleusY, nucleusZ);
    glColor3f(1.0, 0.2, 0.2);
    glutSolidSphere(nucleusRadius, 32, 32);

    glColor3f(0.7, 0.7, 0.7);
    for (int i = 0; i < neutronNumber; ++i) {
        glPushMatrix();
        glTranslatef(0.03f * std::cos(i), 0.03f * std::sin(i), 0.0f);
        glutSolidSphere(nucleusRadius * 0.4, 8, 8);
        glPopMatrix();
    }
    glPopMatrix();

    glColor3f(0.5, 0.5, 0.5);
    glLineWidth(1.0);
    for (const auto& electron : electrons) {
        glPushMatrix();
        glTranslatef(nucleusX, nucleusY, nucleusZ);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 120; ++i) {
            double angle = 2 * M_PI * i / 120;
            double x = electron.radius * std::cos(angle);
            double y = electron.radius * std::sin(angle);
            glVertex3f(x, y, 0.0);
        }
        glEnd();
        glPopMatrix();
    }

    glColor3f(0.1, 0.4, 1.0);
    for (const auto& electron : electrons) {
        double x = nucleusX + electron.radius * std::cos(electron.angle);
        double y = nucleusY + electron.radius * std::sin(electron.angle);

        glPushMatrix();
        glTranslatef(x, y, nucleusZ);
        glutSolidSphere(electronRadius, 16, 16);
        glPopMatrix();
    }

    char info[256];
    std::snprintf(info, sizeof(info),
        "Z=%d N=%d | transition n%d->n%d | E=%.3f eV | lambda=%.2f nm | f=%.2e Hz | dS=%.2e J/K",
        atomicNumber, neutronNumber, transitionFrom, transitionTo,
        transitionEnergyEV, photonWavelengthNm, photonFrequencyHz, entropyDelta);
    renderText(-0.95f, 0.9f, info);

    particleSystem.render();
}

void BohrModel::reset() {
    AtomModel::reset();
    transitionActive = false;
    transitionFrom = 1;
    transitionTo = 1;
    transitionEnergyEV = 0.0;
    photonFrequencyHz = 0.0;
    photonWavelengthNm = 0.0;
    entropyDelta = 0.0;
    initializeElectrons();
}

void BohrModel::triggerTransition(bool upward) {
    if (electrons.empty()) {
        return;
    }

    Electron& electron = electrons[selectedElectronIndex % electrons.size()];
    transitionFrom = electron.energyLevel;
    transitionTo = upward ? std::min(6, transitionFrom + 1) : std::max(1, transitionFrom - 1);

    if (transitionFrom == transitionTo) {
        return;
    }

    double before = calculateLevelEnergyEV(transitionFrom);
    double after = calculateLevelEnergyEV(transitionTo);
    transitionEnergyEV = std::fabs(after - before);

    double energyJ = transitionEnergyEV * Utils::ELECTRON_CHARGE;
    photonFrequencyHz = energyJ / (2.0 * M_PI * Utils::H_BAR);
    if (photonFrequencyHz > 0) {
        photonWavelengthNm = 299792458.0 / photonFrequencyHz * 1e9;
    } else {
        photonWavelengthNm = 0.0;
    }
    entropyDelta = Utils::entropyChange(energyJ, 300.0);

    transitionActive = true;
}

void BohrModel::toggleAutoTransition() {
    autoTransition = !autoTransition;
}

void BohrModel::setElementConfiguration(int protonCount, int neutronCount) {
    atomicNumber = protonCount;
    neutronNumber = neutronCount;
    reset();
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
    return 0.529 * n * n;
}

double BohrModel::calculateOrbitalSpeed(int n) const {
    return 2.188e6 / n;
}

double BohrModel::calculateLevelEnergyEV(int n) const {
    return -13.6 / (n * n);
}

void BohrModel::renderText(float x, float y, const char* text) const {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; ++c) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

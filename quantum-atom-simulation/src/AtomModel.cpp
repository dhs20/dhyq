#include "AtomModel.h"
#include <GL/glut.h>

AtomModel::AtomModel(int atomicNumber) {
    this->atomicNumber = atomicNumber;
    this->nucleusRadius = 0.1;
    this->electronRadius = 0.05;
}

void AtomModel::update(double dt) {
    animationController.update(dt);
    particleSystem.update(dt);
}

void AtomModel::reset() {
    animationController.reset();
    particleSystem.clear();
}

void AtomModel::play() {
    animationController.play();
}

void AtomModel::pause() {
    animationController.pause();
}

void AtomModel::setSpeed(double speed) {
    animationController.setSpeed(speed);
}

bool AtomModel::getIsPlaying() const {
    return animationController.getIsPlaying();
}

double AtomModel::getSpeed() const {
    return animationController.getSpeed();
}

int AtomModel::getAtomicNumber() const {
    return atomicNumber;
}

void AtomModel::setAtomicNumber(int atomicNumber) {
    if (atomicNumber > 0) {
        this->atomicNumber = atomicNumber;
        reset();
    }
}

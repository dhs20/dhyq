#include "AnimationController.h"

AnimationController::AnimationController() {
    time = 0.0;
    deltaTime = 0.0;
    isPlaying = true;
    speed = 1.0;
}

void AnimationController::update(double dt) {
    if (isPlaying) {
        deltaTime = dt * speed;
        time += deltaTime;
    } else {
        deltaTime = 0.0;
    }
}

void AnimationController::play() {
    isPlaying = true;
}

void AnimationController::pause() {
    isPlaying = false;
}

void AnimationController::reset() {
    time = 0.0;
    deltaTime = 0.0;
}

bool AnimationController::getIsPlaying() const {
    return isPlaying;
}

double AnimationController::getTime() const {
    return time;
}

double AnimationController::getDeltaTime() const {
    return deltaTime;
}

double AnimationController::getSpeed() const {
    return speed;
}

void AnimationController::setSpeed(double speed) {
    if (speed > 0) {
        this->speed = speed;
    }
}

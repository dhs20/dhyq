#ifndef ATOM_MODEL_H
#define ATOM_MODEL_H

#include "ParticleSystem.h"
#include "AnimationController.h"

class AtomModel {
protected:
    ParticleSystem particleSystem;
    AnimationController animationController;
    int atomicNumber;
    double nucleusRadius;
    double electronRadius;
    
public:
    AtomModel(int atomicNumber = 1);
    virtual ~AtomModel() = default;
    
    virtual void update(double dt);
    virtual void render() = 0;
    virtual void reset();
    
    void play();
    void pause();
    void setSpeed(double speed);
    bool getIsPlaying() const;
    double getSpeed() const;
    
    int getAtomicNumber() const;
    void setAtomicNumber(int atomicNumber);
};

#endif // ATOM_MODEL_H

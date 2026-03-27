#ifndef BOHR_MODEL_H
#define BOHR_MODEL_H

#include "AtomModel.h"

class BohrModel : public AtomModel {
private:
    struct Electron {
        int energyLevel;
        double angle;
        double radius;
        double speed;
    };
    
    std::vector<Electron> electrons;
    double nucleusX, nucleusY, nucleusZ;
    
public:
    BohrModel(int atomicNumber = 1);
    
    void update(double dt) override;
    void render() override;
    void reset() override;
    
private:
    void initializeElectrons();
    double calculateOrbitalRadius(int n) const;
    double calculateOrbitalSpeed(int n) const;
};

#endif // BOHR_MODEL_H

#ifndef BOHR_MODEL_H
#define BOHR_MODEL_H

#include "AtomModel.h"
#include <vector>

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

    int selectedElectronIndex;
    bool transitionActive;
    bool autoTransition;
    double transitionTimer;
    double transitionDuration;
    int transitionFrom;
    int transitionTo;
    double transitionEnergyEV;
    double photonFrequencyHz;
    double photonWavelengthNm;
    double entropyDelta;

public:
    BohrModel(int atomicNumber = 1, int neutronNumber = 0);

    void update(double dt) override;
    void render() override;
    void reset() override;

    void triggerTransition(bool upward);
    void toggleAutoTransition();
    void setElementConfiguration(int protonCount, int neutronCount);

private:
    void initializeElectrons();
    double calculateOrbitalRadius(int n) const;
    double calculateOrbitalSpeed(int n) const;
    double calculateLevelEnergyEV(int n) const;
    void renderText(float x, float y, const char* text) const;
};

#endif // BOHR_MODEL_H

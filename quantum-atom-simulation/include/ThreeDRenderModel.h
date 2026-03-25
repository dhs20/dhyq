#ifndef THREE_D_RENDER_MODEL_H
#define THREE_D_RENDER_MODEL_H

#include "AtomModel.h"
#include <vector>

class ThreeDRenderModel : public AtomModel {
private:
    int quantumNumbers[3]; // n, l, m
    int resolution;
    double cloudRadius;
    bool showOrbitals;
    double probabilityThreshold;
    std::vector<std::vector<int>> quantumStatePresets;
    int activePresetIndex;

public:
    ThreeDRenderModel(int atomicNumber = 1, int neutronNumber = 0);

    void update(double dt) override;
    void render() override;
    void reset() override;

    void setQuantumNumbers(int n, int l, int m);
    void setShowOrbitals(bool show);
    void setProbabilityThreshold(double threshold);
    void nextQuantumPreset();

private:
    void generateProbabilityCloud();
    void renderNucleus();
    void renderOrbitals();
    void renderInfoPanel();
};

#endif // THREE_D_RENDER_MODEL_H

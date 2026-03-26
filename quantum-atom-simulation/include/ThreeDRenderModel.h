#ifndef THREE_D_RENDER_MODEL_H
#define THREE_D_RENDER_MODEL_H

#include "AtomModel.h"

class ThreeDRenderModel : public AtomModel {
private:
    int quantumNumbers[3]; // n, l, m
    int resolution;
    double cloudRadius;
    bool showOrbitals;
    
public:
    ThreeDRenderModel(int atomicNumber = 1);
    
    void update(double dt) override;
    void render() override;
    void reset() override;
    
    void setQuantumNumbers(int n, int l, int m);
    void setShowOrbitals(bool show);
    
private:
    void generateProbabilityCloud();
    void renderNucleus();
    void renderOrbitals();
};

#endif // THREE_D_RENDER_MODEL_H

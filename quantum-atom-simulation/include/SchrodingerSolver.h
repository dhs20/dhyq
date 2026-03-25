#ifndef SCHRODINGER_SOLVER_H
#define SCHRODINGER_SOLVER_H

#include <vector>

struct OrbitalPoint {
    double r;
    double radialProbability;
    double theta;
    double angularProbability;
};

class SchrodingerSolver {
public:
    static std::vector<OrbitalPoint> sampleOrbitalDistribution(int n, int l, int m, int radialSamples, int angularSamples);
    static std::vector<std::vector<int>> supportedQuantumStates();
};

#endif // SCHRODINGER_SOLVER_H

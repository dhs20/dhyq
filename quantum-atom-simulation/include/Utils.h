#ifndef UTILS_H
#define UTILS_H

#include <cmath>
#include <vector>

class Utils {
public:
    // Mathematical constants
    static const double PI;
    static const double E;
    static const double H_BAR;
    static const double ELECTRON_CHARGE;
    static const double ELECTRON_MASS;
    static const double BOHR_RADIUS;

    // Mathematics helpers
    static double factorial(int n);
    static double combination(int n, int k);
    static double laguerre(int n, int l, double x);
    static double associatedLegendre(int l, int m, double theta);

    // Wave functions
    static double radialWaveFunction(int n, int l, double r);
    static double angularWaveFunction(int l, int m, double theta, double phi);
    static double probabilityDensity(int n, int l, int m, double r, double theta, double phi);

    // Physical quantities
    static double energyLevel(int n);
    static double entropyChange(double heatJoule, double temperatureKelvin);
    static double deBroglieWavelength(double mass, double velocity);

    // Utilities
    static double random(double min, double max);
    static std::vector<double> sphericalToCartesian(double r, double theta, double phi);
};

#endif // UTILS_H

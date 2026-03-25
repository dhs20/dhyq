#include "Utils.h"
#include <cstdlib>

// Mathematical constants definition
const double Utils::PI = 3.14159265358979323846;
const double Utils::E = 2.71828182845904523536;
const double Utils::H_BAR = 1.054571817e-34;
const double Utils::ELECTRON_CHARGE = 1.602176634e-19;
const double Utils::ELECTRON_MASS = 9.1093837015e-31;
const double Utils::BOHR_RADIUS = 5.29177210903e-11;

// Factorial calculation
double Utils::factorial(int n) {
    if (n <= 0) return 1;
    double result = 1;
    for (int i = 1; i <= n; ++i) {
        result *= i;
    }
    return result;
}

// Combination calculation
double Utils::combination(int n, int k) {
    if (k < 0 || k > n) return 0;
    return factorial(n) / (factorial(k) * factorial(n - k));
}

// Laguerre polynomial calculation
double Utils::laguerre(int n, int l, double x) {
    double result = 0;
    for (int k = 0; k <= n - l - 1; ++k) {
        double term = pow(-1, k) * combination(n + l, n - l - 1 - k) * pow(x, k) / factorial(k);
        result += term;
    }
    return result;
}

// Associated Legendre polynomial calculation
double Utils::associatedLegendre(int l, int m, double theta) {
    double cosTheta = cos(theta);
    double sinTheta = sin(theta);
    
    // Base cases
    if (l == 0 && m == 0) return 1.0;
    if (l == 1) {
        if (m == 0) return cosTheta;
        if (m == 1) return -sinTheta;
        if (m == -1) return sinTheta / 2.0;
    }
    
    // Recursive calculation
    if (l == m) {
        double sign = (m % 2 == 0) ? 1.0 : -1.0;
        return sign * pow(2, l) * factorial(l) * pow(sinTheta, l);
    }
    
    if (l == m + 1) {
        return (2 * l - 1) * cosTheta * associatedLegendre(l - 1, m, theta);
    }
    
    return ((2 * l - 1) * cosTheta * associatedLegendre(l - 1, m, theta) - (l + m - 1) * associatedLegendre(l - 2, m, theta)) / (l - m);
}

// Radial wave function calculation
double Utils::radialWaveFunction(int n, int l, double r) {
    double rho = 2 * r / (n * BOHR_RADIUS);
    double normalization = sqrt(pow(2.0 / (n * BOHR_RADIUS), 3) * factorial(n - l - 1) / (2 * n * factorial(n + l)));
    double exponential = exp(-rho / 2);
    double polynomial = pow(rho, l) * laguerre(n, l, rho);
    
    return normalization * exponential * polynomial;
}

// Angular wave function calculation
double Utils::angularWaveFunction(int l, int m, double theta, double phi) {
    double normalization = sqrt((2 * l + 1) * factorial(l - abs(m)) / (4 * PI * factorial(l + abs(m))));
    double legendre = associatedLegendre(l, abs(m), theta);
    double exponential = cos(m * phi);
    
    if (m < 0) {
        exponential = sin(-m * phi);
    }
    
    return normalization * legendre * exponential;
}

// Probability density calculation
double Utils::probabilityDensity(int n, int l, int m, double r, double theta, double phi) {
    double radial = radialWaveFunction(n, l, r);
    double angular = angularWaveFunction(l, m, theta, phi);
    
    return radial * radial * angular * angular * r * r * sin(theta);
}

// Energy level calculation
double Utils::energyLevel(int n) {
    return -13.6 / (n * n); // Unit: electron volts
}

// Random number generation
double Utils::random(double min, double max) {
    return min + (max - min) * rand() / RAND_MAX;
}

// Spherical to Cartesian coordinate conversion
std::vector<double> Utils::sphericalToCartesian(double r, double theta, double phi) {
    std::vector<double> cartesian(3);
    cartesian[0] = r * sin(theta) * cos(phi);
    cartesian[1] = r * sin(theta) * sin(phi);
    cartesian[2] = r * cos(theta);
    return cartesian;
}

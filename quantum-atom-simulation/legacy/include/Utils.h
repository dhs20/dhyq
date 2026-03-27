#ifndef UTILS_H
#define UTILS_H

#include <cmath>
#include <vector>

class Utils {
public:
    // 数学常数
    static const double PI;
    static const double E;
    static const double H_BAR;
    static const double ELECTRON_CHARGE;
    static const double ELECTRON_MASS;
    static const double BOHR_RADIUS;
    
    // 数学函数
    static double factorial(int n);
    static double combination(int n, int k);
    static double laguerre(int n, int l, double x);
    static double associatedLegendre(int l, int m, double theta);
    
    // 波函数计算
    static double radialWaveFunction(int n, int l, double r);
    static double angularWaveFunction(int l, int m, double theta, double phi);
    static double probabilityDensity(int n, int l, int m, double r, double theta, double phi);
    
    // 能量计算
    static double energyLevel(int n);
    
    // 随机数生成
    static double random(double min, double max);
    static std::vector<double> sphericalToCartesian(double r, double theta, double phi);
};

#endif // UTILS_H

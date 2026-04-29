#include "quantum/basis/BasisMetadata.h"

#include <algorithm>
#include <array>
#include <string>

namespace quantum::basis {
namespace {

constexpr std::array<OrbitalDescriptor, 16> kAufbauOrder = {{
    {1, 0, "1s", 2},
    {2, 0, "2s", 2},
    {2, 1, "2p", 6},
    {3, 0, "3s", 2},
    {3, 1, "3p", 6},
    {4, 0, "4s", 2},
    {3, 2, "3d", 10},
    {4, 1, "4p", 6},
    {5, 0, "5s", 2},
    {4, 2, "4d", 10},
    {5, 1, "5p", 6},
    {6, 0, "6s", 2},
    {4, 3, "4f", 14},
    {5, 2, "5d", 10},
    {6, 1, "6p", 6},
    {7, 0, "7s", 2},
}};

} // namespace

std::string_view angularMomentumLabel(int l) {
    switch (l) {
    case 0:
        return "s";
    case 1:
        return "p";
    case 2:
        return "d";
    case 3:
        return "f";
    case 4:
        return "g";
    default:
        return "l";
    }
}

int subshellCapacity(int l) {
    return std::max(0, 2 * (2 * l + 1));
}

std::vector<OrbitalDescriptor> aufbauOrder(int maxPrincipalNumber) {
    const int maxN = std::clamp(maxPrincipalNumber, 1, 7);
    std::vector<OrbitalDescriptor> result;
    for (const auto& orbital : kAufbauOrder) {
        if (orbital.n <= maxN) {
            result.push_back(orbital);
        }
    }
    return result;
}

} // namespace quantum::basis

#pragma once

#include <string_view>
#include <vector>

namespace quantum::basis {

struct OrbitalDescriptor {
    int n = 1;
    int l = 0;
    std::string_view label;
    int capacity = 2;
};

[[nodiscard]] std::string_view angularMomentumLabel(int l);
[[nodiscard]] int subshellCapacity(int l);
[[nodiscard]] std::vector<OrbitalDescriptor> aufbauOrder(int maxPrincipalNumber = 7);

} // namespace quantum::basis

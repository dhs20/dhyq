#pragma once

#include "quantum/solvers/MeanFieldSolver.h"

namespace quantum::solvers {

class TeachingMeanFieldSolver final : public MeanFieldSolver {
public:
    [[nodiscard]] std::string_view name() const override;
    [[nodiscard]] MeanFieldSolveResult solve(const MeanFieldSolveRequest& request) override;
};

[[nodiscard]] const char* meanFieldModeLabel(MeanFieldMode mode);

} // namespace quantum::solvers

#pragma once

#include "quantum/app/SimulationState.h"

#include <optional>
#include <vector>

namespace quantum::app {

struct ParamSnapshot {
    int atomicNumber = 1;
    int chargeState = 0;
    int principalN = 2;
    int maxSpectrumN = 6;
    double solverQnN = 2.0;
    double solverQnL = 1.0;
    double solverQnM = 0.0;
    ModelKind modelKind = ModelKind::Bohr;
    quantum::physics::ApproximationMode approximationMode =
        quantum::physics::ApproximationMode::StrictHydrogenic;
    CloudRenderMode cloudRenderMode = CloudRenderMode::Hybrid;
    bool showOrbitRings = true;
    bool showGrid = true;
    float pointSize = 3.0f;
    float exposure = 1.0f;
    double fitTransitionSeconds = 0.65;

    static ParamSnapshot fromState(const SimulationState& state);
    void applyTo(SimulationState& state) const;
};

class UndoStack {
public:
    void push(const ParamSnapshot& before);
    std::optional<ParamSnapshot> undo(const ParamSnapshot& current);
    std::optional<ParamSnapshot> redo();
    void clear();

private:
    std::vector<ParamSnapshot> snapshots_;
    int position_ = -1;
    static constexpr int kMaxDepth = 50;
};

} // namespace quantum::app
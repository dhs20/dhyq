#include "quantum/app/UndoStack.h"

namespace quantum::app {

ParamSnapshot ParamSnapshot::fromState(const SimulationState& state) {
    ParamSnapshot snap;
    snap.atomicNumber = state.atomicNumber;
    snap.chargeState = state.chargeState;
    snap.principalN = state.bohr.principalN;
    snap.maxSpectrumN = state.bohr.maxSpectrumN;
    snap.solverQnN = static_cast<double>(state.solver.qn.n);
    snap.solverQnL = static_cast<double>(state.solver.qn.l);
    snap.solverQnM = static_cast<double>(state.solver.qn.m);
    snap.modelKind = state.modelKind;
    snap.approximationMode = state.approximationMode;
    snap.cloudRenderMode = state.view.cloudRenderMode;
    snap.showOrbitRings = state.view.showOrbitRings;
    snap.showGrid = state.view.showGrid;
    snap.pointSize = state.view.pointSize;
    snap.exposure = state.view.exposure;
    snap.fitTransitionSeconds = state.view.fitTransitionSeconds;
    return snap;
}

void ParamSnapshot::applyTo(SimulationState& state) const {
    state.atomicNumber = atomicNumber;
    state.chargeState = chargeState;
    state.bohr.principalN = principalN;
    state.bohr.maxSpectrumN = maxSpectrumN;
    state.solver.qn.n = static_cast<int>(solverQnN);
    state.solver.qn.l = static_cast<int>(solverQnL);
    state.solver.qn.m = static_cast<int>(solverQnM);
    state.modelKind = modelKind;
    state.approximationMode = approximationMode;
    state.view.cloudRenderMode = cloudRenderMode;
    state.view.showOrbitRings = showOrbitRings;
    state.view.showGrid = showGrid;
    state.view.pointSize = pointSize;
    state.view.exposure = exposure;
    state.view.fitTransitionSeconds = fitTransitionSeconds;
    state.dirty.physics = true;
    state.dirty.cloud = true;
    state.dirty.solver = true;
    state.dirty.reference = true;
}

void UndoStack::push(const ParamSnapshot& before) {
    if (snapshots_.size() > static_cast<std::size_t>(kMaxDepth)) {
        snapshots_.erase(snapshots_.begin());
        --position_;
    }
    ++position_;
    if (static_cast<int>(snapshots_.size()) > position_) {
        snapshots_.erase(snapshots_.begin() + position_ + 1, snapshots_.end());
    }
    snapshots_.insert(snapshots_.begin() + position_, before);
    if (static_cast<int>(snapshots_.size()) > kMaxDepth) {
        snapshots_.erase(snapshots_.begin());
        --position_;
    }
}

std::optional<ParamSnapshot> UndoStack::undo(const ParamSnapshot& current) {
    if (position_ < 0) return std::nullopt;
    ParamSnapshot result = snapshots_[static_cast<std::size_t>(position_)];
    snapshots_[static_cast<std::size_t>(position_)] = current;
    --position_;
    return result;
}

std::optional<ParamSnapshot> UndoStack::redo() {
    if (position_ + 1 >= static_cast<int>(snapshots_.size())) return std::nullopt;
    ++position_;
    return snapshots_[static_cast<std::size_t>(position_)];
}

void UndoStack::clear() {
    snapshots_.clear();
    position_ = -1;
}

} // namespace quantum::app
#include "quantum/app/SessionPersistence.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace quantum::app {

static std::string isoTimeNow() {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &timeT);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H%M%S");
    return oss.str();
}

SessionRecord SessionRecord::fromState(const SimulationState& state) {
    SessionRecord rec;
    rec.timestamp = isoTimeNow();
    rec.atomicNumber = state.atomicNumber;
    rec.chargeState = state.chargeState;
    rec.principalN = state.bohr.principalN;
    rec.maxSpectrumN = state.bohr.maxSpectrumN;
    rec.solverQnN = static_cast<double>(state.solver.qn.n);
    rec.solverQnL = static_cast<double>(state.solver.qn.l);
    rec.solverQnM = static_cast<double>(state.solver.qn.m);
    rec.modelKindInt = static_cast<int>(state.modelKind);
    rec.approximationModeInt = static_cast<int>(state.approximationMode);
    rec.cloudRenderModeInt = static_cast<int>(state.view.cloudRenderMode);
    rec.showOrbitRings = state.view.showOrbitRings;
    rec.showGrid = state.view.showGrid;
    rec.pointSize = state.view.pointSize;
    rec.exposure = state.view.exposure;
    rec.fitTransitionSeconds = state.view.fitTransitionSeconds;
    rec.learningEnabled = state.learning.enabled;
    rec.learningPathInt = static_cast<int>(state.learning.path);
    rec.learningStepIndex = state.learning.stepIndex;
    rec.meanFieldEnabled = state.meanField.enabled;
    rec.meanFieldUseMixing = state.meanField.useMixing;
    rec.meanFieldMixingAlpha = state.meanField.mixingAlpha;
    return rec;
}

void SessionRecord::applyTo(SimulationState& state) const {
    state.atomicNumber = atomicNumber;
    state.chargeState = chargeState;
    state.bohr.principalN = principalN;
    state.bohr.maxSpectrumN = maxSpectrumN;
    state.solver.qn.n = static_cast<int>(solverQnN);
    state.solver.qn.l = static_cast<int>(solverQnL);
    state.solver.qn.m = static_cast<int>(solverQnM);
    state.modelKind = static_cast<ModelKind>(modelKindInt);
    state.approximationMode = static_cast<quantum::physics::ApproximationMode>(approximationModeInt);
    state.view.cloudRenderMode = static_cast<CloudRenderMode>(cloudRenderModeInt);
    state.view.showOrbitRings = showOrbitRings;
    state.view.showGrid = showGrid;
    state.view.pointSize = pointSize;
    state.view.exposure = exposure;
    state.view.fitTransitionSeconds = fitTransitionSeconds;
    state.learning.enabled = learningEnabled;
    state.learning.path = static_cast<LearningPath>(learningPathInt);
    state.learning.stepIndex = learningStepIndex;
    state.meanField.enabled = meanFieldEnabled;
    state.meanField.useMixing = meanFieldUseMixing;
    state.meanField.mixingAlpha = meanFieldMixingAlpha;
    state.dirty.physics = true;
    state.dirty.cloud = true;
    state.dirty.solver = true;
    state.dirty.reference = true;
}

bool saveSession(const std::filesystem::path& path, const SessionRecord& session) {
    nlohmann::json j;
    j["timestamp"] = session.timestamp;
    j["atomicNumber"] = session.atomicNumber;
    j["chargeState"] = session.chargeState;
    j["principalN"] = session.principalN;
    j["maxSpectrumN"] = session.maxSpectrumN;
    j["solverQnN"] = session.solverQnN;
    j["solverQnL"] = session.solverQnL;
    j["solverQnM"] = session.solverQnM;
    j["modelKind"] = session.modelKindInt;
    j["approximationMode"] = session.approximationModeInt;
    j["cloudRenderMode"] = session.cloudRenderModeInt;
    j["showOrbitRings"] = session.showOrbitRings;
    j["showGrid"] = session.showGrid;
    j["pointSize"] = session.pointSize;
    j["exposure"] = session.exposure;
    j["fitTransitionSeconds"] = session.fitTransitionSeconds;
    j["learningEnabled"] = session.learningEnabled;
    j["learningPath"] = session.learningPathInt;
    j["learningStepIndex"] = session.learningStepIndex;
    j["meanFieldEnabled"] = session.meanFieldEnabled;
    j["meanFieldUseMixing"] = session.meanFieldUseMixing;
    j["meanFieldMixingAlpha"] = session.meanFieldMixingAlpha;

    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << j.dump(2);
    return true;
}

std::optional<SessionRecord> loadSession(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;
    nlohmann::json j;
    try {
        file >> j;
    } catch (...) {
        return std::nullopt;
    }
    SessionRecord rec;
    rec.timestamp = j.value("timestamp", "");
    rec.atomicNumber = j.value("atomicNumber", 1);
    rec.chargeState = j.value("chargeState", 0);
    rec.principalN = j.value("principalN", 2);
    rec.maxSpectrumN = j.value("maxSpectrumN", 6);
    rec.solverQnN = j.value("solverQnN", 2.0);
    rec.solverQnL = j.value("solverQnL", 1.0);
    rec.solverQnM = j.value("solverQnM", 0.0);
    rec.modelKindInt = j.value("modelKind", 0);
    rec.approximationModeInt = j.value("approximationMode", 0);
    rec.cloudRenderModeInt = j.value("cloudRenderMode", 0);
    rec.showOrbitRings = j.value("showOrbitRings", true);
    rec.showGrid = j.value("showGrid", true);
    rec.pointSize = j.value("pointSize", 3.0f);
    rec.exposure = j.value("exposure", 1.0f);
    rec.fitTransitionSeconds = j.value("fitTransitionSeconds", 0.65);
    rec.learningEnabled = j.value("learningEnabled", false);
    rec.learningPathInt = j.value("learningPath", 0);
    rec.learningStepIndex = j.value("learningStepIndex", 0);
    rec.meanFieldEnabled = j.value("meanFieldEnabled", true);
    rec.meanFieldUseMixing = j.value("meanFieldUseMixing", true);
    rec.meanFieldMixingAlpha = j.value("meanFieldMixingAlpha", 0.3);
    return rec;
}

void writeRecoveryFile(const std::filesystem::path& path, const SessionRecord& session) {
    saveSession(path, session);
}

void deleteRecoveryFile(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

bool hasRecoveryFile(const std::filesystem::path& path) {
    return std::filesystem::exists(path);
}

} // namespace quantum::app
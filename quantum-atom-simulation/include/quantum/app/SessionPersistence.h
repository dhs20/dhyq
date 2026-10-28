#pragma once

#include "quantum/app/SimulationState.h"

#include <filesystem>
#include <optional>
#include <string>

namespace quantum::app {

struct SessionRecord {
    std::string timestamp;
    int atomicNumber = 1;
    int chargeState = 0;
    int principalN = 2;
    int maxSpectrumN = 6;
    double solverQnN = 2.0;
    double solverQnL = 1.0;
    double solverQnM = 0.0;
    int modelKindInt = 0;
    int approximationModeInt = 0;
    int cloudRenderModeInt = 0;
    bool showOrbitRings = true;
    bool showGrid = true;
    float pointSize = 3.0f;
    float exposure = 1.0f;
    double fitTransitionSeconds = 0.65;
    bool learningEnabled = false;
    int learningPathInt = 0;
    int learningStepIndex = 0;
    bool meanFieldEnabled = true;
    bool meanFieldUseMixing = true;
    double meanFieldMixingAlpha = 0.3;

    static SessionRecord fromState(const SimulationState& state);
    void applyTo(SimulationState& state) const;
};

bool saveSession(const std::filesystem::path& path, const SessionRecord& session);
std::optional<SessionRecord> loadSession(const std::filesystem::path& path);
void writeRecoveryFile(const std::filesystem::path& path, const SessionRecord& session);
void deleteRecoveryFile(const std::filesystem::path& path);
bool hasRecoveryFile(const std::filesystem::path& path);

} // namespace quantum::app
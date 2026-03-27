#pragma once

#include "quantum/app/SimulationState.h"
#include "quantum/core/Logger.h"
#include "quantum/core/PerformanceTracker.h"
#include "quantum/physics/ElementDatabase.h"

#include <GL/glew.h>
#include <glm/vec2.hpp>

namespace quantum::ui {

struct UiFrameResult {
    glm::vec2 sceneSize{1280.0f, 720.0f};
    bool sceneHovered = false;
    bool sceneActive = false;
    bool sceneFocused = false;
};

class AppUi {
public:
    UiFrameResult draw(quantum::app::SimulationState& state,
                       const quantum::physics::ElementDatabase& elementDatabase,
                       const quantum::core::PerformanceSnapshot& performance,
                       const quantum::core::Logger& logger,
                       GLuint sceneTexture,
                       const glm::ivec2& framebufferSize) const;

private:
    mutable bool layoutInitialized_ = false;
    mutable int energySelectionIndex_ = -1;
    mutable int spectrumSelectionIndex_ = -1;
    mutable bool spectrumSelectionReference_ = false;
    mutable int radialSelectionIndex_ = -1;
    mutable int convergenceSelectionIndex_ = -1;
};

} // namespace quantum::ui

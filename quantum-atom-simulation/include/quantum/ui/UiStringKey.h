#pragma once

#include <cstdint>

namespace quantum::ui {

enum class UiStringKey : uint16_t {
    Panel_Inspector,
    Panel_Charts,
    Panel_Help,
    Panel_Physics,
    Panel_Performance,
    Panel_Log,

    Group_ElementAndMode,
    Group_CloudAndAppearance,
    Group_TransitionAndSpectroscopy,
    Group_AdvancedPhysics,
    Group_ScenePresets,

    SceneBrowser_All,
    SceneBrowser_Hydrogen,
    SceneBrowser_MultiElectron,
    SceneBrowser_Spectroscopy,
    SceneBrowser_Nuclear,

    Learning_Off,
    Learning_AtomicStructure,
    Learning_Spectroscopy,
    Learning_MultiElectron,
    Learning_NuclearPhysics,
    Learning_StepCount,
    Learning_PreviousStep,
    Learning_NextStep,
    Learning_Mode,

    Hint_CtrlS_Screenshot,
    Hint_CtrlZ_Undo,
    Hint_ResetView,
    Hint_F_Fit,
    Hint_ExportReport,
    Hint_ExportCSV,

    Status_Preview,
    Status_BuildingCloud,
    Status_RecoveryTitle,
    Status_RecoveryPrompt,
    Status_Recover,
    Status_Discard,

    Action_ExportReport,
    Action_ExportCSV,

    COUNT
};

const char* uiStr(UiStringKey key);

} // namespace quantum::ui
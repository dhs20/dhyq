#include "quantum/ui/UiStringKey.h"

namespace quantum::ui {

const char* uiStr(UiStringKey key) {
    switch (key) {
    case UiStringKey::Panel_Inspector: return "控制面板";
    case UiStringKey::Panel_Charts: return "图表";
    case UiStringKey::Panel_Help: return "帮助";
    case UiStringKey::Panel_Physics: return "物理信息";
    case UiStringKey::Panel_Performance: return "性能监视";
    case UiStringKey::Panel_Log: return "日志";

    case UiStringKey::Group_ElementAndMode: return "元素与模式";
    case UiStringKey::Group_CloudAndAppearance: return "云与外观";
    case UiStringKey::Group_TransitionAndSpectroscopy: return "跃迁与光谱";
    case UiStringKey::Group_AdvancedPhysics: return "高阶物理";
    case UiStringKey::Group_ScenePresets: return "场景预设";

    case UiStringKey::SceneBrowser_All: return "全部";
    case UiStringKey::SceneBrowser_Hydrogen: return "氢原子";
    case UiStringKey::SceneBrowser_MultiElectron: return "多电子";
    case UiStringKey::SceneBrowser_Spectroscopy: return "光谱";
    case UiStringKey::SceneBrowser_Nuclear: return "核过程";

    case UiStringKey::Learning_Off: return "关闭";
    case UiStringKey::Learning_AtomicStructure: return "原子结构入门";
    case UiStringKey::Learning_Spectroscopy: return "光谱探秘";
    case UiStringKey::Learning_MultiElectron: return "多电子原子";
    case UiStringKey::Learning_NuclearPhysics: return "核物理入门";
    case UiStringKey::Learning_StepCount: return "步骤 %d/%d";
    case UiStringKey::Learning_PreviousStep: return "上一步";
    case UiStringKey::Learning_NextStep: return "下一步";
    case UiStringKey::Learning_Mode: return "学习模式";

    case UiStringKey::Hint_CtrlS_Screenshot: return "截图: 使用 Ctrl+S 快捷键保存当前视图为 PPM 文件";
    case UiStringKey::Hint_CtrlZ_Undo: return "撤销: Ctrl+Z  重做: Ctrl+Y";
    case UiStringKey::Hint_ResetView: return "双击右键重置视图 | 右键菜单";
    case UiStringKey::Hint_F_Fit: return "F 键快速适配全部";
    case UiStringKey::Hint_ExportReport: return "导出当前验证报告";
    case UiStringKey::Hint_ExportCSV: return "导出当前窗口 CSV";

    case UiStringKey::Status_Preview: return "生成预览...";
    case UiStringKey::Status_BuildingCloud: return "生成完整云...";
    case UiStringKey::Status_RecoveryTitle: return "检测到未保存的会话";
    case UiStringKey::Status_RecoveryPrompt: return "上次运行在 %s 中断。是否恢复上次的仿真参数？";
    case UiStringKey::Status_Recover: return "恢复";
    case UiStringKey::Status_Discard: return "放弃，从头开始";

    case UiStringKey::Action_ExportReport: return "导出验证报告";
    case UiStringKey::Action_ExportCSV: return "导出当前窗口 CSV";
    case UiStringKey::COUNT:
    default:
        return "";
    }
}

} // namespace quantum::ui

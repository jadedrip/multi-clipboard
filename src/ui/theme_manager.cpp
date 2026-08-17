#include "theme_manager.h"

namespace {

// ============================================================
// 亮色主题颜色
// ============================================================
ThemeColors buildLightTheme()
{
    ThemeColors t;
    t["window_bg"] = "#f6f8fa";
    t["toolbar_bg"] = "#f6f8fa";
    t["toolbar_border"] = "#d0d7de";
    t["content_bg"] = "#f0f2f5";
    t["status_bar_bg"] = "#f6f8fa";
    t["status_bar_border"] = "#d0d7de";
    t["status_bar_text"] = "#656d76";

    t["search_bg"] = "#ffffff";
    t["search_border"] = "#d0d7de";
    t["search_border_focus"] = "#0969da";
    t["search_text"] = "#24292f";
    t["search_placeholder"] = "#909399";

    t["button_bg"] = "#ffffff";
    t["button_border"] = "#d0d7de";
    t["button_text"] = "#24292f";
    t["button_hover_bg"] = "#f0f4ff";
    t["button_hover_border"] = "#90b8f8";
    t["button_checked_bg"] = "#0969da";
    t["button_checked_text"] = "#ffffff";
    t["button_checked_border"] = "#0969da";

    t["card_normal_bg"] = "#ffffff";
    t["card_normal_border"] = "#909399";
    t["card_normal_hover_bg"] = "#f5f7fa";
    t["card_normal_hover_border"] = "#606266";
    t["card_normal_index"] = "#606266";
    t["card_normal_content"] = "#303133";

    t["card_used_bg"] = "#f0f9eb";
    t["card_used_border"] = "#67c23a";
    t["card_used_hover_bg"] = "#e8f5e1";
    t["card_used_hover_border"] = "#529b2e";
    t["card_used_index"] = "#808080";
    t["card_used_content"] = "#606266";
    t["card_used_check"] = "#67c23a";

    t["card_raw_bg"] = "#f9f0ff";
    t["card_raw_border"] = "#9254de";
    t["card_raw_hover_bg"] = "#f3d9fa";
    t["card_raw_hover_border"] = "#722ed1";
    t["card_raw_index"] = "#9254de";
    t["card_raw_content"] = "#531dab";
    t["card_raw_check"] = "#9254de";

    t["checkbox_bg"] = "#ffffff";
    t["checkbox_border"] = "#909399";
    t["checkbox_hover_border"] = "#4096ff";
    t["checkbox_hover_bg"] = "#ecf5ff";
    t["checkbox_checked_bg"] = "#4096ff";
    t["checkbox_checked_border"] = "#4096ff";

    t["scrollbar_bg"] = "transparent";
    t["scrollbar_handle"] = "#c0c4cc";
    t["scrollbar_handle_hover"] = "#909399";

    t["flash_bg"] = "#d6e8ff";
    t["flash_border"] = "#3b82f6";
    return t;
}

// ============================================================
// 暗色主题颜色
// ============================================================
ThemeColors buildDarkTheme()
{
    ThemeColors t;
    t["window_bg"] = "#0d1117";
    t["toolbar_bg"] = "#161b22";
    t["toolbar_border"] = "#30363d";
    t["content_bg"] = "#0d1117";
    t["status_bar_bg"] = "#161b22";
    t["status_bar_border"] = "#30363d";
    t["status_bar_text"] = "#8b949e";

    t["search_bg"] = "#0d1117";
    t["search_border"] = "#30363d";
    t["search_border_focus"] = "#58a6ff";
    t["search_text"] = "#c9d1d9";
    t["search_placeholder"] = "#484f58";

    t["button_bg"] = "#21262d";
    t["button_border"] = "#30363d";
    t["button_text"] = "#8b949e";
    t["button_hover_bg"] = "#30363d";
    t["button_hover_border"] = "#8b949e";
    t["button_checked_bg"] = "#1f6feb";
    t["button_checked_text"] = "#ffffff";
    t["button_checked_border"] = "#1f6feb";

    t["card_normal_bg"] = "#161b22";
    t["card_normal_border"] = "#30363d";
    t["card_normal_hover_bg"] = "#21262d";
    t["card_normal_hover_border"] = "#8b949e";
    t["card_normal_index"] = "#8b949e";
    t["card_normal_content"] = "#c9d1d9";

    t["card_used_bg"] = "#0e4429";
    t["card_used_border"] = "#238636";
    t["card_used_hover_bg"] = "#0d3d24";
    t["card_used_hover_border"] = "#2ea043";
    t["card_used_index"] = "#484f58";
    t["card_used_content"] = "#8b949e";
    t["card_used_check"] = "#238636";

    t["card_raw_bg"] = "#2d1f47";
    t["card_raw_border"] = "#8957e5";
    t["card_raw_hover_bg"] = "#3d2a5a";
    t["card_raw_hover_border"] = "#a371f7";
    t["card_raw_index"] = "#8957e5";
    t["card_raw_content"] = "#d2a8ff";
    t["card_raw_check"] = "#8957e5";

    t["checkbox_bg"] = "#21262d";
    t["checkbox_border"] = "#30363d";
    t["checkbox_hover_border"] = "#58a6ff";
    t["checkbox_hover_bg"] = "#1c3d5c";
    t["checkbox_checked_bg"] = "#58a6ff";
    t["checkbox_checked_border"] = "#58a6ff";

    t["scrollbar_bg"] = "transparent";
    t["scrollbar_handle"] = "#30363d";
    t["scrollbar_handle_hover"] = "#484f58";

    t["flash_bg"] = "#162332";
    t["flash_border"] = "#58a6ff";
    return t;
}

// ============================================================
// 配置窗口亮色主题
// ============================================================
ThemeColors buildLightConfigTheme()
{
    ThemeColors t;
    t["dialog_bg"] = "#f6f8fa";
    t["group_box_bg"] = "#ffffff";
    t["group_box_border"] = "#d0d7de";
    t["group_box_title"] = "#24292f";
    t["table_bg"] = "#ffffff";
    t["table_border"] = "#d0d7de";
    t["table_grid"] = "#e8eaed";
    t["table_alternate"] = "#f8f9fb";
    t["header_bg"] = "#f0f2f5";
    t["header_text"] = "#57606a";
    t["header_border"] = "#d0d7de";
    t["button_primary_bg"] = "#0969da";
    t["button_primary_text"] = "#ffffff";
    t["button_primary_hover"] = "#0860ca";
    t["button_primary_pressed"] = "#0751b0";
    t["button_secondary_bg"] = "#f6f8fa";
    t["button_secondary_border"] = "#d0d7de";
    t["button_secondary_text"] = "#24292f";
    t["button_secondary_hover"] = "#f0f2f5";
    t["button_secondary_pressed"] = "#e4e7eb";
    t["edit_bg"] = "#f6f8fa";
    t["edit_border"] = "#d0d7de";
    t["edit_text"] = "#24292f";
    t["edit_focus_bg"] = "#ffffff";
    t["edit_focus_border"] = "#0969da";
    t["hint_text"] = "#656d76";
    return t;
}

// ============================================================
// 配置窗口暗色主题
// ============================================================
ThemeColors buildDarkConfigTheme()
{
    ThemeColors t;
    t["dialog_bg"] = "#161b22";
    t["group_box_bg"] = "#21262d";
    t["group_box_border"] = "#30363d";
    t["group_box_title"] = "#c9d1d9";
    t["table_bg"] = "#21262d";
    t["table_border"] = "#30363d";
    t["table_grid"] = "#30363d";
    t["table_alternate"] = "#161b22";
    t["header_bg"] = "#21262d";
    t["header_text"] = "#8b949e";
    t["header_border"] = "#30363d";
    t["button_primary_bg"] = "#1f6feb";
    t["button_primary_text"] = "#ffffff";
    t["button_primary_hover"] = "#388bfd";
    t["button_primary_pressed"] = "#1c64f2";
    t["button_secondary_bg"] = "#21262d";
    t["button_secondary_border"] = "#30363d";
    t["button_secondary_text"] = "#c9d1d9";
    t["button_secondary_hover"] = "#30363d";
    t["button_secondary_pressed"] = "#3d444d";
    t["edit_bg"] = "#21262d";
    t["edit_border"] = "#30363d";
    t["edit_text"] = "#c9d1d9";
    t["edit_focus_bg"] = "#161b22";
    t["edit_focus_border"] = "#58a6ff";
    t["hint_text"] = "#8b949e";
    return t;
}

} // namespace

// ============================================================
// 主题获取
// ============================================================

namespace ThemeManager {

ThemeColors getTheme(const QString& themeName)
{
    static const ThemeColors light = buildLightTheme();
    static const ThemeColors dark = buildDarkTheme();
    return themeName == "dark" ? dark : light;
}

ThemeColors getConfigTheme(const QString& themeName)
{
    static const ThemeColors lightConfig = buildLightConfigTheme();
    static const ThemeColors darkConfig = buildDarkConfigTheme();
    return themeName == "dark" ? darkConfig : lightConfig;
}

} // namespace ThemeManager

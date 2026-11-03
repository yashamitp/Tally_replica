#pragma once

#include <QString>

namespace tally::ui {

class TallyTheme {
public:
    static QString getApplicationStyleSheet();

    // Color definitions
    static constexpr const char* COLOR_BG_APP = "#EDF4FA";
    static constexpr const char* COLOR_PRIMARY = "#1B4977";
    static constexpr const char* COLOR_PRIMARY_DARK = "#123252";
    static constexpr const char* COLOR_ACCENT_YELLOW = "#F6C343";
    static constexpr const char* COLOR_ACCENT_YELLOW_HOVER = "#FFE082";
    static constexpr const char* COLOR_TEXT_MAIN = "#112233";
    static constexpr const char* COLOR_TEXT_MUTED = "#556B82";
    static constexpr const char* COLOR_BORDER = "#B8D0E5";
    static constexpr const char* COLOR_PANEL_BG = "#FFFFFF";
    static constexpr const char* COLOR_GATEWAY_HEADER = "#235B8C";
};

} // namespace tally::ui

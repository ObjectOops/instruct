#ifndef INSTRUCT_CONSTANTS_H
#define INSTRUCT_CONSTANTS_H

#include <filesystem>
#include <string>
#include <chrono>

#include "ftxui/component/component_options.hpp"

namespace instruct::constants {
    #ifdef __INSTRUCT_VERSION__
    const std::string INSTRUCT_VERSION = #__INSTRUCT_VERSION__;
    #else
    const std::string INSTRUCT_VERSION = "Unknown";
    #endif
    
    inline const std::filesystem::path DATA_DIR {"instruct_data"};
    inline const std::filesystem::path LOG_DIR {"logs"};
    
    inline const std::filesystem::path INSTRUCT_LOG_DIR {LOG_DIR / "instruct.log"};

    inline const std::filesystem::path INSTRUCTOR_CONFIG {DATA_DIR / "instructor_config.yaml"};
    inline const std::filesystem::path STUDENTS_CONFIG {DATA_DIR / "students_config.yaml"};
    inline const std::filesystem::path TESTS_CONFIG {DATA_DIR / "tests_config.yaml"};
    inline const std::filesystem::path UI_CONFIG {DATA_DIR / "ui_config.yaml"};

    inline constexpr int MAX_INSTRUCTOR_PASSWORD_LENGTH = 16;
    
    inline const std::string OPENVSCODE_SERVER_VERSION_DEFAULT {"v1.79.2"};
    
    inline const auto INPUT_TRANSFORM_CUSTOM {[] (ftxui::InputState state) {
        state.element |= ftxui::color(ftxui::Color::White);
        state.element |= ftxui::hcenter;
        if (state.is_placeholder) {
            state.element |= ftxui::dim;
        }
        if (!state.focused && state.hovered) {
            state.element |= ftxui::bgcolor(ftxui::Color::GrayDark);
        }
        return state.element;
    }};
    
    inline const int ASYNC_DISPLAY_SPINNER_TYPE {15};
    inline const std::chrono::milliseconds ASYNC_DISPLAY_SPINNER_INTERVAL {64};
}

#endif

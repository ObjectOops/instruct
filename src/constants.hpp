#ifndef INSTRUCT_CONSTANTS_H
#define INSTRUCT_CONSTANTS_H

#include <filesystem>
#include <string>

#include "ftxui/component/component_options.hpp"

namespace instruct::constants {
    inline const std::filesystem::path DATA_DIR {"instruct_data"};
    inline const std::filesystem::path INSTRUCTOR_CONFIG {DATA_DIR / "instructor_config.yaml"};
    inline const std::filesystem::path LOG_DIR {"instruct.log"};

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
}

#endif

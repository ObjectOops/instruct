#ifndef INSTRUCT_CONSTANTS_H
#define INSTRUCT_CONSTANTS_H

#include <filesystem>

namespace instruct::constants {
    inline const std::filesystem::path DATA_DIR {"instruct_data"};
    inline const std::filesystem::path INSTRUCTOR_CONFIG {DATA_DIR / "instructor_config.yaml"};

    inline constexpr int MAX_INSTRUCTOR_PASSWORD_LENGTH = 16;
}

#endif

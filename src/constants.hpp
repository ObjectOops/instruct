#ifndef INSTRUCT_CONSTANTS_H
#define INSTRUCT_CONSTANTS_H

#include <unordered_map>
#include <filesystem>
#include <string>
#include <chrono>
#include <vector>

#include "ftxui/component/component_options.hpp"

namespace instruct::constants {
    #ifdef __INSTRUCT_VERSION__
    const std::string INSTRUCT_VERSION = #__INSTRUCT_VERSION__;
    #else
    const std::string INSTRUCT_VERSION = "Unknown";
    #endif
    
    inline const std::filesystem::path DATA_DIR {"instruct_data"};
    inline const std::filesystem::path LOG_DIR {"instruct_logs"};
    inline const std::filesystem::path OPENVSCODE_SERVER_DIR {DATA_DIR / "openvscode-server"};
    
    inline const std::filesystem::path INSTRUCT_LOG_DIR {LOG_DIR / "instruct.log"};

    inline const std::filesystem::path INSTRUCTOR_CONFIG {DATA_DIR / "instructor_config.yaml"};
    inline const std::filesystem::path STUDENTS_CONFIG {DATA_DIR / "students_config.yaml"};
    inline const std::filesystem::path TESTS_CONFIG {DATA_DIR / "tests_config.yaml"};
    inline const std::filesystem::path UI_CONFIG {DATA_DIR / "ui_config.yaml"};

    inline constexpr int MAX_INSTRUCTOR_PASSWORD_LENGTH = 16;
    
    inline const std::string OPENVSCODE_SERVER_HOST {"github.com"}; // Note: Do not specify scheme.
    inline const std::string OPENVSCODE_SERVER_ROUTE_FORMAT {"/gitpod-io/openvscode-server/releases/download/openvscode-server-${VERSION}/openvscode-server-${VERSION}-linux-${PLATFORM}.tar.gz"};
    inline const std::string OPENVSCODE_SERVER_VERSION_DEFAULT {"v1.79.2"};
    inline const std::filesystem::path OPENVSCODE_SERVER_ARCHIVE {
        OPENVSCODE_SERVER_DIR / "openvscode-server.tar.gz"
    };
    inline const std::vector<std::string> OPENVSCODE_SERVER_PLATFORM {
        "arm64", "armhf", "x64"
    };
    inline const std::unordered_map<std::string, std::string> OPENVSCODE_SERVER_HASHES {
        {"v1.79.2", "d35dbc5678cdee5cce6c9522b5a8668c8982f35441dcda425aeba9e40d38764f"}
    };
        
    inline const int ASYNC_DISPLAY_SPINNER_TYPE {15};
    inline const std::chrono::milliseconds ASYNC_DISPLAY_SPINNER_INTERVAL {64};
}

#endif

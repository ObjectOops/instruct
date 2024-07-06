#include "setup.hpp"

bool dataDirExists() {
    bool pathExists {std::filesystem::exists(DATA_DIR)};
    bool isDir {std::filesystem::is_directory(DATA_DIR)};
    return pathExists && isDir;
}

bool createDataDir() {
    return std::filesystem::create_directory(DATA_DIR, setupError);
}

bool populateDataDir() {
    try {
        YAML::LoadFile(INSTRUCTOR_CONFIG);
    } catch (const std::exception &e) {
        setupError = std::make_error_code(std::errc::io_error);
        disableAlternateScreenBuffer();
        std::cerr << e.what() << '\n';
        enableAlternateScreenBuffer();
        return false;
    }
    return true;
}

void deleteDataDir() {
    std::filesystem::remove_all(DATA_DIR);
}

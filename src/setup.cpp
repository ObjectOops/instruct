#include "setup.hpp"

const SetupError &getSetupError() {
    return setupError;
}

bool setupIncomplete() {
    bool pathExists {std::filesystem::exists(DATA_DIR)};
    bool isDir {std::filesystem::is_directory(DATA_DIR)};
    return !(pathExists && isDir);
}

bool createDataDir() {
    bool success {std::filesystem::create_directory(DATA_DIR, setupError.errCode)};
    if (!success) {
        setupError.msg = "Failed to create data directory.";
    }
    return success;
}

bool populateDataDir() {
    std::filesystem::path paths [] {
        INSTRUCTOR_CONFIG
    };
    for (std::filesystem::path &path : paths) {
        try {
            std::ofstream fout;
            fout.exceptions(std::ios_base::failbit | std::ios_base::badbit);
            fout.open(path);
            fout.close();
        } catch (const std::exception &e) {
            setupError.errCode = std::make_error_code(std::errc::io_error);
            setupError.exMsg = e.what();
            setupError.msg = "Failed to create file `" + std::string {path} + "`.";
            return false;
        }
    }
    return true;
}

bool setDefaults() {
    try {
        instructData = std::make_unique<InstructData>(INSTRUCTOR_CONFIG);
        instructData->data["instructor_port"] = 3000;
        instructData->saveData();
    } catch (const std::exception &e) {
        setupError.errCode = std::make_error_code(std::errc::io_error);
        setupError.exMsg = e.what();
        setupError.msg = "Failed to set defaults.";
        return false;
    }
    return true;
}

void deleteDataDir() {
    std::filesystem::remove_all(DATA_DIR);
}

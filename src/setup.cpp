#include <filesystem>
#include <exception>
#include <fstream>
#include <memory>

#include "loguru.hpp"

#include "constants.hpp"
#include "setup.hpp"
#include "data.hpp"

using namespace instruct;

const setup::SetupError &setup::getSetupError() {
    return setupError;
}

bool setup::setupIncomplete() {
    bool pathExists {std::filesystem::exists(constants::DATA_DIR)};
    bool isDir {std::filesystem::is_directory(constants::DATA_DIR)};
    DLOG_F(INFO, "Data path stat: %d %d", pathExists, isDir);
    return !(pathExists && isDir);
}

bool setup::createDataDir() {
    bool success {
        std::filesystem::create_directory(
            constants::DATA_DIR, setupError.errCode
        )
    };
    if (!success) {
        setupError.msg = "Failed to create data directory.";
    }
    return success;
}

bool setup::populateDataDir() {
    std::filesystem::path paths [] {
        constants::INSTRUCTOR_CONFIG
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
            setupError.msg = 
                "Failed to create file `" + std::string {path} + "`.";
            return false;
        }
    }
    return true;
}

bool setup::setDefaults() {
    try {
        DLOG_F(INFO, "Assigning default data.");
        IData::instructorData->set_authHost("0.0.0.0");
        IData::instructorData->set_authPort(8000);
        IData::instructorData->set_codePort(3000);
        IData::instructorData->set_firstTime(true);
        DLOG_F(INFO, "Saved default data.");
    } catch (const std::exception &e) {
        setupError.errCode = std::make_error_code(std::errc::io_error);
        setupError.exMsg = e.what();
        setupError.msg = "Failed to set defaults.";
        return false;
    }
    return true;
}

void setup::deleteDataDir() {
    DLOG_F(INFO, "Deleted data dir.");
    std::filesystem::remove_all(constants::DATA_DIR);
}

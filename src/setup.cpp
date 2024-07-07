#include "setup.hpp"

const instruct::setup::SetupError &instruct::setup::getSetupError() {
    return instruct::setup::setupError;
}

bool instruct::setup::setupIncomplete() {
    bool pathExists {std::filesystem::exists(instruct::constants::DATA_DIR)};
    bool isDir {std::filesystem::is_directory(instruct::constants::DATA_DIR)};
    DLOG_F(INFO, "Data path stat: %d %d", pathExists, isDir);
    return !(pathExists && isDir);
}

bool instruct::setup::createDataDir() {
    bool success {
        std::filesystem::create_directory(
            instruct::constants::DATA_DIR, instruct::setup::setupError.errCode
        )
    };
    if (!success) {
        instruct::setup::setupError.msg = "Failed to create data directory.";
    }
    return success;
}

bool instruct::setup::populateDataDir() {
    std::filesystem::path paths [] {
        instruct::constants::INSTRUCTOR_CONFIG
    };
    for (std::filesystem::path &path : paths) {
        try {
            std::ofstream fout;
            fout.exceptions(std::ios_base::failbit | std::ios_base::badbit);
            fout.open(path);
            fout.close();
        } catch (const std::exception &e) {
            instruct::setup::setupError.errCode = std::make_error_code(std::errc::io_error);
            instruct::setup::setupError.exMsg = e.what();
            instruct::setup::setupError.msg = 
                "Failed to create file `" + std::string {path} + "`.";
            return false;
        }
    }
    return true;
}

bool instruct::setup::setDefaults() {
    try {
        instruct::Data::initAll();
        instruct::Data::instructData->data[instruct::constants::INSTRUCTOR_PORT_KEY] = 3000;
        instruct::Data::instructData->saveData();
    } catch (const std::exception &e) {
        instruct::setup::setupError.errCode = std::make_error_code(std::errc::io_error);
        instruct::setup::setupError.exMsg = e.what();
        instruct::setup::setupError.msg = "Failed to set defaults.";
        return false;
    }
    return true;
}

void instruct::setup::deleteDataDir() {
    DLOG_F(INFO, "Deleted data dir.");
    std::filesystem::remove_all(instruct::constants::DATA_DIR);
}

#include "setup.hpp"

const Instruct::Setup::SetupError &Instruct::Setup::getSetupError() {
    return Instruct::Setup::setupError;
}

bool Instruct::Setup::setupIncomplete() {
    bool pathExists {std::filesystem::exists(Instruct::Constants::DATA_DIR)};
    bool isDir {std::filesystem::is_directory(Instruct::Constants::DATA_DIR)};
    return !(pathExists && isDir);
}

bool Instruct::Setup::createDataDir() {
    bool success {
        std::filesystem::create_directory(
            Instruct::Constants::DATA_DIR, Instruct::Setup::setupError.errCode
        )
    };
    if (!success) {
        Instruct::Setup::setupError.msg = "Failed to create data directory.";
    }
    return success;
}

bool Instruct::Setup::populateDataDir() {
    std::filesystem::path paths [] {
        Instruct::Constants::INSTRUCTOR_CONFIG
    };
    for (std::filesystem::path &path : paths) {
        try {
            std::ofstream fout;
            fout.exceptions(std::ios_base::failbit | std::ios_base::badbit);
            fout.open(path);
            fout.close();
        } catch (const std::exception &e) {
            Instruct::Setup::setupError.errCode = std::make_error_code(std::errc::io_error);
            Instruct::Setup::setupError.exMsg = e.what();
            Instruct::Setup::setupError.msg = 
                "Failed to create file `" + std::string {path} + "`.";
            return false;
        }
    }
    return true;
}

bool Instruct::Setup::setDefaults() {
    try {
        Instruct::Data::instructData = 
            std::make_unique<Instruct::Data>(Instruct::Constants::INSTRUCTOR_CONFIG);
        Instruct::Data::instructData->data["instructor_port"] = 3000;
        Instruct::Data::instructData->saveData();
    } catch (const std::exception &e) {
        Instruct::Setup::setupError.errCode = std::make_error_code(std::errc::io_error);
        Instruct::Setup::setupError.exMsg = e.what();
        Instruct::Setup::setupError.msg = "Failed to set defaults.";
        return false;
    }
    return true;
}

void Instruct::Setup::deleteDataDir() {
    std::filesystem::remove_all(Instruct::Constants::DATA_DIR);
}

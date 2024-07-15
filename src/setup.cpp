#include <filesystem>
#include <exception>
#include <fstream>
#include <memory>

#include "loguru.hpp"

#include "constants.hpp"
#include "setup.hpp"
#include "data.hpp"

namespace instruct {

static setup::SetupError setupError {};

setup::SetupError &setup::getSetupError() {
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
        constants::INSTRUCTOR_CONFIG, 
        constants::STUDENTS_CONFIG, 
        constants::TESTS_CONFIG, 
        constants::UI_CONFIG
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
        DLOG_F(INFO, "Assigning default instructor data.");
        IData::instructorData->set_authHost("0.0.0.0");
        IData::instructorData->set_authPort(8000);
        IData::instructorData->set_codePort(3000);
        IData::instructorData->set_firstTime(true);
        DLOG_F(INFO, "Saved default data.");

        DLOG_F(INFO, "Assigning default students data.");
        SData::studentsData->set_authHost("0.0.0.0");
        SData::studentsData->set_authPort(8001);
        SData::studentsData->set_codePorts({});
        SData::studentsData->set_codePortRange({3001, 4000});
        SData::studentsData->set_useRandomPorts(true);
        #if DEBUG
        uuids::uuid debugStudentUUID {
            uuids::uuid::from_string("9d5b69cc-1aca-46bd-940a-de1f110357a9").value()
        };
        SData::studentsData->set_students({{
            debugStudentUUID, 
            {
                debugStudentUUID, 
                "Placeholder Student", 
                "7bcbc837d7bf10efc0019386dc1eb29637669eac0a6285ad441f31f97f72f25c", 
                "3805596074", 
                false
            }
        }});
        #endif
        DLOG_F(INFO, "Saved default data.");
        
        DLOG_F(INFO, "Assigning default tests data.");
        TData::testsData->set_selectedTestUUIDs({});
        #if DEBUG
        uuids::uuid debugTestUUID {
            uuids::uuid::from_string("78127002-2f3d-4655-83fd-d0f6b7e25b95").value()
        };
        TData::testsData->set_tests({{
            debugTestUUID, 
            {
                debugTestUUID, 
                "Placeholder Test", 
                "echo 'Hello, world!'", 
                "echo 'Hello, world!'"
            }
        }});
        #endif
        DLOG_F(INFO, "Saved default data.");
        
        DLOG_F(INFO, "Assigning default UI data.");
        UData::uiData->set_alwaysShowStudentUUIDs(false);
        UData::uiData->set_alwaysShowTestUUIDs(false);
        UData::uiData->set_studentPaneWidth(36);
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

}

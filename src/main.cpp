#include "loguru.hpp"

#include "security.hpp"
#include "logging.hpp"
#include "setup.hpp"
#include "ui.hpp"

int main(int argc, char **argv) {
    
    instruct::log::configureLogging(argc, argv);
    
    LOG_F(INFO, "Instruct launched. Timestamps are recorded in local time.");
    
    instruct::log::logVersion();
    
    if (instruct::setup::setupIncomplete()) {
        LOG_F(INFO, "Set up incomplete. Starting setup.");
        auto [setupComplete, setupCode] {instruct::ui::setupMenu()};
        if (!setupComplete) {
            LOG_F(INFO, "Exiting set up (incomplete).");
            return setupCode;
        }
    }
    
    LOG_F(1, "Reading data.");
    if (!instruct::ui::initAllHandled()) {
        LOG_F(INFO, "Exiting.");
        return EXIT_FAILURE;
    }
    if (instruct::sec::instanceActive()) {
        LOG_F(INFO, "Already running. Exiting.");
        instruct::ui::print("Detected an instance of instruct already running.\n");
        return EXIT_FAILURE;
    }
    instruct::sec::createInstance();
    LOG_F(1, "Instance locked.");
    
    LOG_F(INFO, "Starting main application");
    instruct::ui::mainMenu();
    
    LOG_F(INFO, "Finalizing save data.");
    if (!instruct::ui::saveAllHandled()) {
        LOG_F(INFO, "Exiting.");
        return EXIT_FAILURE;
    }
    
    LOG_F(INFO, "Exiting main application.");
    
    return EXIT_SUCCESS;
}

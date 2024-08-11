#include <string>

#include "loguru.hpp"

#include "constants.hpp"
#include "logging.hpp"

namespace instruct {

void log::configureLogging(int argc, char **argv) {
    using namespace std::string_literals;
    
    bool verbose {argc == 2 && argv[1] == "-v"s};

    loguru::g_stderr_verbosity = loguru::Verbosity_OFF;
    loguru::g_preamble_uptime = false;
    loguru::g_preamble_thread = verbose;
    loguru::g_preamble_file = verbose;
    // loguru::init(argc, argv);
    
    loguru::add_file(
        constants::INSTRUCT_LOG_DIR.c_str(), 
        loguru::Truncate, 
        verbose ? loguru::Verbosity_MAX : loguru::Verbosity_INFO
    );
}

void log::logVersion() {
    LOG_F(INFO, "Instruct Version: %s", constants::INSTRUCT_VERSION.c_str());
}

void log::logExceptionWarning(const std::exception &e) {
    LOG_F(WARNING, "%s --> %s", typeid(e).name(), e.what());
}
void log::logErrorCodeWarning(const std::error_code &err) {
    if (err.value() == 0) {
        return;
    }
    LOG_F(
        WARNING, 
        "Error Code: %d --> %s --> %s", 
        err.value(), err.message().c_str(), err.category().name()
    );
}

}

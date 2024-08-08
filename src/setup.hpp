#ifndef INSTRUCT_SETUP_HPP
#define INSTRUCT_SETUP_HPP

#include <system_error>
#include <string>
#include <atomic>

namespace instruct::setup {
    struct SetupError {
        std::error_code errCode;
        std::string exType, exMsg, msg;
    };

    SetupError &getSetupError();

    bool setupIncomplete();

    bool createDataDir();

    bool populateDataDir();

    bool setDefaults();
    
    bool locateCACerts();

    void deleteDataDir();
    
    bool deleteOVSCSDirContents();
    
    bool downloadOVSCS(
        const std::string &, std::atomic<uint64_t> &, std::atomic<uint64_t> &, int
    );
    
    bool unpackOVSCSTarball();
}

#endif

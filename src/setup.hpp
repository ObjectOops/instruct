#ifndef INSTRUCT_SETUP_HPP
#define INSTRUCT_SETUP_HPP

#include <system_error>
#include <string>

namespace instruct::setup {
    struct SetupError {
        std::error_code errCode;
        std::string msg, exMsg;
    };

    SetupError &getSetupError();

    bool setupIncomplete();

    bool createDataDir();

    bool populateDataDir();

    bool setDefaults();

    void deleteDataDir();
}

#endif

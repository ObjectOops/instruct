#ifndef INSTRUCT_SETUP_HPP
#define INSTRUCT_SETUP_HPP

#include <system_error>
#include <filesystem>
#include <exception>
#include <fstream>
#include <string>
#include <memory>

#include "constants.hpp"
#include "data.hpp"

namespace Instruct::Setup {
    struct SetupError {
        std::error_code errCode;
        std::string msg, exMsg;
    };

    inline SetupError setupError {};

    const SetupError &getSetupError();

    bool setupIncomplete();

    bool createDataDir();

    bool populateDataDir();

    bool setDefaults();

    void deleteDataDir();
}

#endif

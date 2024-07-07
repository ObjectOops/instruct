#ifndef INSTRUCT_SECURITY_HPP
#define INSTRUCT_SECURITY_HPP

#include <string>

#include "yaml-cpp/yaml.h"
#include "picosha2.h"

#include "notification.hpp"
#include "constants.hpp"
#include "terminal.hpp"
#include "setup.hpp"
#include "data.hpp"

namespace Instruct::Security {
    bool updateInstructPswd(const std::string &);
}

#endif

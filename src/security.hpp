#ifndef INSTRUCT_SECURITY_HPP
#define INSTRUCT_SECURITY_HPP

#include <string>

#include "yaml-cpp/yaml.h"
#include "picosha2.h"
#include "loguru.hpp"
#include "httplib.h"

#include "notification.hpp"
#include "constants.hpp"
#include "terminal.hpp"
#include "network.hpp"
#include "setup.hpp"
#include "data.hpp"

namespace instruct::sec {
    bool instanceActive();
    
    bool updateInstructPswd(const std::string &);
}

#endif

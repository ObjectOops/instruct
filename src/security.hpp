#ifndef INSTRUCT_SECURITY_HPP
#define INSTRUCT_SECURITY_HPP

#include <string>
#include <thread>

#include "picosha2.h"
#include "loguru.hpp"
#include "httplib.h"

#include "notification.hpp"
#include "constants.hpp"
#include "setup.hpp"
#include "data.hpp"

// This is a huge single header include, so it goes here exclusively.
#include "httplib.h"

namespace instruct::sec {
    bool instanceActive();
    void createInstance();
    
    bool updateInstructPswd(const std::string &);
}

#endif

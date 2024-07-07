#ifndef INSTRUCT_NETWORK_HPP
#define INSTRUCT_NETWORK_HPP

#include <string>

// This is a huge single header include, so it goes here exclusively.
#include "httplib.h"

namespace instruct::net {
    bool ping(std::string);
}

#endif

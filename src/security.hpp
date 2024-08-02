#ifndef INSTRUCT_SECURITY_HPP
#define INSTRUCT_SECURITY_HPP

#include <unordered_map>
#include <string>

#include "uuid.h"

#include "data.hpp"

namespace instruct::sec {
    bool instanceActive();
    bool createInstance();
    
    bool updateInstructPswd(const std::string &);
    void updateStudentPswd(
        std::unordered_map<uuids::uuid, SData::Student> &, 
        const uuids::uuid &, 
        const std::string &
    );
    
    bool verifyOVSCSTarball(const std::string &);
}

#endif

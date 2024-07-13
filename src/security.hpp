#ifndef INSTRUCT_SECURITY_HPP
#define INSTRUCT_SECURITY_HPP

#include <string>

namespace instruct::sec {
    bool instanceActive();
    bool createInstance();
    
    bool updateInstructPswd(const std::string &);
}

#endif

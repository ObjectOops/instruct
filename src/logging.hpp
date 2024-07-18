#ifndef INSTRUCT_LOGGING_HPP
#define INSTRUCT_LOGGING_HPP

#include <exception>

namespace instruct::log {
    void configureLogging(int, char **);
    void logVersion();
    void logExceptionWarning(const std::exception &);
}

#endif

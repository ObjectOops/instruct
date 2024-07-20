#ifndef INSTRUCT_LOGGING_HPP
#define INSTRUCT_LOGGING_HPP

#include <system_error>
#include <exception>

namespace instruct::log {
    void configureLogging(int, char **);
    void logVersion();
    
    void logExceptionWarning(const std::exception &);
    void logErrorCodeWarning(const std::error_code &);
}

#endif

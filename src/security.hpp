#ifndef INSTRUCT_SECURITY_HPP
#define INSTRUCT_SECURITY_HPP

#include <unordered_map>
#include <string>
#include <thread>
#include <memory>

#include "httplib.h"
#include "uuid.h"

#include "data.hpp"

namespace instruct::sec {
    class ThreadedServer {
        public:
        
        std::unique_ptr<std::thread> worker;
        std::unique_ptr<httplib::Server> server;
        bool initialized;
        
        ThreadedServer();
        ThreadedServer(const std::string &, int, const std::string &, httplib::Server::Handler);
        ThreadedServer(const ThreadedServer &) = delete;
        ThreadedServer &operator=(const ThreadedServer &) = delete;
        ThreadedServer &operator=(ThreadedServer &&) noexcept;
        ~ThreadedServer();
    };
    
    bool instanceActive();
    ThreadedServer createInstance();
    
    bool updateInstructPswd(const std::string &);
    void updateStudentPswd(
        std::unordered_map<uuids::uuid, SData::Student> &, 
        const uuids::uuid &, 
        const std::string &
    );
    
    bool verifyOVSCSTarball(const std::string &);
}

#endif

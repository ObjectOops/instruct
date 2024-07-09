#include <thread>

#include "picosha2.h"
#include "loguru.hpp"
#include "httplib.h"

#include "notification.hpp"
#include "security.hpp"
#include "setup.hpp"
#include "data.hpp"

#include "httplib.h"

using namespace instruct;

const std::string ALIVE_CODE {"Instruct Alive"};

bool sec::instanceActive() {
    std::string port = std::to_string(IData::instructorData->authPort);
    std::string url {"http://localhost:" + port};
    DLOG_F(INFO, "Checking for active instance at: %s", url.c_str());

    httplib::Client client {url};
    httplib::Result res {client.Get("/heartbeat")};
    
    // Managed to establish connection.
    bool isGood {res.error() != httplib::Error::Connection};
    
    if (isGood && res->body != ALIVE_CODE) {
        LOG_F(WARNING, ("Detected an unknown process using port " + port + ".").c_str());
    }
    
    return isGood;
}
void sec::createInstance() {
    std::thread worker {[&] {
        httplib::Server server {};
        server.Get(
            "/heartbeat", 
            [&] (const httplib::Request &, httplib::Response &res) {
                DLOG_F(INFO, "Heartbeat connection received.");
                res.set_content(ALIVE_CODE, "text/plain");
            }
        );
        server.listen(
            IData::instructorData->authHost, 
            IData::instructorData->authPort
        );
    }};
    worker.detach();
}

bool sec::updateInstructPswd(const std::string &instructPswd) {
    try {
        IData::instructorData->pswdSHA256 = picosha2::hash256_hex_string(instructPswd);
        IData::instructorData->saveData();
    } catch (const std::exception &e) {
        // Only relevant during initial set up.
        setup::setupError.errCode = std::make_error_code(std::errc::io_error);
        setup::setupError.exMsg = e.what();
        setup::setupError.msg = "Failed to save instructor password.";
        
        notif::setNotification(
            "Failed to save password. Your new password will only be used during this session."
        );
        return false;
    }
    return true;
}

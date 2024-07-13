#include <exception>
#include <thread>
#include <random>

#include "picosha2.h"
#include "loguru.hpp"
#include "httplib.h"

#include "notification.hpp"
#include "security.hpp"
#include "setup.hpp"
#include "data.hpp"

namespace instruct {

static const std::string ALIVE_CODE {"Instruct Alive"};

bool sec::instanceActive() {
    std::string port = std::to_string(IData::instructorData->get_authPort());
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
bool sec::createInstance() {
    try {
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
                IData::instructorData->get_authHost(), 
                IData::instructorData->get_authPort()
            );
        }};
        worker.detach();
    } catch (const std::exception &e) {
        LOG_F(ERROR, "Failed to create instance. Exception: %s", e.what());
        return false;
    }
    return true;
}

bool sec::updateInstructPswd(const std::string &instructPswd) {
    try {
        // These are for salting.
        // Maybe a more cryto-secure RNG would be better.
        std::random_device seed {};
        std::mt19937 rng {seed()};
        std::string salt {std::to_string(rng())};
        
        IData::instructorData->set_pswdSHA256(picosha2::hash256_hex_string(instructPswd + salt));
        IData::instructorData->set_pswdSalt(salt);
    } catch (const std::exception &e) {
        // Only relevant during initial set up.
        setup::SetupError &setupError {setup::getSetupError()};
        setupError.errCode = std::make_error_code(std::errc::io_error);
        setupError.exMsg = e.what();
        setupError.msg = "Failed to save instructor password.";
        
        notif::setNotification(
            "Failed to save password. Your new password will only be used during this session."
        );
        return false;
    }
    return true;
}

}

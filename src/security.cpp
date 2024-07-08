#include "security.hpp"

const std::string ALIVE_CODE {"Instruct Alive"};

bool instruct::sec::instanceActive() {
    std::string port = std::to_string(
        instruct::Data::instructData->data[
            instruct::constants::INSTRUCTOR_AUTH_PORT_KEY
        ].as<int>()
    );
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
void instruct::sec::createInstance() {
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
            instruct::Data::instructData->data[
                instruct::constants::INSTRUCTOR_HOST_KEY
            ].as<std::string>(), 
            instruct::Data::instructData->data[
                instruct::constants::INSTRUCTOR_AUTH_PORT_KEY
            ].as<int>()
        );
    }};
    worker.detach();
}

bool instruct::sec::updateInstructPswd(const std::string &instructPswd) {
    try {
        instruct::Data::instructData->data["instructor_password_sha256"] 
            = picosha2::hash256_hex_string(instructPswd);
        instruct::Data::instructData->saveData();
    } catch (const std::exception &e) {
        // Only relevant during initial set up.
        instruct::setup::setupError.errCode = std::make_error_code(std::errc::io_error);
        instruct::setup::setupError.exMsg = e.what();
        instruct::setup::setupError.msg = "Failed to save instructor password.";
        
        instruct::notif::setNotification(
            "Failed to save password. Your new password will only be used during this session."
        );
        return false;
    }
    return true;
}

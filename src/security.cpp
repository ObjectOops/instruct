#include <exception>
#include <typeinfo>
#include <fstream>
#include <random>
#include <tuple>

#include "picosha2.h"
#include "loguru.hpp"

#include "notification.hpp"
#include "constants.hpp"
#include "security.hpp"
#include "logging.hpp"
#include "setup.hpp"

namespace instruct {

static const std::string ALIVE_CODE {"Instruct Alive"};

sec::ThreadedServer::ThreadedServer() : initialized {false} {
}

sec::ThreadedServer::ThreadedServer(
    const std::string &host, 
    int port, 
    const std::string &route, 
    httplib::Server::Handler handler
) {
    try {
        // The `this` pointer goes out-of-scope, so we'll have to hack around it.
        server = std::make_unique<httplib::Server>();
        httplib::Server *new_server {server.get()};
        worker = std::make_unique<std::thread>([=] {
            new_server->Get(route, handler);
            new_server->listen(host, port);
        });
        initialized = true;
    } catch (const std::exception &e) {
        LOG_F(ERROR, "Failed to create instance. Exception: %s", e.what());
        LOG_F(ERROR, "%s", typeid(e).name());
        LOG_F(ERROR, "%s", e.what());
        initialized = false;
    }
}

sec::ThreadedServer &sec::ThreadedServer::operator=(sec::ThreadedServer &&rhs) noexcept {
    worker = std::move(rhs.worker);
    server = std::move(rhs.server);
    initialized = rhs.initialized;
    rhs.initialized = false;
    return *this;
}

sec::ThreadedServer::~ThreadedServer() {
    if (server != nullptr) {
        server->stop();
    }
    if (worker != nullptr) {
        worker->join();
    }
}

bool sec::instanceActive() {
    std::string port = std::to_string(IData::instructorData->get_authPort());
    std::string url {"http://localhost:" + port};
    DLOG_F(INFO, "Checking for active instance at: %s", url.c_str());

    httplib::Client client {url};
    httplib::Result res {client.Get("/heartbeat")};
    
    // Managed to establish connection.
    bool isGood {res.error() != httplib::Error::Connection};
    
    if (isGood && res->body != ALIVE_CODE) {
        LOG_F(WARNING, "%s", ("Detected an unknown process using port " + port + ".").c_str());
    }
    
    return isGood;
}
sec::ThreadedServer sec::createInstance() {
    return {
        IData::instructorData->get_authHost(), 
        IData::instructorData->get_authPort(), 
        "/heartbeat", 
        [] (const httplib::Request &, httplib::Response &res) {
            DLOG_F(INFO, "Heartbeat connection received.");
            res.set_content(ALIVE_CODE, "text/plain");
        }
    };
}

static std::tuple<std::string, std::string> hashAndSalt(const std::string &pswd) {
    // These are for salting. Maybe a more cryto-secure RNG would be better.
    std::random_device seed {};
    std::mt19937 rng {seed()};
    std::string salt {std::to_string(rng())};
    
    return {picosha2::hash256_hex_string(pswd + salt), salt};
}

bool sec::updateInstructPswd(const std::string &instructPswd) {
    try {
        auto [instructPswdHash, instructPswdSalt] {hashAndSalt(instructPswd)};
        
        // There exists a possible edge-case here where the first save succeeds but 
        // the second one doesn't. However, I can think of no plausible instance 
        // where such a scenario can occur.
        IData::instructorData->set_pswdSHA256(instructPswdHash);
        IData::instructorData->set_pswdSalt(instructPswdSalt);
    } catch (const std::exception &e) {
        if (IData::instructorData->get_firstTime()) {
            // Only relevant during initial set up.
            setup::SetupError &setupError {setup::getSetupError()};
            setupError.errCode = std::make_error_code(std::errc::io_error);
            setupError.exType = typeid(e).name();
            setupError.exMsg = e.what();
            setupError.msg = "Failed to save instructor password.";
        }
        
        notif::notify(
            "Failed to save password. Your new password will only be used during this session."
        );
        
        log::logExceptionWarning(e);
        return false;
    }
    return true;
}

void sec::updateStudentPswd(
    std::unordered_map<uuids::uuid, SData::Student> &studentMap, 
    const uuids::uuid &studentUUID, 
    const std::string &studentPswd
) {
    auto [studentPswdHash, studentPswdSalt] {hashAndSalt(studentPswd)};
    SData::Student &student {studentMap.at(studentUUID)};
    student.pswdSHA256 = studentPswdHash;
    student.pswdSalt = studentPswdSalt;
}

bool sec::verifyOVSCSTarball(const std::string &ovscsVersion) {
    try {
        std::ifstream fin {constants::OPENVSCODE_SERVER_ARCHIVE, std::ios::binary};
        std::string hash {picosha2::hash256_hex_string(
            std::istreambuf_iterator<char> {fin}, 
            std::istreambuf_iterator<char> {}
        )};
        fin.close();
        DLOG_F(INFO, "Tarball hash: %s", hash.c_str());
        return constants::OPENVSCODE_SERVER_HASHES.count(ovscsVersion) != 0 
            && hash == constants::OPENVSCODE_SERVER_HASHES.at(ovscsVersion);
    } catch (const std::exception &e) {
        log::logExceptionWarning(e);
        return false;
    }
}

}

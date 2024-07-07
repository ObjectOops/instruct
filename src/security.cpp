#include "security.hpp"

bool Instruct::Security::updateInstructPswd(const std::string &instructPswd) {
    try {
        Instruct::Data::instructData->data["instructor_password_sha256"] 
            = picosha2::hash256_hex_string(instructPswd);
        Instruct::Data::instructData->saveData();
    } catch (const std::exception &e) {
        // Only relevant during initial set up.
        Instruct::Setup::setupError.errCode = std::make_error_code(std::errc::io_error);
        Instruct::Setup::setupError.exMsg = e.what();
        Instruct::Setup::setupError.msg = "Failed to save instructor password.";
        
        Instruct::Notification::setNotification(
            "Failed to save password. Your new password will only be used during this session."
        );
        return false;
    }
    return true;
}

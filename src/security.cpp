#include "security.hpp"

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

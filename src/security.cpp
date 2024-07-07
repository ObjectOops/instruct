#include "security.hpp"

bool updateInstructPswd(const std::string &instructPswd) {
    try {
        instructData->data["instructor_password_sha256"] = picosha2::hash256_hex_string(instructPswd);
        instructData->saveData();
    } catch (const std::exception &e) {
        // Only relevant during initial set up.
        setupError.errCode = std::make_error_code(std::errc::io_error);
        setupError.exMsg = e.what();
        setupError.msg = "Failed to save instructor password.";
        
        setNotification("Failed to save password. Your new password will only be used during this session.");
        return false;
    }
    return true;
}

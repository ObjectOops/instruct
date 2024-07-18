#include "loguru.hpp"

#include "notification.hpp"

namespace instruct::notif {

namespace {
    // This needs to be a vector since the FTXUI Menu component only takes vectors.
    std::vector<std::string> recentNotifications {};
    std::string notification {};
    bool notice {false};
}

void notify(const std::string &newNotification) {
    notice = true;
    notification = newNotification;
    recentNotifications.insert(recentNotifications.begin(), notification);
    
    LOG_F(INFO, "New Notification: %s", newNotification.c_str());
}
void setNotification(const std::string &notif) {
    notification = notif;
}
const std::string &getNotification() {
    return notification;
}
const std::vector<std::string> &getRecentNotifications() {
    return recentNotifications;
}

bool &getNotice() {
    return notice;
}
void ackNotice() {
    notice = false;
}
void copyNotice() {
    notice = true;
}

}

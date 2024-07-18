#ifndef INSTRUCT_NOTIFICATION_HPP
#define INSTRUCT_NOTIFICATION_HPP

#include <string>
#include <vector>

namespace instruct::notif {
    // This needs to be a vector since the FTXUI Menu component only takes vectors.
    inline std::vector<std::string> recentNotifications {};
    inline std::string notification {};
    inline bool notice {false};

    inline void setNotification(const std::string &newNotification) {
        notice = true;
        notification = newNotification;
        recentNotifications.insert(recentNotifications.begin(), notification);
    }
    inline const std::string &getNotification() {
        return notification;
    }
    inline const std::vector<std::string> &getRecentNotifications() {
        return recentNotifications;
    }

    inline bool &getNotice() {
        return notice;
    }
    inline void ackNotice() {
        notice = false;
    }
    inline void copyNotice() {
        notice = true;
    }
}

#endif

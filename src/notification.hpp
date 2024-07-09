#ifndef INSTRUCT_NOTIFICATION_HPP
#define INSTRUCT_NOTIFICATION_HPP

#include <string>
#include <vector>

namespace instruct::notif {
    inline std::vector<std::string> recentNotifications {};
    inline std::string notification {};
    inline bool notice {false};

    inline void setNotification(const std::string &newNotification) {
        notice = true;
        notification = newNotification;
        recentNotifications.push_back(notification);
    }
    inline const std::string &getNotification() {
        return notification;
    }

    inline bool getNotice() {
        return notice;
    }
    inline void ackNotice() {
        notice = false;
    }
}

#endif

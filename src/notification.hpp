#ifndef INSTRUCT_NOTIFICATION_HPP
#define INSTRUCT_NOTIFICATION_HPP

#include <string>
#include <list>

namespace instruct::notif {
    inline std::list<std::string> recentNotifications {};
    inline std::string notification {};
    inline bool notice {false};

    inline void setNotification(const std::string &newNotification) {
        notice = true;
        notification = newNotification;
        recentNotifications.push_front(notification);
    }
    inline const std::string &getNotification() {
        return notification;
    }
    inline const std::list<std::string> &getRecentNotifications() {
        return recentNotifications;
    }

    inline bool getNotice() {
        return notice;
    }
    inline void ackNotice() {
        notice = false;
    }
}

#endif

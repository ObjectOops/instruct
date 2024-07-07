#ifndef INSTRUCT_NOTIFICATION_HPP
#define INSTRUCT_NOTIFICATION_HPP

#include <string>

inline std::string notification {};
inline bool notice {false};

inline void setNotification(const std::string &newNotification) {
    notice = true;
    notification = newNotification;
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

#endif

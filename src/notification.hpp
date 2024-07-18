#ifndef INSTRUCT_NOTIFICATION_HPP
#define INSTRUCT_NOTIFICATION_HPP

#include <string>
#include <vector>

namespace instruct::notif {
    void notify(const std::string &newNotification);
    
    void setNotification(const std::string &notification);
    const std::string &getNotification();
    const std::vector<std::string> &getRecentNotifications();

    bool &getNotice();
    void ackNotice();
    void copyNotice();
}

#endif

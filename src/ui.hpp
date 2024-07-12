#ifndef INSTRUCT_UI_HPP
#define INSTRUCT_UI_HPP

#include <string>
#include <tuple>

namespace instruct::ui {
    bool initAllHandled();
    bool saveAllHandled();
    void print(const std::string &);
    std::tuple<bool, int> setupMenu();
    bool instructSetup(const std::string &);
    bool mainMenu();
}

#endif

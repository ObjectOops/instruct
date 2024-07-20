#ifndef INSTRUCT_UI_HPP
#define INSTRUCT_UI_HPP

#include <string>
#include <tuple>

namespace instruct::ui {
    bool initAllHandled();
    std::tuple<bool, bool> saveAllHandled();
    void print(const std::string &);
    std::tuple<bool, int> setupMenu();
    std::tuple<bool, bool> mainMenu();
}

#endif

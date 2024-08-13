#ifndef INSTRUCT_INPUT_HPP
#define INSTRUCT_INPUT_HPP

#include "ftxui/component/component.hpp"
#include "ftxui/component/task.hpp"

namespace instruct::ui {
    ftxui::Element inputTransformCustom(ftxui::InputState);
    ftxui::Component makeInput(
        std::string &, 
        std::string, 
        ftxui::Closure = {}, 
        ftxui::InputOption = ftxui::InputOption::Default()
    );
}

#endif

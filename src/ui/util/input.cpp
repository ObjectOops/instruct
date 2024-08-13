#include "input.hpp"

namespace instruct {

ftxui::Element ui::inputTransformCustom(ftxui::InputState state) {
    state.element |= ftxui::color(ftxui::Color::White);
    if (state.is_placeholder) {
        state.element |= ftxui::dim;
    }
    if (!state.focused && state.hovered) {
        state.element |= ftxui::bgcolor(ftxui::Color::GrayDark);
    }
    return state.element;
}

ftxui::Component ui::makeInput(
    std::string &content, 
    std::string placeholder, 
    ftxui::Closure on_enter, 
    ftxui::InputOption base
) {
    ftxui::InputOption inputOptions {base};
    inputOptions.content = &content;
    inputOptions.multiline = false;
    inputOptions.on_enter = on_enter;
    inputOptions.placeholder = placeholder;
    inputOptions.transform = inputTransformCustom;
    return ftxui::Input(inputOptions);
}

}

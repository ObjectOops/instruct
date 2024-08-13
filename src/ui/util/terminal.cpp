#include <iostream>

#include "terminal.hpp"

namespace instruct {

void ui::enableAlternateScreenBuffer() {
    std::cout << "\033[?1049h";
}

void ui::disableAlternateScreenBuffer() {
    std::cout << "\033[?1049l";
}

void ui::forceCursorRow(int row) {
    std::cout << "\033[" + std::to_string(row) + ";H";
}

}
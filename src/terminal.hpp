#ifndef INSTRUCT_TERMINAL_HPP
#define INSTRUCT_TERMINAL_HPP

#include <iostream>

namespace instruct::term {
    inline void enableAlternateScreenBuffer() {
        std::cout << "\033[?1049h";
    }

    inline void disableAlternateScreenBuffer() {
        std::cout << "\033[?1049l";
    }
    
    inline void forceCursorRow(int row) {
        std::cout << "\033[" + std::to_string(row) + ";H";
    }
}

#endif

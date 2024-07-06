#ifndef INSTRUCT_TERMINAL_HPP
#define INSTRUCT_TERMINAL_HPP

#include <iostream>

using namespace std;

inline void enableAlternateScreenBuffer() {
    cout << "\033[?1049h";
}

inline void disableAlternateScreenBuffer() {
    cout << "\033[?1049l";
}

#endif

#ifndef INSTRUCT_TERMINAL_HPP
#define INSTRUCT_TERMINAL_HPP

#include <iostream>

using namespace std;

void enableAlternateScreenBuffer() {
    cout << "\033[?1049h";
}

void disableAlternateScreenBuffer() {
    cout << "\033[?1049l";
}

#endif

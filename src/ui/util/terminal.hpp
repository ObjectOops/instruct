#ifndef INSTRUCT_TERMINAL_HPP
#define INSTRUCT_TERMINAL_HPP

namespace instruct::ui {
    void enableAlternateScreenBuffer();
    void disableAlternateScreenBuffer();
    void forceCursorRow(int row);
}

#endif

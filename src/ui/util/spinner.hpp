#ifndef INSTRUCT_SPINNER_HPP
#define INSTRUCT_SPINNER_HPP

#include <thread>
#include <atomic>

namespace instruct::ui {
    void startAsyncSpinner(const std::string &);
    void stopAsyncSpinner();
}

#endif

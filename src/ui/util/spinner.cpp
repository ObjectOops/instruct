#include <string>

#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "loguru.hpp"

#include "../../constants.hpp"
#include "terminal.hpp"
#include "spinner.hpp"

namespace instruct {

static std::atomic_bool spinning {false};
static std::thread spinnerThread {};

void ui::startAsyncSpinner(const std::string &msg) {
    // Make a copy first to avoid a race condition.
    spinnerThread = std::thread {[=] {
        DLOG_F(INFO, "Spinner thread started.");
        spinning = true;
        auto spinnerScreen {ftxui::Screen::Create({ftxui::Terminal::Size().dimx, 1})};
        unsigned long long step {};
        while (spinning) {
            ftxui::Element spinner {ftxui::hbox(
                ftxui::spinner(constants::ASYNC_DISPLAY_SPINNER_TYPE, step) 
                    | ftxui::color(ftxui::Color::Gold1), 
                ftxui::separator(), 
                ftxui::text(msg)
            ) | ftxui::bgcolor(ftxui::Color::DarkRed)};
            ftxui::Render(spinnerScreen, spinner);
            term::forceCursorRow(ftxui::Terminal::Size().dimy);
            spinnerScreen.Print();
            std::this_thread::sleep_for(constants::ASYNC_DISPLAY_SPINNER_INTERVAL);
            ++step;
        }
        DLOG_F(INFO, "Spinner thread stopped.");
    }};
}

void ui::stopAsyncSpinner() {
    spinning = false;
    spinnerThread.join();
}

}

#include <iostream>
#include <string>
#include <tuple>

#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component.hpp"

#include "loguru.hpp"

#include "constants.hpp"
#include "terminal.hpp"
#include "security.hpp"
#include "setup.hpp"

namespace instruct {
    void configureLogging(int, char **);
    void logVersion();
    std::tuple<bool, int> setupMenu();
    bool instructSetup(std::string);
}

int main(int argc, char **argv) {
    
    instruct::configureLogging(argc, argv);
    
    LOG_F(INFO, "Instruct launched. Timestamps are recorded in local time.");
    
    instruct::logVersion();
    
    if (instruct::setup::setupIncomplete()) {
        LOG_F(INFO, "Set up incomplete. Starting setup.");
        auto [setupComplete, setupCode] {instruct::setupMenu()};
        if (!setupComplete) {
            LOG_F(INFO, "Exiting set up (incomplete).");
            return setupCode;
        }
    }
    
    // Check if another instance is already running.
    // auto appScreen {ftxui::ScreenInteractive::Fullscreen()};
    // auto app {ftxui::Renderer()}
    // Also reset appScreen cursor manually.
    // appScreen.Exit();
    // appScreen.Loop();
    
    LOG_F(INFO, "Exiting.");
    
    return 0;
}

void instruct::configureLogging(int argc, char **argv) {
    using namespace std::string_literals;

    loguru::g_stderr_verbosity = loguru::Verbosity_OFF;
    loguru::g_preamble_uptime = false;
    loguru::g_preamble_thread = false;
    loguru::g_preamble_file = false;
    // loguru::init(argc, argv);
    
    loguru::add_file(
        instruct::constants::LOG_DIR.c_str(), 
        loguru::Truncate, 
        argc == 2 && argv[1] == "-v"s ? loguru::Verbosity_MAX : loguru::Verbosity_INFO
    );
}

void instruct::logVersion() {
    std::string ver;
    #ifdef INSTRUCT_VERSION
    ver = #INSTRUCT_VERSION;
    #else
    ver = "Unknown";
    #endif
    LOG_F(INFO, "Instruct Version: %s", ver.c_str());
}

std::tuple<bool, int> instruct::setupMenu() {
    auto setupScreen {ftxui::ScreenInteractive::Fullscreen()};

    std::string instructPswd {};
    
    bool setupSuccess {false};
    auto continueSetup {[&] {
        LOG_F(INFO, "Continuing set up.");
        setupScreen.WithRestoredIO([&] {
            setupSuccess = instruct::instructSetup(instructPswd);
        })();
        // Exit must be scheduled without restored I/O.
        setupScreen.Exit();
    }};
    
    bool setupStop {false};
    auto cancelSetup {[&] {
        LOG_F(INFO, "Canceling setup.");
        setupStop = true;
        setupScreen.Exit();
    }};

    ftxui::Element h1 {ftxui::text("Instruct Set Up")};
    ftxui::Element h2 {ftxui::text("Please enter an instructor password: ")};

    ftxui::InputOption promptOptions {ftxui::InputOption::Default()};
    promptOptions.content = &instructPswd;
    promptOptions.password = true;
    promptOptions.multiline = false;
    promptOptions.on_enter = continueSetup;
    promptOptions.transform = instruct::constants::INPUT_TRANSFORM_CUSTOM;
    ftxui::Component instructPswdPrompt {ftxui::Input(promptOptions)};
    instructPswdPrompt |= ftxui::CatchEvent([&] (ftxui::Event event) {
        return event.is_character() 
            && instructPswd.length() > instruct::constants::MAX_INSTRUCTOR_PASSWORD_LENGTH;
    });

    ftxui::ButtonOption confirmOptions {ftxui::ButtonOption::Ascii()};
    confirmOptions.label = "Confirm";
    confirmOptions.on_click = continueSetup;
    confirmOptions.transform = [] (ftxui::EntryState state) {
        return ftxui::ButtonOption::Ascii().transform(state) 
            | ftxui::hcenter 
            | ftxui::color(ftxui::Color::GreenYellow);
    };
    ftxui::Component confirmButton {ftxui::Button(confirmOptions)};

    ftxui::ButtonOption cancelOptions {ftxui::ButtonOption::Ascii()};
    cancelOptions.label = "Cancel";
    cancelOptions.on_click = cancelSetup;
    cancelOptions.transform = [] (ftxui::EntryState state) {
        return ftxui::ButtonOption::Ascii().transform(state) 
            | ftxui::hcenter 
            | ftxui::color(ftxui::Color::Red);
    };
    ftxui::Component cancelButton {ftxui::Button(cancelOptions)};
    ftxui::Component setup {
        ftxui::Renderer(
            ftxui::Container::Vertical({
                instructPswdPrompt, 
                ftxui::Container::Horizontal({
                    confirmButton, cancelButton
                })
            }), 
            [&] {
                return ftxui::vbox(
                    h1 | ftxui::bold | ftxui::hcenter, 
                    h2, 
                    ftxui::window(ftxui::text("Password"), instructPswdPrompt->Render()), 
                    ftxui::hbox(
                        confirmButton->Render() | ftxui::border | ftxui::flex, 
                        cancelButton->Render() | ftxui::border | ftxui::flex
                    )
                ) | ftxui::center;
            }
        )
    };
    setup |= ftxui::CatchEvent([&] (ftxui::Event event) {
        if (event == ftxui::Event::Escape) {
            cancelSetup();
        }
        return false;
    });
    setupScreen.Loop(setup);
    if (setupStop) {
        return std::make_tuple(false, 0);
    }
    if (!setupSuccess) {
        return std::make_tuple(false, 1);
    }
    return std::make_tuple(true, 0);
}

bool instruct::instructSetup(std::string instructPswd) {
    instruct::term::enableAlternateScreenBuffer();
    
    auto nestedSetupScreen {ftxui::Screen::Create(ftxui::Dimension::Full())};

    ftxui::Elements progress {};
    auto displayProgress {[&] (std::string entry, bool errMsg = false) {
        VLOG_F(errMsg ? loguru::Verbosity_ERROR : 1, entry.c_str());
        progress.push_back(
            ftxui::paragraph(entry) 
            | (errMsg ? ftxui::color(ftxui::Color::Red) : ftxui::color(ftxui::Color::Default))
        );
        ftxui::Render(
            nestedSetupScreen, 
            ftxui::window(ftxui::text("Setting up..."), ftxui::vbox(progress))
        );
        nestedSetupScreen.Print();
    }};
    auto handleStep {[&] (bool stepSuccess) {
        if (!stepSuccess) {
            const instruct::setup::SetupError &setupError {instruct::setup::getSetupError()};
            std::string setupErrorStr {
                "An error occurred: " 
                + (setupError.errCode.value() != 0 ? 
                    "Error Code: " 
                    + std::to_string(setupError.errCode.value()) 
                    + " \"" + setupError.errCode.message() 
                    + "\" " + setupError.errCode.category().name()
                    : "No error code.")
                + (!setupError.exMsg.empty() ? 
                    " | Exception: " + setupError.exMsg
                    : " | No exception.")
                + " | Message: " + setupError.msg
            };
            instruct::term::disableAlternateScreenBuffer();
            std::cerr << setupErrorStr << '\n';
            instruct::term::enableAlternateScreenBuffer();
            displayProgress(setupErrorStr, true);
            displayProgress("Press [Enter] to exit.");
            std::cin.get();
            return false;
        }
        return true;
    }};
    auto setupFail {[&] {
        displayProgress("Deleting data directory.");
        instruct::setup::deleteDataDir();
    }};
    
    displayProgress(
        "Creating data directory `" + std::string {instruct::constants::DATA_DIR} + "`."
    );
    if (!handleStep(instruct::setup::createDataDir())) {
        return false;
    }
    
    displayProgress("Populating data directory.");
    if (!handleStep(instruct::setup::populateDataDir())) {
        setupFail();
        return false;
    }
    
    displayProgress("Setting defaults.");
    if (!handleStep(instruct::setup::setDefaults())) {
        setupFail();
        return false;
    }
    
    displayProgress("Setting instructor password: " 
        + instructPswd + " length " + std::to_string(instructPswd.length()));
    if (!handleStep(instruct::sec::updateInstructPswd(instructPswd))) {
        setupFail();
        return false;
    }
    
    displayProgress("Press [Enter] to complete set up.");
    std::cin.get();
    
    instruct::term::disableAlternateScreenBuffer();
    return true;
}

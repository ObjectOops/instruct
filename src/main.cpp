#include <filesystem>
#include <exception>
#include <iostream>
#include <string>
#include <vector>
#include <tuple>

#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component.hpp"

#include "loguru.hpp"

#include "notification.hpp"
#include "constants.hpp"
#include "terminal.hpp"
#include "security.hpp"
#include "setup.hpp"
#include "data.hpp"

namespace instruct {
    void configureLogging(int, char **);
    void logVersion();
    std::tuple<bool, int> setupMenu();
    bool instructSetup(std::string);
    bool initAllHandled();
    bool mainMenu();
    bool saveAllHandled();
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
    
    LOG_F(1, "Reading data.");
    if (!instruct::initAllHandled()) {
        LOG_F(INFO, "Exiting.");
        return EXIT_FAILURE;
    }
    if (instruct::sec::instanceActive()) {
        LOG_F(INFO, "Already running. Exiting.");
        std::cout << "Detected an instance of instruct already running.\n";
        return EXIT_FAILURE;
    }
    instruct::sec::createInstance();
    LOG_F(1, "Instance locked.");
    
    LOG_F(INFO, "Starting main application");
    instruct::mainMenu();
    
    LOG_F(INFO, "Finalizing save data.");
    if (!instruct::saveAllHandled()) {
        LOG_F(INFO, "Exiting.");
        return EXIT_FAILURE;
    }
    
    LOG_F(INFO, "Exiting main application.");
    
    return EXIT_SUCCESS;
}

void instruct::configureLogging(int argc, char **argv) {
    using namespace std::string_literals;

    loguru::g_stderr_verbosity = loguru::Verbosity_OFF;
    loguru::g_preamble_uptime = false;
    loguru::g_preamble_thread = false;
    loguru::g_preamble_file = false;
    // loguru::init(argc, argv);
    
    loguru::add_file(
        instruct::constants::INSTRUCT_LOG_DIR.c_str(), 
        loguru::Truncate, 
        argc == 2 && argv[1] == "-v"s ? loguru::Verbosity_MAX : loguru::Verbosity_INFO
    );
}

void instruct::logVersion() {
    LOG_F(INFO, "Instruct Version: %s", instruct::constants::INSTRUCT_VERSION.c_str());
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

    ftxui::Component confirmButton {ftxui::Button(
        "Confirm", continueSetup, ftxui::ButtonOption::Ascii()
    )};
    ftxui::Component cancelButton {ftxui::Button(
        "Cancel", cancelSetup, ftxui::ButtonOption::Ascii()
    )};
    
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
                        confirmButton->Render() 
                            | ftxui::color(ftxui::Color::GreenYellow) 
                            | ftxui::hcenter 
                            | ftxui::border 
                            | ftxui::flex, 
                        cancelButton->Render() 
                            | ftxui::color(ftxui::Color::Red) 
                            | ftxui::hcenter 
                            | ftxui::border 
                            | ftxui::flex
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
        return std::make_tuple(false, EXIT_SUCCESS);
    }
    if (!setupSuccess) {
        return std::make_tuple(false, EXIT_FAILURE);
    }
    return std::make_tuple(true, EXIT_SUCCESS);
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
                + (setupError.errCode.value() != 0 
                    ? "Error Code: " 
                    + std::to_string(setupError.errCode.value()) 
                    + " \"" + setupError.errCode.message() 
                    + "\" " + setupError.errCode.category().name()
                    : "No error code.")
                + (!setupError.exMsg.empty() 
                    ? " | Exception: " + setupError.exMsg
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
    
    instruct::Data::initEmpty();
    
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

bool instruct::initAllHandled() {
    try {
        instruct::Data::initAll();
    } catch (const std::exception &e) {
        try {
            std::filesystem::rename(
                instruct::constants::DATA_DIR, 
                std::string {instruct::constants::DATA_DIR} + "_copy"
            );
            std::string msg {R"(Possible data corruption detected.
To avoid a loss of information, restart the instruct setup.
Then, manually resolve your data from `)" 
+ std::string {instruct::constants::DATA_DIR} + R"(_copy`.
See the log file for more details.)"};
            LOG_F(ERROR, msg.c_str());
            LOG_F(ERROR, e.what());
            std::cerr << msg << '\n';
        } catch (const std::exception &e2) {
            std::string msg {R"(Possible data corruption detected.
To avoid a loss of information, backup `)" + std::string {instruct::constants::DATA_DIR} 
+ R"(` and restart the instruct setup.
Then, manually resolve your data from the backup.
See the log file for more details.)"};
            LOG_F(ERROR, msg.c_str());
            LOG_F(ERROR, e.what());
            std::cerr << msg << '\n';
        }
        return false;
    }
    return true;
}

bool instruct::saveAllHandled() {
    try {
        instruct::Data::saveAll();
    } catch (const std::exception &e) {
        std::string msg {R"(Some of your data failed to save.
Please manually resolve your data from `)" 
+ std::string {instruct::constants::DATA_DIR} + R"(`.
See the log file for more details.)"};
        LOG_F(ERROR, msg.c_str());
        LOG_F(ERROR, e.what());
        std::cerr << msg << '\n';
        return false;
    }
    return true;
}

bool instruct::mainMenu() {
    auto appScreen {ftxui::ScreenInteractive::Fullscreen()};
    
    if (instruct::IData::instructorData->firstTime) {
        instruct::notif::setNotification(R"(Welcome to instruct.

Before you begin, you'll need to select a verison of OpenVsCode Server to install.
Afterwards, you can import your list of students.

This message will only show once.)");
    }

    // These are button labels which need to change dynamically.
    struct {
        const char *icblStart {"Instructor (Start)"};
        const char *icblStop {"Instructor (Stop)"};
        const char *scblStart {"Student (Start All)"};
        const char *scblStop {"Student (Stop All)"};
        const char *ratbl {"Run All Tests"};
        const char *satbl {"Stop All Tests"};
    } dynamicLabels;
    std::string instructorCodeButtonLabel {dynamicLabels.icblStart};
    std::string studentCodeButtonLabel {dynamicLabels.scblStart};
    std::string runAllTestsButtonLabel {dynamicLabels.ratbl};
    // ---------------------------------------------------------
    
    struct TitleBarButtonContents {
        ftxui::ConstStringRef label;
        ftxui::Closure on_click;
    };
    struct TitleBarMenuContents {
        std::string label;
        std::vector<TitleBarButtonContents> options;
    };
    std::vector<TitleBarMenuContents> titleBarMenuContents {
        {
            "Code", {
                {
                    instructorCodeButtonLabel, 
                    [&] {}
                }, 
                {
                    studentCodeButtonLabel, 
                    [&] {}
                }
            }
        }, 
        {
            "Tools", {
                {
                    "Add File", 
                    [] {}
                }, 
                {
                    "Remove File", 
                    [] {}
                }, 
                {
                    "Add Student", 
                    [] {}
                }, 
                {
                    "Remove Student", 
                    [] {}
                }
            }
        }, 
        {
            "Tests", {
                {
                    runAllTestsButtonLabel, 
                    [&] {}
                }, 
                {
                    "Add Test", 
                    [] {}
                }, 
                {
                    "Remove Test", 
                    [] {}
                }
            }, 
        }, 
        {
            "Set Up", {
                {
                    "Import Students List", 
                    [] {}
                }, 
                {
                    "Export Students List", 
                    [] {}
                }, 
                {
                    "Install OpenVsCode Server", 
                    [] {}
                }
            }
        }
    };
    ftxui::Components titleBarMenus {};
    for (auto &menuContents : titleBarMenuContents) {
        ftxui::Components buttons {};
        for (auto &buttonContents : menuContents.options) {
            buttons.push_back(ftxui::Button(
                buttonContents.label, buttonContents.on_click, ftxui::ButtonOption::Ascii()
            ));
        }
        titleBarMenus.push_back(
            ftxui::Collapsible(menuContents.label, ftxui::Container::Vertical(buttons))
        );
    }
    struct {
        int problemCount {};
        ftxui::Component renderer {ftxui::Renderer([&] {
            return ftxui::hbox({
                (problemCount == 0
                    ? ftxui::text("Systems Operational") | ftxui::borderLight 
                    : ftxui::text("Problems Encountered: " + std::to_string(problemCount)) 
                        | ftxui::color(ftxui::Color::Red) | ftxui::borderLight), 
                ftxui::text(instruct::constants::INSTRUCT_VERSION) | ftxui::borderLight
            });
        })};
    } titleBarStatus;
    ftxui::Component notificationsButton {ftxui::Button("◆", [] {
    }, ftxui::ButtonOption::Animated(ftxui::Color::Black, ftxui::Color::GreenYellow))};
    ftxui::Component settingsButton {ftxui::Button("Settings", [] {
    }, ftxui::ButtonOption::Ascii())};
    ftxui::Component exitButton {ftxui::Button(
        "Exit", appScreen.ExitLoopClosure(), ftxui::ButtonOption::Ascii()
    )};
    for (ftxui::Component &titleBarMenu : titleBarMenus) {
        titleBarMenu |= ftxui::border;
    }
    ftxui::Component titleBar {ftxui::Container::Horizontal({
        ftxui::Container::Horizontal(titleBarMenus), 
        titleBarStatus.renderer, 
        notificationsButton, 
        settingsButton | ftxui::border, 
        exitButton | ftxui::border
    })};
    
    // auto app {ftxui::Renderer()};
    appScreen.Loop(titleBar);
    // Also reset appScreen cursor manually.
    // Escape also brings up exit menu.
    
    return true;
}

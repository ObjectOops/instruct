#include <filesystem>
#include <exception>
#include <algorithm>
#include <iostream>
#include <vector>

#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component.hpp"

#include "loguru.hpp"

#include "notification.hpp"
#include "constants.hpp"
#include "terminal.hpp"
#include "security.hpp"
#include "setup.hpp"
#include "data.hpp"
#include "ui.hpp"

namespace instruct {

bool ui::initAllHandled() {
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

bool ui::saveAllHandled() {
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

void ui::print(const std::string &s) {
    std::cout << s;
}

static bool instructSetup(const std::string &);

std::tuple<bool, int> ui::setupMenu() {
    auto setupScreen {ftxui::ScreenInteractive::Fullscreen()};

    std::string instructPswd {};
    
    bool setupSuccess {false};
    auto continueSetup {[&] {
        LOG_F(INFO, "Continuing set up.");
        setupScreen.WithRestoredIO([&] {
            setupSuccess = instructSetup(instructPswd);
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
                            | ftxui::hcenter 
                            | ftxui::border 
                            | ftxui::flex 
                            | ftxui::color(ftxui::Color::GreenYellow), 
                        cancelButton->Render() 
                            | ftxui::hcenter 
                            | ftxui::border 
                            | ftxui::flex
                            | ftxui::color(ftxui::Color::Red) 
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

static bool instructSetup(const std::string &instructPswd) {
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
            ftxui::window(
                ftxui::text("Setting up..."), 
                ftxui::vbox(progress) | ftxui::focusPositionRelative(0, 1) | ftxui::frame
            )
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

namespace {
    struct TitleBarButtonContents {
        ftxui::ConstStringRef label;
        ftxui::Closure on_click;
    };
    struct TitleBarMenuContents {
        std::string label;
        std::vector<TitleBarButtonContents> options;
    };
}

static void createTitleBarMenus(
    std::vector<TitleBarMenuContents> &, ftxui::Components &, bool *
);
static void collapseInactiveTitleBarMenus(const int, bool *, int &);
static void renderTitleBarMenus(
    int, 
    const int, 
    std::vector<TitleBarMenuContents> &, 
    ftxui::Components &, 
    ftxui::Elements &, 
    bool *
);

bool ui::mainMenu() {
    auto appScreen {ftxui::ScreenInteractive::Fullscreen()};
    
    if (instruct::IData::instructorData->get_firstTime()) {
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
    
    // Data structure containing the titles of each title bar menu, 
    // along with the label of each button and their functions.
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

    // Create title bar menus.
    const int titleBarMenuCount {static_cast<int>(titleBarMenuContents.size())};
    ftxui::Components titleBarMenus {};
    bool titleBarMenusShown [titleBarMenuCount] {};
    createTitleBarMenus(titleBarMenuContents, titleBarMenus, titleBarMenusShown);

    // The status bar next to the title bar menus that contains 
    // the application status and version.
    struct {
        int problemCount {};
        ftxui::Component renderer {ftxui::Renderer([&] {
            return ftxui::hbox({
                (problemCount == 0
                    ? ftxui::text("Systems Operational") | ftxui::borderLight 
                    : ftxui::text("Problems Encountered: " + std::to_string(problemCount)) 
                        | ftxui::borderLight | ftxui::color(ftxui::Color::Red)), 
                ftxui::text(instruct::constants::INSTRUCT_VERSION) | ftxui::borderLight
            });
        })};
    } titleBarStatus;
    
    // Buttons on the right-most side of the title bar.
    ftxui::Component notificationsButton {ftxui::Button("◆", [] {
    }, ftxui::ButtonOption::Animated(ftxui::Color::Black, ftxui::Color::GreenYellow))};
    ftxui::Component settingsButton {ftxui::Button("Settings", [] {
    }, ftxui::ButtonOption::Ascii())};
    ftxui::Component exitButton {ftxui::Button(
        "Exit", appScreen.ExitLoopClosure(), ftxui::ButtonOption::Ascii()
    )};
    
    // The index of the last opened title bar menu.
    // -1 if there was no last opened menu.
    int lastTitleBarMenuIdx {-1};

    ftxui::Component titleBar {ftxui::Renderer(
        ftxui::Container::Horizontal({
            ftxui::Container::Horizontal(titleBarMenus), 
            titleBarStatus.renderer, 
            notificationsButton, 
            settingsButton, 
            exitButton
        }), 
        [&] {
            collapseInactiveTitleBarMenus(
                titleBarMenuCount, titleBarMenusShown, lastTitleBarMenuIdx
            );
            
            /*
            Collapsible Menu Rendering Nuances:
            1. FTXUI is not intended to render elements like this.
            2. We must render the menus in reverse order so they properly overlap.
            3. We must manually calculate the width of each menu when in its collapsed state.
            */
            ftxui::Elements e_titleBarMenus {};
            
            // The complete cell width that all title bar menus collectively occupy.
            int totalTitleBarMenuBuffer {};
            for (TitleBarMenuContents &titleBarMenuContent : titleBarMenuContents) {
                // +4 for unicode symbol and border.
                totalTitleBarMenuBuffer += titleBarMenuContent.label.length() + 4;
            }
            
            renderTitleBarMenus(
                totalTitleBarMenuBuffer, 
                titleBarMenuCount, 
                titleBarMenuContents, 
                titleBarMenus, 
                e_titleBarMenus, 
                titleBarMenusShown
            );
            
            return ftxui::hbox({
                ftxui::dbox({
                    ftxui::hbox({
                        ftxui::emptyElement() 
                            | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, totalTitleBarMenuBuffer), 
                        ftxui::vbox(titleBarStatus.renderer->Render(), ftxui::filler())
                    }), 
                    ftxui::dbox(e_titleBarMenus)
                }), 
                ftxui::filler(), 
                ftxui::vbox(
                    ftxui::hbox(
                        notificationsButton->Render(), 
                        settingsButton->Render() | ftxui::border, 
                        exitButton->Render() | ftxui::border | ftxui::color(ftxui::Color::Red)
                    ), 
                    ftxui::filler()
                )
            });
        }
    )};
    
    ftxui::Component app {ftxui::Renderer(titleBar, [&] {
        return ftxui::vbox(
            titleBar->Render(), 
            ftxui::filler()
        );
    })};

    appScreen.Loop(app);
    // Also reset appScreen cursor manually.
    // Escape also brings up exit menu.
    
    return true;
}

static void createTitleBarMenus(
    std::vector<TitleBarMenuContents> &titleBarMenuContents, ftxui::Components &titleBarMenus, bool *titleBarMenusShown
) {
    int titleBarMenuIdx {};
    for (auto &menuContents : titleBarMenuContents) {
        ftxui::Components buttons {};
        for (auto &buttonContents : menuContents.options) {
            buttons.push_back(ftxui::Button(
                buttonContents.label, buttonContents.on_click, ftxui::ButtonOption::Ascii()
            ));
        }
        titleBarMenus.push_back(
            ftxui::Collapsible(
                menuContents.label, 
                ftxui::Container::Vertical(buttons), 
                &titleBarMenusShown[titleBarMenuIdx]
            )
        );
        ++titleBarMenuIdx;
    }
}
static void collapseInactiveTitleBarMenus(
    const int titleBarMenuCount, bool *titleBarMenusShown, int &lastTitleBarMenuIdx
) {
    int newLastTitleBarMenuIdx {-1}, collapseTitleBarIdx {-1};
    for (
        int titleBarMenuShownIdx {}; 
        titleBarMenuShownIdx < titleBarMenuCount; 
        ++titleBarMenuShownIdx
    ) {
        if (titleBarMenusShown[titleBarMenuShownIdx] 
            && titleBarMenuShownIdx == lastTitleBarMenuIdx) {
            // This title bar menu will be collapsed 
            // since it is currently being shown and was 
            // the last one shown.
            collapseTitleBarIdx = titleBarMenuShownIdx;
        } else if (titleBarMenusShown[titleBarMenuShownIdx]) {
            // This title bar menu will be shown next 
            // since it wasn't the last one shown.
            newLastTitleBarMenuIdx = titleBarMenuShownIdx;
        }
    }
    if (newLastTitleBarMenuIdx != -1) {
        lastTitleBarMenuIdx = newLastTitleBarMenuIdx;
        if (collapseTitleBarIdx != -1) {
            titleBarMenusShown[collapseTitleBarIdx] = false;
        }
    }
}
static void renderTitleBarMenus(
    int totalTitleBarMenuBuffer, 
    const int titleBarMenuCount, 
    std::vector<TitleBarMenuContents> &titleBarMenuContents, 
    ftxui::Components &titleBarMenus, 
    ftxui::Elements &e_titleBarMenus, 
    bool *titleBarMenusShown
) {
    // Render the title bar menus in reverse order 
    // per stated nuances.
    int titleBarMenuBuffer {totalTitleBarMenuBuffer};
    for (
        int rTitleBarMenuIdx {titleBarMenuCount - 1}; 
        rTitleBarMenuIdx >= 0; 
        --rTitleBarMenuIdx
    ) {
        // +4 for unicode symbol and border.
        titleBarMenuBuffer -= titleBarMenuContents
            .at(rTitleBarMenuIdx).label.length() + 4;
        
        ftxui::Component &titleBarMenu {titleBarMenus.at(rTitleBarMenuIdx)};
        ftxui::Element e_titleBarMenu {titleBarMenu->Render() | ftxui::border};
        ftxui::Element e_titleBarMenuBuffer {
            ftxui::emptyElement() 
            | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, titleBarMenuBuffer)
        };
        TitleBarMenuContents &currTitleBarMenuContents {
            titleBarMenuContents.at(rTitleBarMenuIdx)
        };
        
        // Compute the width and height of a title bar menu 
        // so we can create a background of spaces to 
        // cover overlapping characters from other menus.
        int currTitleBarMenuContentsWidthHeight [2] {
            // +3 for left border and button edges.
            static_cast<int>(std::max(
                currTitleBarMenuContents.label.length(), 
                std::max_element(
                    currTitleBarMenuContents.options.begin(), 
                    currTitleBarMenuContents.options.end(), 
                    [] (
                        const TitleBarButtonContents &lhs, 
                        const TitleBarButtonContents &rhs
                    ) {
                        return lhs.label->length() < rhs.label->length();
                    }
                )->label->length()
            )) + 3, 
            // +3 for menu label and border.
            static_cast<int>(currTitleBarMenuContents.options.size() + 3)
        };
        
        // Create the background of spaces.
        ftxui::Elements titleBarMenuBackground {};
        for (int bgRow {}; bgRow < currTitleBarMenuContentsWidthHeight[1]; ++bgRow) {
            titleBarMenuBackground.push_back(ftxui::text(
                std::string (currTitleBarMenuContentsWidthHeight[0], ' ')
            ));
        }

        // Render.        
        e_titleBarMenus.push_back(
            ftxui::vbox(
                ftxui::hbox({
                    e_titleBarMenuBuffer, 
                    titleBarMenusShown[rTitleBarMenuIdx] 
                    ? ftxui::dbox(
                        ftxui::vbox(titleBarMenuBackground), 
                        e_titleBarMenu
                    )
                    : e_titleBarMenu
                }), 
                ftxui::filler()
            )
        );
    }
}

}

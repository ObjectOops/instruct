#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <exception>
#include <algorithm>
#include <iostream>
#include <typeinfo>
#include <vector>
#include <memory>

#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component.hpp"

#include "loguru.hpp"
#include "uuid.h"

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
            LOG_F(ERROR, typeid(e).name());
            LOG_F(ERROR, e.what());
            std::cerr << msg << '\n';
        } catch (const std::exception &e2) {
            std::string msg {R"(Possible data corruption detected.
To avoid a loss of information, backup `)" + std::string {instruct::constants::DATA_DIR} 
+ R"(` and restart the instruct setup.
Then, manually resolve your data from the backup.
See the log file for more details.)"};
            LOG_F(ERROR, msg.c_str());
            LOG_F(ERROR, typeid(e).name());
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
    
    ftxui::Component setup {ftxui::Renderer(
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
    )};
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
                + (!setupError.exType.empty() 
                    ? " | Exception: " + setupError.exType + " --> " + setupError.exMsg
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
    ftxui::Elements &
);

template<typename DataType>
static void createPaneBoxes(
    const std::unordered_map<uuids::uuid, DataType> &, 
    std::unordered_set<uuids::uuid> &, 
    bool *, 
    ftxui::Components &, 
    bool *, 
    int &, 
    bool
);

// This is a very large function. The main UI does a lot of things.
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
            return ftxui::hbox(
                (problemCount == 0
                    ? ftxui::text("Systems Operational") | ftxui::borderLight 
                    : ftxui::text("Problems Encountered: " + std::to_string(problemCount)) 
                        | ftxui::borderLight | ftxui::color(ftxui::Color::Red)), 
                ftxui::text(instruct::constants::INSTRUCT_VERSION) | ftxui::borderLight
            );
        })};
    } titleBarStatus;
    
    // Buttons on the right-most side of the title bar.
    ftxui::Component notificationsButton {ftxui::Button("◆", [] {
    }, ftxui::ButtonOption::Animated(ftxui::Color::Black, ftxui::Color::GreenYellow))};
    ftxui::Component settingsButton {ftxui::Button("Settings", [] {
    }, ftxui::ButtonOption::Ascii())};
    
    bool exitModalShown {false};
    ftxui::Component exitButton {ftxui::Button(
        "Exit", [&] {exitModalShown = true;}, ftxui::ButtonOption::Ascii()
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
                e_titleBarMenus
            );
            
            return ftxui::hbox(
                ftxui::dbox(
                    ftxui::hbox(
                        ftxui::emptyElement() 
                            | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, totalTitleBarMenuBuffer), 
                        ftxui::vbox(titleBarStatus.renderer->Render(), ftxui::filler())
                    ), 
                    ftxui::dbox(e_titleBarMenus)
                ), 
                ftxui::filler(), 
                ftxui::vbox(
                    ftxui::hbox(
                        notificationsButton->Render(), 
                        settingsButton->Render() | ftxui::border, 
                        exitButton->Render() | ftxui::border | ftxui::color(ftxui::Color::Red)
                    ), 
                    ftxui::filler()
                )
            );
        }
    )};

    // Create the student pane.
    const std::unordered_map<uuids::uuid, SData::Student> &studentMap {
        SData::studentsData->get_students()
    };
    const int studentCount {static_cast<int>(studentMap.size())};
    
    ftxui::Components studentBoxes {};
    studentBoxes.reserve(studentCount);
    std::unique_ptr<bool []> u_studentBoxStates {std::make_unique<bool []>(studentCount)};
    bool *p_studentBoxStates {u_studentBoxStates.get()};
    std::unordered_set<uuids::uuid> selectedStudentUUIDS {};
    
    createPaneBoxes(
        studentMap, 
        selectedStudentUUIDS, 
        p_studentBoxStates, 
        studentBoxes, 
        titleBarMenusShown, 
        lastTitleBarMenuIdx, 
        UData::uiData->get_alwaysShowStudentUUIDs()
    );
    
    ftxui::Component studentPane {studentBoxes.empty() 
        ? ftxui::Renderer([] {return ftxui::text("No students to display.");}) 
        : ftxui::Container::Vertical(studentBoxes)
    };
    
    // Create the test pane.
    const std::unordered_map<uuids::uuid, TData::TestCase> &testMap {
        TData::testsData->get_tests()
    };
    const int testCount {static_cast<int>(testMap.size())};
    
    ftxui::Components testBoxes {};
    testBoxes.reserve(testCount);
    std::unique_ptr<bool []> u_testBoxStates {std::make_unique<bool []>(testCount)};
    bool *p_testBoxStates {u_testBoxStates.get()};
        
    createPaneBoxes(
        testMap, 
        TData::testsData->get_selectedTestUUIDs(), 
        p_testBoxStates, 
        testBoxes, 
        titleBarMenusShown, 
        lastTitleBarMenuIdx, 
        UData::uiData->get_alwaysShowTestUUIDs()
    );
    
    ftxui::Component testPane {testBoxes.empty() 
        ? ftxui::Renderer([] {return ftxui::text("No tests to display.");}) 
        : ftxui::Container::Vertical(testBoxes)
    };
    
    // A separator between the student and test panes.
    ftxui::Component mainPanes {ftxui::ResizableSplitLeft(
        studentPane | ftxui::vscroll_indicator | ftxui::yframe, 
        testPane | ftxui::vscroll_indicator | ftxui::yframe, 
        &UData::uiData->get_studentPaneWidth()
    )};
    
    // Parent container for all components.
    ftxui::Component mainScreen {ftxui::Container::Vertical({titleBar, mainPanes})};
    
    // // Modals (exit confirmation, settings, notifications, title bar and pane functions, etc.).
    // ftxui::Component exitModal {[&] {
    //     ftxui::Component exitNegative {ftxui::Button(
    //         "No", [&] {exitModalShown = false;}, ftxui::ButtonOption::Ascii()
    //     )};
    //     ftxui::Component exitAffirmative {ftxui::Button(
    //         "Yes", appScreen.ExitLoopClosure(), ftxui::ButtonOption::Ascii()
    //     )};
    //     return ftxui::Renderer(
    //         ftxui::Container::Horizontal({exitNegative, exitAffirmative}), 
    //         [&] {
    //             return ftxui::vbox(
    //                 ftxui::text("Exit?") | ftxui::hcenter, 
    //                 exitNegative->Render(), exitAffirmative->Render()
    //             ) | ftxui::border;
    //         }
    //     );
    // }()};
    
    // mainScreen |= ftxui::Modal(exitModal, &exitModalShown);
    
    ftxui::Component app {ftxui::Renderer(
        mainScreen, 
        [&] {
        return ftxui::dbox(
            ftxui::vbox( // 3 --> height of title bar.
                ftxui::emptyElement() | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 3), 
                mainPanes->Render() | ftxui::flex
            ), 
            titleBar->Render()
        );
    })};

    appScreen.Loop(app);
    // Also reset appScreen cursor manually.
    // Escape also brings up exit menu.
    
    return true;
}

static void createTitleBarMenus(
    std::vector<TitleBarMenuContents> &titleBarMenuContents, 
    ftxui::Components &titleBarMenus, 
    bool *titleBarMenusShown
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
    } else if (lastTitleBarMenuIdx != -1 && !titleBarMenusShown[lastTitleBarMenuIdx]) {
        lastTitleBarMenuIdx = -1;
    }
}
static void renderTitleBarMenus(
    int totalTitleBarMenuBuffer, 
    const int titleBarMenuCount, 
    std::vector<TitleBarMenuContents> &titleBarMenuContents, 
    ftxui::Components &titleBarMenus, 
    ftxui::Elements &e_titleBarMenus
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

        // Render.        
        e_titleBarMenus.push_back(
            ftxui::vbox(
                ftxui::hbox(e_titleBarMenuBuffer, e_titleBarMenu | ftxui::clear_under), 
                ftxui::filler()
            )
        );
    }
}

template<typename DataType>
static void createPaneBoxes(
    const std::unordered_map<uuids::uuid, DataType> &dataMap, 
    std::unordered_set<uuids::uuid> &selectedDataUUIDS, 
    bool *p_dataBoxStates, 
    ftxui::Components &dataBoxes, 
    bool *titleBarMenusShown, 
    int &lastTitleBarMenuIdx, 
    bool showUUIDs
) {
    int dataBoxIdx {};
    std::vector<const DataType *> p_dataVec {};
    p_dataVec.reserve(dataMap.size());
    for (auto &[uuid, data] : dataMap) {
        p_dataVec.push_back(&data);
    }
    // Sort labels by lexicographical order.
    std::sort(
        p_dataVec.begin(), 
        p_dataVec.end(), 
        [] (const DataType *lhs, const DataType *rhs) {
            return lhs->displayName < rhs->displayName;
        }
    );
    for (const DataType *p_data : p_dataVec) {
        ftxui::CheckboxOption checkboxOption {ftxui::CheckboxOption::Simple()};
        // checkboxOption.checked = &p_dataBoxStates[dataBoxIdx];
        // checkboxOption.label = dataData.second.displayName;
        
        // Note that this lambda is capturing some variables by value!
        // Only add a UUID to the selected set if the state 
        // changed to true.
        checkboxOption.on_change = [=, &selectedDataUUIDS, &lastTitleBarMenuIdx] {
            // Close a title bar menu if open.
            if (lastTitleBarMenuIdx != -1) {
                titleBarMenusShown[lastTitleBarMenuIdx] = false;
                lastTitleBarMenuIdx = -1;
                // Suppress the student box's changed state.
                p_dataBoxStates[dataBoxIdx] = !p_dataBoxStates[dataBoxIdx];
            } else if (p_dataBoxStates[dataBoxIdx]) {
                selectedDataUUIDS.emplace(p_data->uuid);
            } else {
                selectedDataUUIDS.erase(p_data->uuid);
            }
        };
        
        checkboxOption.transform = [&, p_data] (const ftxui::EntryState &e) {
            bool titleBarMenusHidden {lastTitleBarMenuIdx == -1};
            ftxui::EntryState e2 {e};
            e2.focused = e2.focused && titleBarMenusHidden;
            ftxui::Element elem {ftxui::CheckboxOption::Simple().transform(e2)};
            if ((e.focused && titleBarMenusHidden) || e.state) {
                return ftxui::hbox(
                    elem, 
                    ftxui::text(showUUIDs ? " " + uuids::to_string(p_data->uuid) : "")
                ) | ftxui::bgcolor(ftxui::Color::GrayDark);
            }
            return elem;
        };
        
        // Ensure that the UI matches saved selected boxes.
        p_dataBoxStates[dataBoxIdx] = selectedDataUUIDS.count(p_data->uuid);
        
        // The library developer forgot to implement a constructor, 
        // so this is a workaround.
        dataBoxes.push_back(ftxui::Checkbox(
            p_data->displayName, 
            &p_dataBoxStates[dataBoxIdx], 
            checkboxOption
        ));
        ++dataBoxIdx;
    }
}

}

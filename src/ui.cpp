#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <exception>
#include <algorithm>
#include <iostream>
#include <typeinfo>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <cctype>

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

static std::thread asyncDisplaySpinner(const std::string &, std::atomic_bool &);

bool ui::initAllHandled() {
    try {
        term::enableAlternateScreenBuffer();
        std::atomic_bool spin {true};
        std::thread spinnerThread {asyncDisplaySpinner("Loading...", spin)};

        Data::initAll();

        spin = false;
        spinnerThread.join();
        term::disableAlternateScreenBuffer();
    } catch (const std::exception &e) {
        try {
            std::filesystem::rename(
                constants::DATA_DIR, 
                std::string {constants::DATA_DIR} + "_copy"
            );
            std::string msg {R"(Possible data corruption detected.
To avoid a loss of information, restart the instruct setup.
Then, manually resolve your data from `)" 
+ std::string {constants::DATA_DIR} + R"(_copy`.
See the log file for more details.)"};
            LOG_F(ERROR, msg.c_str());
            LOG_F(ERROR, typeid(e).name());
            LOG_F(ERROR, e.what());
            std::cerr << msg << '\n';
        } catch (const std::exception &e2) {
            std::string msg {R"(Possible data corruption detected.
To avoid a loss of information, backup `)" + std::string {constants::DATA_DIR} 
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
        Data::saveAll();
    } catch (const std::exception &e) {
        std::string msg {R"(Some of your data failed to save.
Please manually resolve your data from `)" 
+ std::string {constants::DATA_DIR} + R"(`.
See the log file for more details.)"};
        LOG_F(ERROR, msg.c_str());
        LOG_F(ERROR, typeid(e).name());
        LOG_F(ERROR, e.what());
        std::cerr << msg << '\n';
        return false;
    }
    return true;
}

void ui::print(const std::string &s) {
    std::cout << s;
}

static ftxui::Element inputTransformCustom(ftxui::InputState);
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
    promptOptions.transform = [] (ftxui::InputState state) {
        ftxui::Element elem {inputTransformCustom(state)};
        return elem |= ftxui::hcenter;
    };
    ftxui::Component instructPswdPrompt {ftxui::Input(promptOptions)};
    instructPswdPrompt |= ftxui::CatchEvent([&] (ftxui::Event event) {
        return event.is_character() 
            && instructPswd.length() > constants::MAX_INSTRUCTOR_PASSWORD_LENGTH;
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
    term::enableAlternateScreenBuffer();
    
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
            const setup::SetupError &setupError {setup::getSetupError()};
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
            term::disableAlternateScreenBuffer();
            std::cerr << setupErrorStr << '\n';
            term::enableAlternateScreenBuffer();
            displayProgress(setupErrorStr, true);
            displayProgress("Press [Enter] to exit.");
            std::cin.get();
            return false;
        }
        return true;
    }};
    auto setupFail {[&] {
        displayProgress("Deleting data directory.");
        setup::deleteDataDir();
    }};
    
    displayProgress(
        "Creating data directory `" + std::string {constants::DATA_DIR} + "`."
    );
    if (!handleStep(setup::createDataDir())) {
        return false;
    }
    
    displayProgress("Populating data directory.");
    if (!handleStep(setup::populateDataDir())) {
        setupFail();
        return false;
    }
    
    Data::initEmpty();
    
    displayProgress("Setting defaults.");
    if (!handleStep(setup::setDefaults())) {
        setupFail();
        return false;
    }
    
    displayProgress("Setting instructor password: " 
        + instructPswd + " length " + std::to_string(instructPswd.length()));
    if (!handleStep(sec::updateInstructPswd(instructPswd))) {
        setupFail();
        return false;
    }
    
    displayProgress("Press [Enter] to complete set up.");
    std::cin.get();
    
    term::disableAlternateScreenBuffer();
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
    const bool &
);

static ftxui::Dimensions getDimensions();

// This is a very large function. The main UI does a lot of things.
bool ui::mainMenu() {
    auto appScreen {ftxui::ScreenInteractive::Fullscreen()};
    bool exitState {true};
    
    if (IData::instructorData->get_firstTime()) {
        notif::setNotification(
            "Please select a version of OpenVsCode Server to install. "
            "Then, import a list of students. "
            "This message will only show once."
        );
        IData::instructorData->set_firstTime(false);
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
                ftxui::text(constants::INSTRUCT_VERSION) | ftxui::borderLight
            );
        })};
    } titleBarStatus;
    
    // Buttons on the right-most side of the title bar.
    bool recentNotifsModalShown {false};
    ftxui::Component notificationsButton {ftxui::Button("◆", [&] {
        recentNotifsModalShown = true;
    }, ftxui::ButtonOption::Animated(ftxui::Color::Black, ftxui::Color::GreenYellow))};

    bool settingsModalShown {false};
    ftxui::Component settingsButton {ftxui::Button(
        "Settings", [&] {settingsModalShown = true;}, ftxui::ButtonOption::Ascii()
    )};
    
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
                            | ftxui::size(
                                ftxui::WIDTH, ftxui::EQUAL, totalTitleBarMenuBuffer
                            ), 
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
    
    // Modals (exit confirmation, settings, notifications, title bar and pane functions, etc.).
    
    // Exit modal.
    ftxui::Component exitNegative {ftxui::Button(
        "No", [&] {exitModalShown = false;}, ftxui::ButtonOption::Ascii()
    )};
    ftxui::Component exitAffirmative {ftxui::Button("Yes", [&] {
        // Save remaining data if it wasn't already.
        // This includes information such as the selected tests and some UI settings.
        std::atomic_bool spin {true};
        std::thread spinnerThread {asyncDisplaySpinner("Saving all data...", spin)};

        LOG_F(INFO, "Finalizing save data.");
        exitState = instruct::ui::saveAllHandled();
        
        spin = false;
        spinnerThread.join();
        appScreen.Exit();
    }, ftxui::ButtonOption::Ascii())};
    ftxui::Component exitModal {ftxui::Renderer(
        ftxui::Container::Horizontal({exitNegative, exitAffirmative}), 
        [&] {
            return ftxui::vbox(
                ftxui::text("Exit?") | ftxui::bold | ftxui::hcenter, 
                ftxui::hbox(
                    exitNegative->Render() 
                        | ftxui::hcenter 
                        | ftxui::border 
                        | ftxui::flex 
                        | ftxui::color(ftxui::Color::Red),
                    exitAffirmative->Render() 
                        | ftxui::hcenter 
                        | ftxui::border 
                        | ftxui::flex 
                        | ftxui::color(ftxui::Color::GreenYellow)
                )
            ) 
                | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, getDimensions().dimx * 0.25) 
                | ftxui::border;
        }
    )};
    
    // Settings modal.
    auto onlyDigits {[] (ftxui::Event event) {
        return event.is_character() && !std::isdigit(event.character()[0]);
    }};
    auto makeInput {[] (
        std::string &content, std::string placeholder, ftxui::Closure on_enter = {}
    ) {
        ftxui::InputOption inputOptions;
        inputOptions.content = &content;
        inputOptions.multiline = false;
        inputOptions.on_enter = on_enter;
        inputOptions.placeholder = placeholder;
        inputOptions.transform = inputTransformCustom;
        return ftxui::Input(inputOptions);
    }};
    std::vector<std::string> onOffToggle {"Off", "On"};
    auto makeOnOffToggle([] (std::vector<std::string> &toggles, int &selection) {
        return ftxui::Toggle(&toggles, &selection);
    });
    
    // Instructor settings for host and port config.
    std::string iAuthHostContent;
    ftxui::Component iAuthHostInput {makeInput(iAuthHostContent, "i.e. 0.0.0.0")};
    std::string iAuthPortContent;
    ftxui::Component iAuthPortInput {makeInput(iAuthPortContent, "i.e. 8000")};
    iAuthPortInput |= ftxui::CatchEvent(onlyDigits);
    std::string iCodePortContent;
    ftxui::Component iCodePortInput {makeInput(iCodePortContent, "i.e. 3000")};
    iCodePortInput |= ftxui::CatchEvent(onlyDigits);

    // Student settings for host and port config.
    std::string sAuthHostContent;
    ftxui::Component sAuthHostInput {makeInput(sAuthHostContent, "i.e. 0.0.0.0")};
    std::string sAuthPortContent;
    ftxui::Component sAuthPortInput {makeInput(sAuthPortContent, "i.e. 8001")};
    sAuthPortInput |= ftxui::CatchEvent(onlyDigits);
    
    ftxui::MenuOption sCodePortMenuOptions {ftxui::MenuOption::Vertical()};
    std::vector<std::string> sCodePortVec {};
    int sCodePortSelected {};
    sCodePortMenuOptions.entries = &sCodePortVec;
    sCodePortMenuOptions.selected = &sCodePortSelected;
    std::string sCodePortContent {};
    ftxui::Component sCodePortInput;
    ftxui::Closure sAddCodePort {[&] {
        if (std::count(sCodePortVec.begin(), sCodePortVec.end(), sCodePortContent) == 0 
            && !sCodePortContent.empty()) {
            sCodePortVec.push_back(sCodePortContent);
            std::sort(sCodePortVec.begin(), sCodePortVec.end());
            sCodePortContent.clear();
            sCodePortInput->TakeFocus();
        } else {
            notif::setNotification("Port " + sCodePortContent + " already present.");
        }
    }};
    ftxui::Closure sRemoveCodePort {[&] {
        if (!sCodePortVec.empty()) {
            sCodePortVec.erase(sCodePortVec.begin() + sCodePortSelected);
        }
    }};
    sCodePortInput = makeInput(sCodePortContent, "Click here to add a port.", sAddCodePort);
    sCodePortInput |= ftxui::CatchEvent(onlyDigits);
    ftxui::Component sAddCodePortButton {ftxui::Button(
        "Add Code Port", sAddCodePort, ftxui::ButtonOption::Ascii()
    )};
    ftxui::Component sRemoveCodePortButton {ftxui::Button(
        "Remove Code Port", sRemoveCodePort, ftxui::ButtonOption::Ascii()
    )};
    ftxui::Component sCodePortMenu {ftxui::Menu(sCodePortMenuOptions)};
    
    std::pair<std::string, std::string> sCodePortRangeContent;
    ftxui::Component sCodePortRangeInput_lb {
        makeInput(sCodePortRangeContent.first, "lower bound inclusive (i.e. 3001)")
    };
    sCodePortRangeInput_lb |= ftxui::CatchEvent(onlyDigits);
    ftxui::Component sCodePortRangeInput_ub {
        makeInput(sCodePortRangeContent.second, "i.e. 4000")
    };
    sCodePortRangeInput_ub |= ftxui::CatchEvent(onlyDigits);
    
    int sUseRandomPortsSelection;
    ftxui::Component sUseRandomPortsToggle {
        makeOnOffToggle(onOffToggle, sUseRandomPortsSelection)
    };
    
    // Instruct UI settings.
    int alwaysShowStudentUUIDsSelection;
    ftxui::Component alwaysShowStudentUUIDsToggle {
        makeOnOffToggle(onOffToggle, alwaysShowStudentUUIDsSelection)
    };
    int alwaysShowTestUUIDsSelection;
    ftxui::Component alwaysShowTestUUIDsToggle {
        makeOnOffToggle(onOffToggle, alwaysShowTestUUIDsSelection)
    };
    
    auto resetValues {[&] {
        iAuthHostContent = IData::instructorData->get_authHost();
        iAuthPortContent = std::to_string(IData::instructorData->get_authPort());
        iCodePortContent = std::to_string(IData::instructorData->get_codePort());
        
        sAuthHostContent = SData::studentsData->get_authHost();
        sAuthPortContent = std::to_string(SData::studentsData->get_authPort());
        
        sCodePortVec.clear();
        const auto &sCodePortSet {SData::studentsData->get_codePorts()};
        for (int codePort : sCodePortSet) {
            sCodePortVec.push_back(std::to_string(codePort));
        }
        std::sort(sCodePortVec.begin(), sCodePortVec.end());
        
        sCodePortRangeContent.first = 
            std::to_string(SData::studentsData->get_codePortRange().first);
        sCodePortRangeContent.second = 
            std::to_string(SData::studentsData->get_codePortRange().second);
        
        sUseRandomPortsSelection = SData::studentsData->get_useRandomPorts();

        alwaysShowStudentUUIDsSelection = UData::uiData->get_alwaysShowStudentUUIDs();
        alwaysShowTestUUIDsSelection = UData::uiData->get_alwaysShowTestUUIDs();
    }};
    resetValues();
    
    ftxui::Component settingsCancelChangesButton {ftxui::Button("Cancel Changes", [&] {
        resetValues();
        settingsModalShown = false;
    }, ftxui::ButtonOption::Ascii())};
    ftxui::Component settingsSaveChangesButton {ftxui::Button("Save Changes", [&] {
        std::atomic_bool spin {true};
        std::thread spinnerThread {asyncDisplaySpinner("Saving changes...", spin)};

        IData::instructorData->set_authHost(iAuthHostContent);
        IData::instructorData->set_authPort(std::stoi(iAuthPortContent));
        IData::instructorData->set_codePort(std::stoi(iCodePortContent));
        
        SData::studentsData->set_authHost(sAuthHostContent);
        SData::studentsData->set_authPort(std::stoi(sAuthPortContent));

        std::set<int> sCodePortSet {};
        for (std::string &codePort : sCodePortVec) {
            sCodePortSet.insert(std::stoi(codePort));
        }
        SData::studentsData->set_codePorts(sCodePortSet);
        
        std::pair<int, int> i_sCodePortRangeContent {
            std::stoi(sCodePortRangeContent.first), std::stoi(sCodePortRangeContent.second)
        };
        SData::studentsData->set_codePortRange(i_sCodePortRangeContent);
        
        SData::studentsData->set_useRandomPorts(sUseRandomPortsSelection);
        
        UData::uiData->set_alwaysShowStudentUUIDs(alwaysShowStudentUUIDsSelection);
        UData::uiData->set_alwaysShowTestUUIDs(alwaysShowTestUUIDsSelection);
        
        resetValues(); // Possibly redudant.

        spin = false;
        spinnerThread.join();
    }, ftxui::ButtonOption::Ascii())};

    auto inputLine {[] (std::string label, ftxui::Component &component) {
        return ftxui::hbox(ftxui::text(label), component->Render());
    }};
    int settingsScrollPosition {};
    ftxui::Component settingsContainer;
    ftxui::Component settingsModal {ftxui::Renderer(
        settingsContainer = ftxui::Container::Vertical({
            iAuthHostInput, 
            iAuthPortInput, 
            iCodePortInput, 
            sAuthHostInput, 
            sAuthPortInput, 
            sCodePortInput, 
            ftxui::Container::Horizontal({
                sAddCodePortButton, 
                sRemoveCodePortButton
            }), 
            sCodePortMenu, 
            ftxui::Container::Horizontal({
                sCodePortRangeInput_lb, 
                sCodePortRangeInput_ub
            }), 
            sUseRandomPortsToggle, 
            alwaysShowStudentUUIDsToggle, 
            alwaysShowTestUUIDsToggle, 
            ftxui::Container::Horizontal({
                settingsCancelChangesButton, 
                settingsSaveChangesButton
            })
        }) | ftxui::CatchEvent([&] (ftxui::Event event) {
            ftxui::Mouse mouse {event.mouse()};
            if (event == ftxui::Event::ArrowUp 
                || mouse.button == ftxui::Mouse::Button::WheelUp
            ) {
                settingsScrollPosition = std::max(1, settingsScrollPosition - 1);
                settingsSaveChangesButton->TakeFocus();
                return true;
            }
            if (event == ftxui::Event::ArrowDown 
                || mouse.button == ftxui::Mouse::Button::WheelDown
            ) {
                // Dynamically approximate the total height encompassed 
                // by settings items.
                std::size_t items {settingsContainer->ChildAt(0)->ChildCount()};
                settingsScrollPosition = std::min(
                    static_cast<int>(items * 2 + sCodePortVec.size()), 
                    settingsScrollPosition + 1
                );
                settingsSaveChangesButton->TakeFocus();
                return true;
            }
            return false;
        }), 
        [&] {
            ftxui::Dimensions dims {getDimensions()};
            return ftxui::vbox(
                ftxui::vbox(
                    ftxui::text("Instructor Settings") | ftxui::bold | ftxui::underlined, 
                    inputLine("Instruct Host: ", iAuthHostInput), 
                    inputLine("Instruct Port: ", iAuthPortInput), 
                    inputLine("Code Ports: ", iCodePortInput), 
                    ftxui::separatorEmpty(), 
                    ftxui::text("Student Settings") | ftxui::bold | ftxui::underlined, 
                    inputLine("Instruct Host: ", sAuthHostInput), 
                    inputLine("Instruct Port: ", sAuthPortInput), 
                    ftxui::text("Code Port: "), 
                    sCodePortInput->Render(), 
                    ftxui::hbox(
                        sAddCodePortButton->Render() 
                            | ftxui::hcenter 
                            | ftxui::border 
                            | ftxui::color(ftxui::Color::GreenYellow), 
                        sRemoveCodePortButton->Render() 
                            | ftxui::hcenter 
                            | ftxui::border 
                            | ftxui::color(ftxui::Color::Red)
                    ), 
                    sCodePortMenu->Render() | ftxui::border, 
                    ftxui::text("Code Port Range: "), 
                    sCodePortRangeInput_lb->Render(), 
                    ftxui::text(" to "), 
                    sCodePortRangeInput_ub->Render(), 
                    inputLine("Use Random Ports: ", sUseRandomPortsToggle), 
                    ftxui::separatorEmpty(), 
                    ftxui::text("UI Settings") | ftxui::bold | ftxui::underlined, 
                    inputLine(
                        "Always Show Student UUIDs: ", alwaysShowStudentUUIDsToggle
                    ), 
                    inputLine("Always Show Test UUIDs: ", alwaysShowTestUUIDsToggle)
                ) 
                    | ftxui::focusPosition(1, settingsScrollPosition) 
                    | ftxui::vscroll_indicator 
                    | ftxui::yframe 
                    | ftxui::flex_shrink, 
                ftxui::separator(), 
                ftxui::hbox(
                    settingsCancelChangesButton->Render() 
                        | ftxui::hcenter 
                        | ftxui::border 
                        | ftxui::color(ftxui::Color::Red), 
                    settingsSaveChangesButton->Render() 
                        | ftxui::hcenter 
                        | ftxui::border 
                        | ftxui::color(ftxui::Color::GreenYellow)
                )
            )
                | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, dims.dimx * 0.75) 
                | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, dims.dimy * 0.75) 
                | ftxui::border;
        }
    )};
    
    // Notifications modal.
    ftxui::Component closeNotifButton {ftxui::Button(
        "Close", notif::ackNotice, ftxui::ButtonOption::Ascii()
    )};
    ftxui::Component notifModal {ftxui::Renderer(
        closeNotifButton, 
        [&] {
            return ftxui::vbox(
                ftxui::paragraph(notif::getNotification()), 
                ftxui::separator(), 
                closeNotifButton->Render() 
                    | ftxui::hcenter 
                    | ftxui::border 
                    | ftxui::color(ftxui::Color::Blue)
            ) 
                | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, getDimensions().dimx * 0.75) 
                | ftxui::border;
        }
    )};
    
    ftxui::Component closeRecentNotifsButton {ftxui::Button(
        "Close", [&] {recentNotifsModalShown = false;}, ftxui::ButtonOption::Ascii()
    )};
    ftxui::MenuOption recentNotifsMenuOptions {ftxui::MenuOption::VerticalAnimated()};
    recentNotifsMenuOptions.entries = &notif::getRecentNotifications();
    recentNotifsMenuOptions.on_enter = notif::copyNotice;
    ftxui::Component recentNotifsMenu {ftxui::Menu(recentNotifsMenuOptions)};
    ftxui::Component recentNotifsModal {ftxui::Renderer(
        ftxui::Container::Vertical({recentNotifsMenu, closeRecentNotifsButton}), 
        [&] {
            ftxui::Dimensions dims {getDimensions()};
            return ftxui::vbox(
                ftxui::text("Click on a notification and press [Enter] to view it."), 
                ftxui::separator(), 
                recentNotifsMenu->Render() 
                    | ftxui::vscroll_indicator 
                    | ftxui::yframe 
                    | ftxui::flex, 
                ftxui::separator(), 
                closeRecentNotifsButton->Render() 
                    | ftxui::hcenter 
                    | ftxui::border 
                    | ftxui::color(ftxui::Color::Blue)
            ) 
                | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, dims.dimx * 0.75) 
                | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, dims.dimy * 0.75) 
                | ftxui::border;
        }
    )};
    
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
        }
    )};
    
    app |= ftxui::Modal(exitModal, &exitModalShown);
    app |= ftxui::Modal(settingsModal, &settingsModalShown);
    app |= ftxui::Modal(recentNotifsModal, &recentNotifsModalShown);
    app |= ftxui::Modal(notifModal, &notif::getNotice());

    appScreen.Loop(app);

    return exitState;
    // Also reset appScreen cursor manually.
    // Escape also brings up exit menu.
}

static std::thread asyncDisplaySpinner(const std::string &msg, std::atomic_bool &spin) {
    // Make a copy first to avoid a race condition.
    std::string msgCopy {msg};
    return std::thread {[msgCopy, &spin] {
        DLOG_F(INFO, "Spinner thread started.");
        auto spinnerScreen {ftxui::Screen::Create({ftxui::Terminal::Size().dimx, 1})};
        unsigned long long step {};
        while (spin) {
            ftxui::Element spinner {ftxui::hbox(
                ftxui::spinner(constants::ASYNC_DISPLAY_SPINNER_TYPE, step) 
                    | ftxui::color(ftxui::Color::Gold1), 
                ftxui::separator(), 
                ftxui::text(msgCopy)
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

static ftxui::Element inputTransformCustom(ftxui::InputState state) {
    state.element |= ftxui::color(ftxui::Color::White);
    if (state.is_placeholder) {
        state.element |= ftxui::dim;
    }
    if (!state.focused && state.hovered) {
        state.element |= ftxui::bgcolor(ftxui::Color::GrayDark);
    }
    return state.element;
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
    const bool &alwaysShowUUIDs
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
                selectedDataUUIDS.insert(p_data->uuid);
            } else {
                selectedDataUUIDS.erase(p_data->uuid);
            }
        };
        
        checkboxOption.transform = [&, p_data] (const ftxui::EntryState &e) {
            bool titleBarMenusHidden {lastTitleBarMenuIdx == -1};
            ftxui::EntryState e2 {e};
            e2.focused = e2.focused && titleBarMenusHidden;
            ftxui::Element elem {ftxui::CheckboxOption::Simple().transform(e2)};
            ftxui::Element sepElem {ftxui::text("|")};
            ftxui::Element uuidElem {
                ftxui::text(uuids::to_string(p_data->uuid)) | ftxui::flex_shrink
            };
            if ((e.focused && titleBarMenusHidden) || e.state) {
                return ftxui::hbox(elem, sepElem, uuidElem) 
                    | ftxui::bgcolor(ftxui::Color::GrayDark);
            }
            return alwaysShowUUIDs ? ftxui::hbox(elem, sepElem, uuidElem) : elem;
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

static ftxui::Dimensions getDimensions() {
    return ftxui::Terminal::Size();
}

}

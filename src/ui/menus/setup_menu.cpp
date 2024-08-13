#include "../ui.hpp"

#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component.hpp"
#include "loguru.hpp"

#include "../util/terminal.hpp"
#include "../../constants.hpp"
#include "../../security.hpp"
#include "../util/input.hpp"
#include "../../setup.hpp"
#include "../../data.hpp"

namespace instruct {

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
    ui::enableAlternateScreenBuffer();
    
    auto nestedSetupScreen {ftxui::Screen::Create(ftxui::Dimension::Full())};

    ftxui::Elements progress {};
    auto displayProgress {[&] (std::string entry, bool errMsg = false) {
        VLOG_F(errMsg ? loguru::Verbosity_ERROR : 1, "%s", entry.c_str());
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
            ui::disableAlternateScreenBuffer();
            std::cerr << setupErrorStr << '\n';
            ui::enableAlternateScreenBuffer();
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
    
    displayProgress("Locating system CA certificates.");
    if (!handleStep(setup::locateCACerts())) {
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
    
    ui::disableAlternateScreenBuffer();
    return true;
}

}

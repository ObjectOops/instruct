#include <iostream>
#include <string>

#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component.hpp"

#include "constants.hpp"
#include "terminal.hpp"
#include "security.hpp"
#include "setup.hpp"

namespace Instruct {
    bool instructSetup(std::string);
}

int main() {
    
    // auto appScreen {ftxui::ScreenInteractive::Fullscreen()};
    
    if (Instruct::Setup::setupIncomplete()) {
        auto setupScreen {ftxui::ScreenInteractive::Fullscreen()};
        
        ftxui::Element h1 {ftxui::text("Instruct Set Up")};
        ftxui::Element h2 {ftxui::text("Please enter an instructor password: ")};
        
        std::string instructPswd {};
        bool setupSuccess {false};
        auto continueSetup {[&] {
            setupScreen.WithRestoredIO([&] {
                setupSuccess = Instruct::instructSetup(instructPswd);
            })();
            // Exit must be scheduled without restored I/O.
            setupScreen.Exit();
        }};
        ftxui::InputOption promptOptions {ftxui::InputOption::Default()};
        promptOptions.content = &instructPswd;
        promptOptions.password = true;
        promptOptions.multiline = false;
        promptOptions.on_enter = continueSetup;
        promptOptions.transform = [&] (ftxui::InputState state) {
            state.element |= ftxui::color(ftxui::Color::White);
            if (state.is_placeholder) {
                state.element |= ftxui::dim;
            }
            if (!state.focused && state.hovered) {
                state.element |= ftxui::bgcolor(ftxui::Color::GrayDark);
            }
            return state.element;
        };
        ftxui::Component instructPswdPrompt {ftxui::Input(promptOptions)};
        instructPswdPrompt |= ftxui::CatchEvent([&] (ftxui::Event event) {
            return event.is_character() 
                && instructPswd.length() > Instruct::Constants::MAX_INSTRUCTOR_PASSWORD_LENGTH;
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
        
        bool cancelSetup {false};
        ftxui::ButtonOption cancelOptions {ftxui::ButtonOption::Ascii()};
        cancelOptions.label = "Cancel";
        cancelOptions.on_click = [&] {
            cancelSetup = true;
            setupScreen.Exit();
        };
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
                setupScreen.Exit();
            }
            return false;
        });
        setupScreen.Loop(setup);
        if (cancelSetup) {
            return 0;
        }
        if (!setupSuccess) {
            return 1;
        }
    }
    
    
    // auto app {ftxui::Renderer()}
    
    // Do namespace things once setup has been cleared.
        /*
        Also scroll the setup output.
        */
       // Set up screen in separate function?
    // Also reset appScreen cursor manually.
    // Check if another instance is already running.
    // appScreen.Exit();
    // appScreen.Loop();
    
    return 0;
}

bool Instruct::instructSetup(std::string instructPswd) {
    Instruct::Terminal::enableAlternateScreenBuffer();
    
    auto nestedSetupScreen {ftxui::Screen::Create(ftxui::Dimension::Full())};

    ftxui::Elements progress {};
    auto displayProgress {[&] (std::string entry, bool errMsg = false) {
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
            const Instruct::Setup::SetupError &setupError {Instruct::Setup::getSetupError()};
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
            Instruct::Terminal::disableAlternateScreenBuffer();
            std::cerr << setupErrorStr << '\n';
            Instruct::Terminal::enableAlternateScreenBuffer();
            displayProgress(setupErrorStr, true);
            displayProgress("Press [Enter] to exit.");
            std::cin.get();
            return false;
        }
        return true;
    }};
    auto setupFail {[&] {
        displayProgress("Deleting data directory.");
        Instruct::Setup::deleteDataDir();
    }};
    
    displayProgress(
        "Creating data directory `" + std::string {Instruct::Constants::DATA_DIR} + "`."
    );
    if (!handleStep(Instruct::Setup::createDataDir())) {
        return false;
    }
    
    displayProgress("Populating data directory.");
    if (!handleStep(Instruct::Setup::populateDataDir())) {
        setupFail();
        return false;
    }
    
    displayProgress("Setting defaults.");
    if (!handleStep(Instruct::Setup::setDefaults())) {
        setupFail();
        return false;
    }
    
    displayProgress("Setting instructor password: " 
        + instructPswd + " length " + std::to_string(instructPswd.length()));
    if (!handleStep(Instruct::Security::updateInstructPswd(instructPswd))) {
        setupFail();
        return false;
    }
    
    displayProgress("Press [Enter] to complete set up.");
    std::cin.get();
    
    Instruct::Terminal::disableAlternateScreenBuffer();
    return true;
}

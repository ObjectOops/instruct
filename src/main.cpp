#include <unistd.h>
#include <iostream>
#include <string>

#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component.hpp"

#include "constants.hpp"
#include "terminal.hpp"
#include "setup.hpp"

bool instructSetup(std::string);

int main() {
    
    // auto appScreen {ftxui::ScreenInteractive::Fullscreen()};
    
    if (/*!dataDirExists()*/true) {
        auto setupScreen {ftxui::ScreenInteractive::Fullscreen()};
        
        ftxui::Element h1 {ftxui::text("Instruct Set Up")};
        ftxui::Element h2 {ftxui::text("Please enter an instructor password: ")};
        
        std::string instructPswd {};
        bool setupSuccess {false};
        auto continueSetup {[&] {
            setupScreen.WithRestoredIO([&] {
                setupSuccess = instructSetup(instructPswd);
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
                    && instructPswd.length() > MAX_INSTRUCTOR_PASSWORD_LENGTH;
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
    
    // Also reset appScreen cursor manually.
    // appScreen.Exit();
    // appScreen.Loop();
    
    return 0;
}

bool instructSetup(std::string instructPswd) {
    enableAlternateScreenBuffer();
    
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
        usleep(500000);
    }};
    auto errStr {[] {
        return "An error occurred: Error Code " 
            + std::to_string(setupError.value()) 
            + " \"" + setupError.message() + "\" " 
            + setupError.category().name();
    }};
    auto handleStep {[&] (bool stepSuccess) {
        if (!stepSuccess) {
            displayProgress(errStr(), true);
            displayProgress("Press [Enter] to exit. More details may be provided on termination.");
            std::cin.get();
            return false;
        }
        return true;
    }};
    
    displayProgress("Creating data directory `" + std::string {DATA_DIR} + "`.");
    if (!handleStep(createDataDir())) {
        return false;
    }
    
    displayProgress("Populating data directory.");
    if (!handleStep(populateDataDir())) {
        displayProgress("Deleting data directory.");
        deleteDataDir();
        return false;
    }
    
    // displayProgress("Setting instructor password: " + instructPswd);
    // if (!handleStep(updateInstructPswd(instructPswd))) {
    //     displayProgress("Deleting data directory.");
    //     deleteDataDir();
    //     return false;
    // }
    (void)instructPswd;
    
    disableAlternateScreenBuffer();
    return true;
}

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

int main() {
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    auto renderer = ftxui::Renderer([] {
        return ftxui::text("Markdown Editor - Demo placeholder");
    });

    screen.Loop(renderer);
    return 0;
}

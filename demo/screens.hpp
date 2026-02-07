#pragma once

#include <string>
#include <vector>

#include <ftxui/component/component.hpp>

ftxui::Component make_editor_screen(
    int& current_screen, int& theme_index,
    std::vector<std::string>& theme_names);

ftxui::Component make_viewer_screen(
    int& current_screen, int& theme_index,
    std::vector<std::string>& theme_names);

ftxui::Component make_email_screen(
    int& current_screen, int& theme_index,
    std::vector<std::string>& theme_names);

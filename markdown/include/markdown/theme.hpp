#pragma once

#include <string>

#include <ftxui/dom/elements.hpp>

namespace markdown {

struct Theme {
    std::string name;
    // Editor syntax highlighting
    ftxui::Decorator syntax;
    ftxui::Decorator gutter;
    // Viewer elements
    ftxui::Decorator heading1;
    ftxui::Decorator heading2;
    ftxui::Decorator heading3;
    ftxui::Decorator link;
    ftxui::Decorator code_inline;
    ftxui::Decorator code_block;
    ftxui::Decorator blockquote;
};

inline Theme const& theme_default() {
    static Theme t{
        "Default",
        ftxui::color(ftxui::Color::Yellow) | ftxui::dim,
        ftxui::dim,
        ftxui::Decorator(ftxui::bold) | ftxui::underlined,
        ftxui::bold,
        ftxui::Decorator(ftxui::bold) | ftxui::dim,
        ftxui::color(ftxui::Color::Blue),
        ftxui::inverted,
        ftxui::nothing,
        ftxui::dim,
    };
    return t;
}

inline Theme const& theme_high_contrast() {
    static Theme t{
        "Contrast",
        ftxui::color(ftxui::Color::White) | ftxui::bold,
        ftxui::color(ftxui::Color::White),
        ftxui::Decorator(ftxui::bold) | ftxui::underlined,
        ftxui::bold,
        ftxui::bold,
        ftxui::color(ftxui::Color::CyanLight),
        ftxui::inverted,
        ftxui::nothing,
        ftxui::nothing,
    };
    return t;
}

inline Theme const& theme_colorful() {
    static Theme t{
        "Colorful",
        ftxui::color(ftxui::Color::Green),
        ftxui::color(ftxui::Color::Blue) | ftxui::dim,
        ftxui::Decorator(ftxui::bold) | ftxui::underlined
            | ftxui::color(ftxui::Color::Cyan),
        ftxui::Decorator(ftxui::bold) | ftxui::color(ftxui::Color::Green),
        ftxui::Decorator(ftxui::bold) | ftxui::color(ftxui::Color::Yellow),
        ftxui::color(ftxui::Color::BlueLight),
        ftxui::Decorator(ftxui::inverted) | ftxui::color(ftxui::Color::Yellow),
        ftxui::color(ftxui::Color::Green),
        ftxui::color(ftxui::Color::Blue),
    };
    return t;
}

} // namespace markdown

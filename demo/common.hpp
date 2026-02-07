#pragma once

#include <memory>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>
#include <ftxui/screen/screen.hpp>

#include "markdown/scroll_frame.hpp"
#include "markdown/theme.hpp"

namespace demo {

// Reports min_x=0 so that flex distributes hbox space equally,
// while passing through the full assigned width to the child.
class ZeroMinWidth : public ftxui::Node {
public:
    explicit ZeroMinWidth(ftxui::Element child)
        : ftxui::Node({std::move(child)}) {}
    void ComputeRequirement() override {
        children_[0]->ComputeRequirement();
        requirement_ = children_[0]->requirement();
        requirement_.min_x = 0;
    }
    void SetBox(ftxui::Box box) override {
        ftxui::Node::SetBox(box);
        children_[0]->SetBox(box);
    }
    void Render(ftxui::Screen& screen) override {
        children_[0]->Render(screen);
    }
};

inline ftxui::Element zero_min_width(ftxui::Element e) {
    return std::make_shared<ZeroMinWidth>(std::move(e));
}

// Returns the active Theme based on theme_index.
inline markdown::Theme const& get_theme(int index) {
    if (index == 1) return markdown::theme_high_contrast();
    if (index == 2) return markdown::theme_colorful();
    return markdown::theme_default();
}

// Minimal focusable component for wrapping renderers that need focus.
class FocusableBase : public ftxui::ComponentBase {
public:
    bool Focusable() const override { return true; }
};

// Theme toggle bar.
inline ftxui::Element theme_bar(ftxui::Component toggle) {
    return ftxui::hbox({
        ftxui::text("  Theme: "),
        toggle->Render(),
        ftxui::filler(),
    }) | ftxui::dim;
}

} // namespace demo

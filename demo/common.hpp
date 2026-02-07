#pragma once

#include <memory>
#include <string>
#include <vector>

#include <algorithm>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>
#include <ftxui/screen/screen.hpp>

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

// Direct-offset vertical scroll frame. Unlike yframe (which centers the
// focused element), this sets scroll offset = ratio * scrollable_height.
// Works with vscroll_indicator placed inside (between content and this frame).
class DirectScrollFrame : public ftxui::Node {
public:
    DirectScrollFrame(ftxui::Element child, float ratio)
        : Node({std::move(child)}), ratio_(ratio) {}

    void ComputeRequirement() override {
        children_[0]->ComputeRequirement();
        requirement_ = children_[0]->requirement();
    }

    void SetBox(ftxui::Box box) override {
        Node::SetBox(box);
        int external = box.y_max - box.y_min;
        int internal = std::max(children_[0]->requirement().min_y, external);
        int scrollable = std::max(0, internal - external - 1);
        int dy = static_cast<int>(ratio_ * static_cast<float>(scrollable));
        dy = std::max(0, std::min(scrollable, dy));

        ftxui::Box child_box = box;
        child_box.y_min = box.y_min - dy;
        child_box.y_max = box.y_min + internal - dy;
        children_[0]->SetBox(child_box);
    }

    void Render(ftxui::Screen& screen) override {
        auto stencil = screen.stencil;
        screen.stencil = ftxui::Box::Intersection(box_, screen.stencil);
        children_[0]->Render(screen);
        screen.stencil = stencil;
    }

private:
    float ratio_;
};

inline ftxui::Element direct_scroll(ftxui::Element child, float ratio) {
    return std::make_shared<DirectScrollFrame>(std::move(child), ratio);
}

// Theme toggle bar.
inline ftxui::Element theme_bar(ftxui::Component toggle) {
    return ftxui::hbox({
        ftxui::text("  Theme: "),
        toggle->Render(),
        ftxui::filler(),
    }) | ftxui::dim;
}

} // namespace demo

#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>
#include <ftxui/screen/screen.hpp>

#include "markdown/scroll_frame.hpp"
#include "markdown/theme.hpp"
#include "markdown/viewer.hpp"

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

// Theme toggle bar.
inline ftxui::Element theme_bar(ftxui::Component toggle) {
    return ftxui::hbox({
        ftxui::text("  Theme: "),
        toggle->Render(),
        ftxui::filler(),
    }) | ftxui::dim;
}

// Handle arrow/page/home/end scroll when viewer is inactive.
// Returns true if the event was consumed.
inline bool handle_inactive_scroll(markdown::Viewer& viewer,
                                    ftxui::Event const& ev,
                                    float step = 0.05f,
                                    float page_step = 0.3f) {
    if (viewer.active()) return false;
    auto adjust = [&](float delta) {
        viewer.set_scroll(std::clamp(viewer.scroll() + delta, 0.0f, 1.0f));
        return true;
    };
    if (ev == ftxui::Event::ArrowDown) return adjust(step);
    if (ev == ftxui::Event::ArrowUp) return adjust(-step);
    if (ev == ftxui::Event::PageDown) return adjust(page_step);
    if (ev == ftxui::Event::PageUp) return adjust(-page_step);
    if (ev == ftxui::Event::Home) return adjust(-viewer.scroll());
    if (ev == ftxui::Event::End) return adjust(1.0f - viewer.scroll());
    return false;
}

// Handle ArrowLeft/Right theme cycling.
// Returns true if the event was consumed.
inline bool handle_theme_cycling(int& theme_index, int count,
                                  ftxui::Event const& ev) {
    if (ev == ftxui::Event::ArrowLeft) {
        theme_index = (theme_index + count - 1) % count;
        return true;
    }
    if (ev == ftxui::Event::ArrowRight) {
        theme_index = (theme_index + 1) % count;
        return true;
    }
    return false;
}

} // namespace demo

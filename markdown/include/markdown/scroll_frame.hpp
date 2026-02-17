#pragma once

#include <algorithm>
#include <memory>

#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>
#include <ftxui/screen/screen.hpp>

namespace markdown {

/// Viewport/content dimensions filled by DirectScrollFrame during layout.
struct ScrollInfo {
    int viewport_height = 0;
    int content_height = 0;
    int viewport_y_min = 0;
};

// Direct-offset vertical scroll frame. Unlike yframe (which centers the
// focused element), this sets scroll offset = ratio * scrollable_height.
// Works with vscroll_indicator placed inside (between content and this frame).
class DirectScrollFrame : public ftxui::Node {
public:
    DirectScrollFrame(ftxui::Element child, float ratio,
                      ScrollInfo* info = nullptr)
        : Node({std::move(child)}), ratio_(ratio), info_(info) {}

    void ComputeRequirement() override {
        children_[0]->ComputeRequirement();
        requirement_ = children_[0]->requirement();
    }

    void SetBox(ftxui::Box box) override {
        Node::SetBox(box);
        int external = box.y_max - box.y_min;
        int internal = std::max(children_[0]->requirement().min_y, external);
        if (info_) {
            info_->viewport_height = external;
            info_->content_height = internal;
            info_->viewport_y_min = box.y_min;
        }
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
    ScrollInfo* info_ = nullptr;
};

inline ftxui::Element direct_scroll(ftxui::Element child, float ratio,
                                    ScrollInfo* info = nullptr) {
    return std::make_shared<DirectScrollFrame>(std::move(child), ratio, info);
}

} // namespace markdown

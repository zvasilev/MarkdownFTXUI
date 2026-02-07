#pragma once

#include <string>

#include <ftxui/component/component.hpp>

namespace markdown {

class Editor {
public:
    explicit Editor();

    ftxui::Component component();

    std::string const& content() const;

    void set_content(std::string text);

private:
    std::string content_;
};

} // namespace markdown

#pragma once

#include <string_view>

#include <ftxui/dom/elements.hpp>

namespace markdown {

ftxui::Element highlight_markdown_syntax(std::string_view text);

} // namespace markdown

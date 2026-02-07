#pragma once

#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>

namespace test_detail {
template <typename T>
auto printable(T const& v) {
    if constexpr (std::is_enum_v<T>) {
        return static_cast<std::underlying_type_t<T>>(v);
    } else {
        return v;
    }
}
} // namespace test_detail

#define ASSERT_TRUE(expr) \
    if (!(expr)) { \
        std::cerr << "FAIL: " #expr " at " << __FILE__ << ":" << __LINE__ << "\n"; \
        return 1; \
    }

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        std::cerr << "FAIL: " #a " != " #b " at " << __FILE__ << ":" << __LINE__ \
                  << "\n  actual:   " << test_detail::printable(a) \
                  << "\n  expected: " << test_detail::printable(b) << "\n"; \
        return 1; \
    }

#define ASSERT_CONTAINS(haystack, needle) \
    if (std::string(haystack).find(needle) == std::string::npos) { \
        std::cerr << "FAIL: \"" << needle << "\" not found at " << __FILE__ << ":" << __LINE__ << "\n"; \
        return 1; \
    }

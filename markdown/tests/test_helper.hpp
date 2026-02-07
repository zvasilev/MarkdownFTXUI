#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

#define ASSERT_TRUE(expr) \
    if (!(expr)) { \
        std::cerr << "FAIL: " #expr " at " << __FILE__ << ":" << __LINE__ << "\n"; \
        return 1; \
    }

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        std::cerr << "FAIL: " #a " != " #b " at " << __FILE__ << ":" << __LINE__ << "\n"; \
        return 1; \
    }

#define ASSERT_CONTAINS(haystack, needle) \
    if (std::string(haystack).find(needle) == std::string::npos) { \
        std::cerr << "FAIL: \"" << needle << "\" not found at " << __FILE__ << ":" << __LINE__ << "\n"; \
        return 1; \
    }

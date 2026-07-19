#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

// Tiny header-only test harness: no external dependencies, keeps CI fast.
namespace testing_lite {

inline int& failure_count() {
  static int count = 0;
  return count;
}

inline void Check(bool condition, const std::string& expr, const char* file, int line) {
  if (!condition) {
    ++failure_count();
    std::cerr << file << ":" << line << ": CHECK failed: " << expr << "\n";
  }
}

}  // namespace testing_lite

#define CHECK(expr) testing_lite::Check((expr), #expr, __FILE__, __LINE__)

#define TEST_MAIN_BEGIN() int main() {
#define TEST_MAIN_END()                                                 \
  if (testing_lite::failure_count() > 0) {                              \
    std::cerr << testing_lite::failure_count() << " check(s) failed\n"; \
    return EXIT_FAILURE;                                                \
  }                                                                     \
  std::cout << "All checks passed\n";                                   \
  return EXIT_SUCCESS;                                                  \
  }

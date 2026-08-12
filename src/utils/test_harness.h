#pragma once

#include <cstdlib>
#include <iostream>

#define TEST(test_case, test_name)                                       \
  void test_case##_##test_name();                                         \
  static int register_##test_case##_##test_name() {                      \
    std::cout << "[ RUN      ] " << #test_case << "." << #test_name      \
              << std::endl;                                               \
    test_case##_##test_name();                                           \
    std::cout << "[       OK ] " << #test_case << "." << #test_name      \
              << std::endl;                                               \
    return 0;                                                             \
  }                                                                       \
  static int dummy_##test_case##_##test_name =                           \
      register_##test_case##_##test_name();                               \
  void test_case##_##test_name()

#define EXPECT_TRUE(cond)                                                 \
  if (!(cond)) {                                                          \
    std::cerr << "Assertion failed: " #cond << " at " << __FILE__ << ":" \
              << __LINE__ << std::endl;                                   \
    std::exit(1);                                                         \
  }

#define EXPECT_FALSE(cond)                                                \
  if ((cond)) {                                                           \
    std::cerr << "Assertion failed: !(" #cond ") at " << __FILE__ << ":"  \
              << __LINE__ << std::endl;                                   \
    std::exit(1);                                                         \
  }

#define EXPECT_EQ(a, b)                                                   \
  if ((a) != (b)) {                                                       \
    std::cerr << "Assertion failed: " #a " == " #b << " at " << __FILE__  \
              << ":" << __LINE__ << std::endl;                            \
    std::exit(1);                                                         \
  }

int main() {
  std::cout << "[==========] Running tests." << std::endl;
  std::cout << "[==========] All tests passed." << std::endl;
  return 0;
}

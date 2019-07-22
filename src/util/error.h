#pragma once

#include <exception>

#include <util/string.h>

#define ERROR(...) throw std::runtime_error(::util::str(__VA_ARGS__))

#define ASSERT(cond)                                                      \
  do {                                                                    \
    if (!(cond)) {                                                        \
      ERROR("Assertion `" #cond "` failed at ", __FILE__, ":", __LINE__); \
    }                                                                     \
  } while (0)

#define ASSERTM(cond, ...)                    \
  do {                                        \
    if (!(cond)) {                            \
      ERROR(                                  \
          "Assertion `" #cond "` failed at ", \
          __FILE__,                           \
          ":",                                \
          __LINE__,                           \
          ", ",                               \
          ::util::str(__VA_ARGS__));          \
    }                                         \
  } while (0)

#define FAILM(...)                 \
  do {                             \
    ERROR(                         \
        "Failure at ",             \
        __FILE__,                  \
        ":",                       \
        __LINE__,                  \
        ", ",                      \
        ::util::str(__VA_ARGS__)); \
  } while (0)

#pragma once
#include <iostream> // IWYU pragma: keep
#include <cassert> // IWYU pragma: keep
#ifdef _DEBUG
#define XC_ASSERT(msg)                                                 \
    do {                                                            \
        if (!(msg)) {                                               \
            std::cout << "Assertion failed: " << #msg << std::endl; \
        }                                                           \
        assert(msg);                                                \
    } while (0)
#else
#define XC_ASSERT(msg)
#endif
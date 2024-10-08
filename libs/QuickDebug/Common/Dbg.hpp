#pragma once

#include <string>
#include <array>
#include <charconv>

// Insert function that prints the actual string
#define DEBUG_PRINT_NOARGS(x) std::cout << x

// Insert function that prints the actual string with parameters
#define DEBUG_PRINT(x, ...) std::cout << x << __VA_ARGS__

// Placeholder for strings (e.g. %s for printf, {} for spdlog)
#define PRINT_STRING "%s"

// Placeholder for ints (e.g. %d for printf, {} for spdlog)
#define PRINT_INT64 "%d"

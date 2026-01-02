#pragma once

#include <catch2/catch_test_macros.hpp>

// Helper macro to define a test case with a name and description - combines them into a single string with a dot
// separator. It is useful for organizing tests into categories in Visual Studio Test Explorer.
#define LUX_TEST_CASE(name, desc,  ...) \
	TEST_CASE(name "." desc, __VA_ARGS__)
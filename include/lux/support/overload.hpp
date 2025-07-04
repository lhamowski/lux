#pragma once

namespace lux {
	// A utility to create an overload set from multiple callable types
	template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
}
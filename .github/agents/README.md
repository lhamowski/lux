# GitHub Copilot Custom Agents

This directory contains custom agents (specialized AI models) for GitHub Copilot that are trained on the Lux project's specific conventions and patterns.

## Available Agents

### Test Generator (`test-generator.md`)

A specialized agent for generating high-quality unit tests that match the Lux project's testing conventions.

**Purpose**: Generate Catch2 v3 tests following the project's established patterns, naming conventions, and code style.

**Key Features**:
- Learns from existing test suite in `test/` directory
- Applies consistent test naming conventions
- Uses the project's `LUX_TEST_CASE` macro
- Follows C++23 code style (uniform initialization, const correctness, Allman braces)
- Generates tests for various component types (value types, I/O, async, mocks)
- Provides comprehensive coverage patterns

**When to Use**:
- Creating tests for new components
- Adding test coverage to existing components
- Ensuring tests match project conventions
- Learning the project's testing patterns

**Usage Example**:

When working in GitHub Copilot, you can reference the test generator agent to generate tests that perfectly match the project's style:

```
@workspace Generate unit tests for the buffer_reader component using the test-generator agent patterns
```

or when chatting with Copilot:

```
I need to write tests for my new tcp_client component. 
Please follow the test-generator agent conventions.
```

## How Custom Agents Work

GitHub Copilot can use these markdown files as specialized instructions for specific tasks. When you reference an agent:

1. Copilot reads the agent's instructions from the markdown file
2. It applies those instructions to understand your project's conventions
3. It generates code that matches your established patterns

This ensures consistency across the codebase and reduces the need for manual corrections.

## Project Conventions Summary

For quick reference, here are the key conventions enforced by our agents:

### Test Naming
- **Format**: `LUX_TEST_CASE("component", "description", "[tags]")`
- **Component**: Lowercase (e.g., `tcp_socket`, `buffer_reader`)
- **Description**: Present tense, specific behavior (e.g., "reads primitive types successfully")
- **Tags**: Hierarchical (e.g., `[io][net][tcp]`)

### Code Style
- **Indentation**: 4 spaces
- **Braces**: Allman style
- **Initialization**: Uniform `{}` (e.g., `int x{42};`)
- **Naming**: Private members end with `_`
- **Const**: Use for immutable variables and non-mutating methods

### Test Structure
```cpp
#include "test_case.hpp"
#include <lux/module/component.hpp>
#include <catch2/catch_all.hpp>

LUX_TEST_CASE("component", "behavior description", "[tags]")
{
    SECTION("Specific scenario")
    {
        // Setup
        // Action
        // Assertion using CHECK/REQUIRE
    }
}
```

## Contributing

When adding new agents:

1. Create a descriptive markdown file in this directory
2. Start with a clear purpose statement
3. Include comprehensive examples from the codebase
4. Document conventions, patterns, and anti-patterns
5. Update this README with information about the new agent

## References

- [GitHub Copilot Documentation](https://docs.github.com/en/copilot)
- [Lux Testing Guide](../../.github/copilot-instructions.md) - Main project instructions
- [Catch2 Documentation](https://github.com/catchorg/Catch2/tree/devel/docs)

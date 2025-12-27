#include <lux/utils/memory_arena.hpp>

#include <catch2/catch_all.hpp>

#include <vector>

TEST_CASE("memory_arena: growable arena", "[utils][memory_arena]")
{
    SECTION("growable_memory_arena basic functionality")
    {
        auto arena = lux::make_growable_memory_arena(3, 100); // initial size 3, each item reserves 100 bytes

        auto mem1 = arena->get(150);
        REQUIRE(mem1->capacity() == 150);
        REQUIRE(mem1->size() == 150);

        auto mem2 = arena->get(50);
        REQUIRE(mem2->capacity() == 100);
        REQUIRE(mem2->size() == 50);

        auto mem3 = arena->get(150);
        REQUIRE(mem3->capacity() == 150);
        REQUIRE(mem3->size() == 150);

        mem1.reset(); // Return to arena

        // Check that we reuse the memory
        auto mem4 = arena->get(50);
        REQUIRE(mem4->capacity() == 150); // Reused memory should have the same capacity as mem1, we don't shrink it
        REQUIRE(mem4->size() == 50);      // Size should be as requested
    }
}
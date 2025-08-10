#include <lux/utils/buffer_reader.hpp>
#include <lux/utils/buffer_writer.hpp>

#include <catch2/catch_all.hpp>

#include <array>
#include <string>
#include <string_view>
#include <vector>

TEST_CASE("buffer_reader basic functionality", "[utils][buffer_reader]")
{
    SECTION("Construction and basic properties")
    {
        std::array<std::byte, 100> buffer{};
        lux::buffer_reader reader{buffer};

        CHECK(reader.position() == 0);
        CHECK(reader.size() == 100);
        CHECK(reader.remaining() == 100);
        CHECK(reader.remaining_data().size() == 100);
    }

    SECTION("Reading primitive types")
    {
        // First write data using buffer_writer
        std::array<std::byte, 100> buffer{};
        lux::buffer_writer writer{buffer};
        writer << std::uint32_t{42} << float{3.14f} << double{2.71828};

        // Now read it back
        lux::buffer_reader reader{writer.written_data()};

        std::uint32_t uint_val;
        float float_val;
        double double_val;

        reader >> uint_val >> float_val >> double_val;

        CHECK(uint_val == 42);
        CHECK(float_val == Catch::Approx(3.14f));
        CHECK(double_val == Catch::Approx(2.71828));
        CHECK(reader.position() == sizeof(std::uint32_t) + sizeof(float) + sizeof(double));
        CHECK(reader.remaining() == 0);
    }

    SECTION("Reading primitive types with read() method")
    {
        std::array<std::byte, 100> buffer{};
        lux::buffer_writer writer{buffer};
        writer << std::uint32_t{42} << float{3.14f};

        lux::buffer_reader reader{writer.written_data()};

        std::uint32_t uint_val;
        float float_val;
        reader.read(uint_val).read(float_val);

        CHECK(uint_val == 42);
        CHECK(float_val == Catch::Approx(3.14f));
    }

    SECTION("Reading with T read() template")
    {
        std::array<std::byte, 100> buffer{};
        lux::buffer_writer writer{buffer};
        writer << std::uint32_t{42} << float{3.14f};

        lux::buffer_reader reader{writer.written_data()};

        auto uint_val = reader.read<std::uint32_t>();
        auto float_val = reader.read<float>();

        CHECK(uint_val == 42);
        CHECK(float_val == Catch::Approx(3.14f));
    }
}

TEST_CASE("buffer_reader arrays", "[utils][buffer_reader]")
{
    SECTION("Reading std::array with >> operator")
    {
        std::array<std::byte, 100> buffer{};
        lux::buffer_writer writer{buffer};

        std::array<int, 3> original = {1, 2, 3};
        writer << original;

        lux::buffer_reader reader{writer.written_data()};
        std::array<int, 3> read_array;
        reader >> read_array;

        CHECK(read_array[0] == 1);
        CHECK(read_array[1] == 2);
        CHECK(read_array[2] == 3);
        CHECK(reader.position() == sizeof(int) * 3);
    }

    SECTION("Reading std::array with read() method")
    {
        std::array<std::byte, 100> buffer{};
        lux::buffer_writer writer{buffer};

        std::array<float, 2> original = {1.5f, 2.5f};
        writer << original;

        lux::buffer_reader reader{writer.written_data()};
        std::array<float, 2> read_array;
        reader.read(read_array);

        CHECK(read_array[0] == Catch::Approx(1.5f));
        CHECK(read_array[1] == Catch::Approx(2.5f));
    }

    SECTION("Reading std::array with T read() template")
    {
        std::array<std::byte, 100> buffer{};
        lux::buffer_writer writer{buffer};

        std::array<double, 2> original = {4.5, 6.7};
        writer << original;

        lux::buffer_reader reader{writer.written_data()};
        auto read_array = reader.read<std::array<double, 2>>();

        CHECK(read_array[0] == Catch::Approx(4.5));
        CHECK(read_array[1] == Catch::Approx(6.7));
    }

    SECTION("Reading C-style arrays")
    {
        std::array<std::byte, 100> buffer{};
        lux::buffer_writer writer{buffer};

        int original[4] = {10, 20, 30, 40};
        writer << original;

        lux::buffer_reader reader{writer.written_data()};
        int read_array[4];
        reader.read(read_array);

        CHECK(read_array[0] == 10);
        CHECK(read_array[1] == 20);
        CHECK(read_array[2] == 30);
        CHECK(read_array[3] == 40);
    }
}

TEST_CASE("buffer_reader skip functionality", "[utils][buffer_reader]")
{
    SECTION("Skip advances position")
    {
        std::array<std::byte, 100> buffer{};
        lux::buffer_writer writer{buffer};
        writer << std::uint32_t{42} << std::uint32_t{123} << std::uint32_t{456};

        lux::buffer_reader reader{writer.written_data()};

        reader.skip(sizeof(std::uint32_t)); // Skip first value
        CHECK(reader.position() == sizeof(std::uint32_t));
        CHECK(reader.remaining() == sizeof(std::uint32_t) * 2);

        auto value = reader.read<std::uint32_t>();
        CHECK(value == 123); // Should read the second value
    }

    SECTION("Skip can be chained")
    {
        std::array<std::byte, 100> buffer{};
        lux::buffer_writer writer{buffer};
        writer << std::uint32_t{1} << std::uint32_t{2} << std::uint32_t{3};

        lux::buffer_reader reader{writer.written_data()};
        reader.skip(4).skip(4); // Skip 8 bytes total

        auto value = reader.read<std::uint32_t>();
        CHECK(value == 3); // Should read the third value
    }

    SECTION("Skip with read operations")
    {
        std::array<std::byte, 100> buffer{};
        lux::buffer_writer writer{buffer};

        // Write: header(4) + data1(2) + data2(2) + footer(4)
        writer << std::uint32_t{0xDEADBEEF} << std::uint16_t{123} << std::uint16_t{456} << std::uint32_t{0xFEEDFACE};

        lux::buffer_reader reader{writer.written_data()};

        // Skip header
        reader.skip(sizeof(std::uint32_t));

        // Read data
        auto data1 = reader.read<std::uint16_t>();
        auto data2 = reader.read<std::uint16_t>();

        // Read footer
        auto footer = reader.read<std::uint32_t>();

        CHECK(data1 == 123);
        CHECK(data2 == 456);
        CHECK(footer == 0xFEEDFACE);
    }
}

TEST_CASE("buffer_reader error handling", "[utils][buffer_reader]")
{
    SECTION("Buffer underflow throws exception")
    {
        std::array<std::byte, 10> small_buffer{};
        lux::buffer_writer writer{small_buffer};
        writer << std::uint32_t{42}; // Uses 4 bytes

        lux::buffer_reader reader{writer.written_data()};

        // This should succeed
        auto value = reader.read<std::uint32_t>();
        CHECK(value == 42);

        // This should fail - trying to read more data than available
        CHECK_THROWS_AS(reader.read<std::uint32_t>(), lux::formatted_exception);
    }

    SECTION("Skip underflow throws exception")
    {
        std::array<std::byte, 10> small_buffer{};
        lux::buffer_reader reader{small_buffer};

        CHECK_THROWS_AS(reader.skip(15), lux::formatted_exception);
    }

    SECTION("Array read underflow throws exception")
    {
        std::array<std::byte, 5> tiny_buffer{};
        lux::buffer_reader reader{tiny_buffer};

        std::array<int, 2> arr; // Needs 8 bytes, but only 5 available
        CHECK_THROWS_AS(reader.read(arr), lux::formatted_exception);
    }
}

TEST_CASE("buffer_reader remaining_data", "[utils][buffer_reader]")
{
    SECTION("remaining_data reflects current position")
    {
        std::array<std::byte, 100> buffer{};
        lux::buffer_writer writer{buffer};
        writer << std::uint32_t{1} << std::uint32_t{2} << std::uint32_t{3};

        lux::buffer_reader reader{writer.written_data()};

        CHECK(reader.remaining_data().size() == 12); // 3 * sizeof(uint32_t)

        reader.read<std::uint32_t>();
        CHECK(reader.remaining_data().size() == 8); // 2 * sizeof(uint32_t)

        reader.skip(4);
        CHECK(reader.remaining_data().size() == 4); // 1 * sizeof(uint32_t)
    }
}

namespace {

struct test_point
{
    float x, y, z;
};

lux::buffer_reader& operator>>(lux::buffer_reader& reader, test_point& p)
{
    float dummy;
    return reader >> p.x >> dummy >> p.y >> dummy >> p.z;
}

} // namespace

TEST_CASE("buffer_reader user-defined types", "[utils][buffer_reader]")
{
    SECTION("Reading user-defined types")
    {
        std::array<std::byte, 100> buffer{};
        lux::buffer_writer writer{buffer};

        test_point original{1.0f, 2.0f, 3.0f};
        writer << original; // Uses the writer's operator<<

        lux::buffer_reader reader{writer.written_data()};
        test_point read_point;
        reader >> read_point; // Uses the reader's operator>>

        CHECK(read_point.x == Catch::Approx(1.0f));
        CHECK(read_point.y == Catch::Approx(2.0f));
        CHECK(read_point.z == Catch::Approx(3.0f));
    }
}

TEST_CASE("buffer_reader round-trip compatibility", "[utils][buffer_reader]")
{
    SECTION("Write and read back various types")
    {
        std::array<std::byte, 1000> buffer{};
        lux::buffer_writer writer{buffer};

        // Write various types
        writer << std::uint8_t{255} << std::int32_t{-12345} << float{3.14159f} << double{2.718281828}
               << std::uint64_t{0xDEADBEEFCAFEBABE};

        std::array<int, 3> test_array = {100, 200, 300};
        writer << test_array;

        // Read back
        lux::buffer_reader reader{writer.written_data()};

        auto u8_val = reader.read<std::uint8_t>();
        auto i32_val = reader.read<std::int32_t>();
        auto f_val = reader.read<float>();
        auto d_val = reader.read<double>();
        auto u64_val = reader.read<std::uint64_t>();
        auto arr_val = reader.read<std::array<int, 3>>();

        CHECK(u8_val == 255);
        CHECK(i32_val == -12345);
        CHECK(f_val == Catch::Approx(3.14159f));
        CHECK(d_val == Catch::Approx(2.718281828));
        CHECK(u64_val == 0xDEADBEEFCAFEBABE);
        CHECK(arr_val[0] == 100);
        CHECK(arr_val[1] == 200);
        CHECK(arr_val[2] == 300);

        CHECK(reader.remaining() == 0);
    }
}
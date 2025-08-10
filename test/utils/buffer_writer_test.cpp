#include <lux/utils/buffer_writer.hpp>

#include <catch2/catch_all.hpp>

#include <array>
#include <string>
#include <string_view>
#include <vector>

TEST_CASE("buffer_writer basic functionality", "[utils][buffer_writer]")
{
    SECTION("Construction and basic properties")
    {
        std::array<std::byte, 100> buffer;
        lux::buffer_writer writer{buffer};

        CHECK(writer.position() == 0);
        CHECK(writer.size() == 100);
        CHECK(writer.remaining() == 100);
        CHECK(writer.written_data().empty());
    }

    SECTION("Writing primitive types")
    {
        std::array<std::byte, 100> buffer;
        lux::buffer_writer writer{buffer};

        writer << std::uint32_t{42} << float{3.14f} << double{2.71828};

        CHECK(writer.position() == sizeof(std::uint32_t) + sizeof(float) + sizeof(double));
        CHECK(writer.remaining() == 100 - writer.position());

        auto written = writer.written_data();
        CHECK(written.size() == writer.position());

        // Verify the written data
        auto uint_val = *reinterpret_cast<const std::uint32_t*>(written.data());
        auto float_val = *reinterpret_cast<const float*>(written.data() + sizeof(std::uint32_t));
        auto double_val = *reinterpret_cast<const double*>(written.data() + sizeof(std::uint32_t) + sizeof(float));

        CHECK(uint_val == 42);
        CHECK(float_val == Catch::Approx(3.14f));
        CHECK(double_val == Catch::Approx(2.71828));
    }

    SECTION("Writing strings")
    {
        std::array<std::byte, 100> buffer;
        lux::buffer_writer writer{buffer};

        std::string test_string = "Hello";
        std::string_view test_view = "World";

        writer << test_string << test_view << "C-style";

        auto written = writer.written_data();
        std::string result{reinterpret_cast<const char*>(written.data()), written.size()};
        CHECK(result == "HelloWorldC-style");
    }

    SECTION("Writing empty strings")
    {
        std::array<std::byte, 100> buffer;
        lux::buffer_writer writer{buffer};

        std::string empty_string;
        std::string_view empty_view;

        writer << empty_string << empty_view;

        CHECK(writer.position() == 0);
        CHECK(writer.written_data().empty());
    }
}

TEST_CASE("buffer_writer containers", "[utils][buffer_writer]")
{
    SECTION("Writing std::array")
    {
        std::array<std::byte, 100> buffer;
        lux::buffer_writer writer{buffer};

        std::array<int, 3> test_array = {1, 2, 3};
        writer << test_array;

        CHECK(writer.position() == sizeof(int) * 3);

        auto written = writer.written_data();
        auto data_ptr = reinterpret_cast<const int*>(written.data());
        CHECK(data_ptr[0] == 1);
        CHECK(data_ptr[1] == 2);
        CHECK(data_ptr[2] == 3);
    }

    SECTION("Writing std::vector")
    {
        std::array<std::byte, 100> buffer;
        lux::buffer_writer writer{buffer};

        std::vector<float> test_vector = {1.0f, 2.0f, 3.0f};
        writer << test_vector;

        CHECK(writer.position() == sizeof(float) * 3);

        auto written = writer.written_data();
        auto data_ptr = reinterpret_cast<const float*>(written.data());
        CHECK(data_ptr[0] == Catch::Approx(1.0f));
        CHECK(data_ptr[1] == Catch::Approx(2.0f));
        CHECK(data_ptr[2] == Catch::Approx(3.0f));
    }

    SECTION("Writing empty vector")
    {
        std::array<std::byte, 100> buffer;
        lux::buffer_writer writer{buffer};

        std::vector<int> empty_vector;
        writer << empty_vector;

        CHECK(writer.position() == 0);
        CHECK(writer.written_data().empty());
    }

    SECTION("Writing std::span")
    {
        std::array<std::byte, 100> buffer;
        lux::buffer_writer writer{buffer};

        std::array<double, 2> source = {4.5, 6.7};
        std::span<const double> test_span{source};
        writer << test_span;

        CHECK(writer.position() == sizeof(double) * 2);

        auto written = writer.written_data();
        auto data_ptr = reinterpret_cast<const double*>(written.data());
        CHECK(data_ptr[0] == Catch::Approx(4.5));
        CHECK(data_ptr[1] == Catch::Approx(6.7));
    }

    SECTION("Writing C-style array")
    {
        std::array<std::byte, 100> buffer;
        lux::buffer_writer writer{buffer};

        int c_array[4] = {10, 20, 30, 40};
        writer << c_array;

        CHECK(writer.position() == sizeof(int) * 4);

        auto written = writer.written_data();
        auto data_ptr = reinterpret_cast<const int*>(written.data());
        CHECK(data_ptr[0] == 10);
        CHECK(data_ptr[1] == 20);
        CHECK(data_ptr[2] == 30);
        CHECK(data_ptr[3] == 40);
    }
}

TEST_CASE("buffer_writer skip functionality", "[utils][buffer_writer]")
{
    SECTION("Skip advances position")
    {
        std::array<std::byte, 100> buffer;
        lux::buffer_writer writer{buffer};

        writer.skip(10);
        CHECK(writer.position() == 10);
        CHECK(writer.remaining() == 90);

        writer << std::uint32_t{42};
        CHECK(writer.position() == 14);
    }

    SECTION("Skip can be chained")
    {
        std::array<std::byte, 100> buffer;
        lux::buffer_writer writer{buffer};

        writer.skip(5).skip(3).skip(2);
        CHECK(writer.position() == 10);
    }

    SECTION("Skip with write operations")
    {
        std::array<std::byte, 100> buffer;
        lux::buffer_writer writer{buffer};

        // Write header placeholder
        auto header_pos = writer.position();
        writer.skip(sizeof(std::uint32_t));

        // Write data
        writer << std::uint16_t{123} << std::uint16_t{456};

        // Calculate and write header
        auto data_size = writer.position() - header_pos - sizeof(std::uint32_t);
        std::memcpy(buffer.data() + header_pos, &data_size, sizeof(std::uint32_t));

        auto written = writer.written_data();
        auto header_val = *reinterpret_cast<const std::uint32_t*>(written.data());
        CHECK(header_val == sizeof(std::uint16_t) * 2);
    }
}

TEST_CASE("buffer_writer error handling", "[utils][buffer_writer]")
{
    SECTION("Buffer overflow throws exception")
    {
        std::array<std::byte, 10> small_buffer;
        lux::buffer_writer writer{small_buffer};

        // This should succeed
        writer << std::uint32_t{42};

        // This should fail - trying to write 8 more bytes when only 6 remain
        CHECK_THROWS_AS(writer << double{3.14159}, lux::formatted_exception);
    }

    SECTION("Skip overflow throws exception")
    {
        std::array<std::byte, 10> small_buffer;
        lux::buffer_writer writer{small_buffer};

        CHECK_THROWS_AS(writer.skip(15), lux::formatted_exception);
    }

    SECTION("Large string overflow throws exception")
    {
        std::array<std::byte, 5> tiny_buffer;
        lux::buffer_writer writer{tiny_buffer};

        std::string large_string(10, 'x');
        CHECK_THROWS_AS(writer << large_string, lux::formatted_exception);
    }
}

namespace {

struct test_point
{
    float x, y, z;
};

lux::buffer_writer& operator<<(lux::buffer_writer& writer, const test_point& p)
{
    return writer << p.x << 0.5f << p.y << 0.5f << p.z;
}

} // namespace

TEST_CASE("buffer_writer user-defined types", "[utils][buffer_writer]")
{
    SECTION("Writing user-defined types")
    {
        std::array<std::byte, 100> buffer;
        lux::buffer_writer writer{buffer};

        test_point point{1.0f, 2.0f, 3.0f};
        writer << point;

        CHECK(writer.position() == sizeof(float) * 5);

        auto written = writer.written_data();
        auto data_ptr = reinterpret_cast<const float*>(written.data());
        CHECK(data_ptr[0] == Catch::Approx(1.0f));
        CHECK(data_ptr[1] == Catch::Approx(0.5f));
        CHECK(data_ptr[2] == Catch::Approx(2.0f));
        CHECK(data_ptr[3] == Catch::Approx(0.5f));
        CHECK(data_ptr[4] == Catch::Approx(3.0f));
    }
}
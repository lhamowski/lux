#include "test_case.hpp"

#include <lux/support/enum.hpp>

#include <catch2/catch_all.hpp>

LUX_TEST_CASE("enum", "converts enum to underlying type", "[enum][support]")
{
    enum class color : int
    {
        red = 1,
        green = 2,
        blue = 3
    };

    SECTION("Convert enum to underlying type")
    {
        CHECK(lux::to_underlying(color::red) == 1);
        CHECK(lux::to_underlying(color::green) == 2);
        CHECK(lux::to_underlying(color::blue) == 3);
    }

    SECTION("Underlying type is correct")
    {
        static_assert(std::is_same_v<decltype(lux::to_underlying(color::red)), int>);
    }
}

LUX_TEST_CASE("enum", "to_string_view conversion", "[enum][support]")
{
    // clang-format off
    enum class http_status
    {
        unknown = 0,
    
        continue_                       = 100,
        switching_protocols             = 101,
        processing                      = 102,
        early_hints                     = 103,
    
        ok                              = 200,
        created                         = 201,
        accepted                        = 202,
        non_authoritative_information   = 203,
        no_content                      = 204,
        reset_content                   = 205,
        partial_content                 = 206,
        multi_status                    = 207,
        already_reported                = 208,
        im_used                         = 226,
    
        multiple_choices                = 300,
        moved_permanently               = 301,
        found                           = 302,
        see_other                       = 303,
        not_modified                    = 304,
        use_proxy                       = 305,
        temporary_redirect              = 307,
        permanent_redirect              = 308,
    
        bad_request                     = 400,
        unauthorized                    = 401,
        payment_required                = 402,
        forbidden                       = 403,
        not_found                       = 404,
        method_not_allowed              = 405,
        not_acceptable                  = 406,
        proxy_authentication_required   = 407,
        request_timeout                 = 408,
        conflict                        = 409,
        gone                            = 410,
        length_required                 = 411,
        precondition_failed             = 412,
        payload_too_large               = 413,
        uri_too_long                    = 414,
        unsupported_media_type          = 415,
        range_not_satisfiable           = 416,
        expectation_failed              = 417,
        misdirected_request             = 421,
        unprocessable_entity            = 422,
        locked                          = 423,
        failed_dependency               = 424,
        too_early                       = 425,
        upgrade_required                = 426,
        precondition_required           = 428,
        too_many_requests               = 429,
        request_header_fields_too_large = 431,
        unavailable_for_legal_reasons   = 451,
    
        internal_server_error           = 500,
        not_implemented                 = 501,
        bad_gateway                     = 502,
        service_unavailable             = 503,
        gateway_timeout                 = 504,
        http_version_not_supported      = 505,
        variant_also_negotiates         = 506,
        insufficient_storage            = 507,
        loop_detected                   = 508,
        not_extended                    = 510,
        network_authentication_required = 511
    };
    // clang-format on

    SECTION("Convert enum to string_view")
    {
        CHECK(lux::to_string_view(http_status::ok) == "ok");
        CHECK(lux::to_string_view(http_status::not_found) == "not_found");
        CHECK(lux::to_string_view(http_status::network_authentication_required) == "network_authentication_required");
    }

    SECTION("Unknown enum value within supported range returns error marker")
    {
        CHECK(lux::to_string_view(static_cast<http_status>(999)) == "<error:unknown>");
    }

    SECTION("Enum value below supported range returns lower bound error")
    {
        CHECK(lux::to_string_view(static_cast<http_status>(-1)) == "<error:value is less than enum range minimum (0)>");
    }

    SECTION("Enum value above supported range returns upper bound error")
    {
        CHECK(lux::to_string_view(static_cast<http_status>(1025)) == "<error:value is greater than enum range maximum (1024)>");
    }

    SECTION("String representation is correct")
    {
        static_assert(std::is_same_v<decltype(lux::to_string_view(http_status::ok)), std::string_view>);
    }
}

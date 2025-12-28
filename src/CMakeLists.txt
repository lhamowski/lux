set(lux_include_files_dir ${lux_SOURCE_DIR}/include/lux)
set(lux_source_files_dir ${lux_SOURCE_DIR}/src/lux)

cflex_add_library(lux SOURCES 
	# Logger files
	${lux_include_files_dir}/logger/log_level.hpp ${lux_source_files_dir}/logger/log_level.cpp
	${lux_include_files_dir}/logger/logger.hpp
	${lux_include_files_dir}/logger/logger_manager.hpp ${lux_source_files_dir}/logger/logger_manager.cpp

	# Support files
	${lux_include_files_dir}/support/assert.hpp
	${lux_include_files_dir}/support/concepts.hpp
	${lux_include_files_dir}/support/container.hpp
	${lux_include_files_dir}/support/exception.hpp
	${lux_include_files_dir}/support/enum.hpp
	${lux_include_files_dir}/support/finally.hpp
	${lux_include_files_dir}/support/hash.hpp
	${lux_include_files_dir}/support/macros.hpp
	${lux_include_files_dir}/support/move.hpp
	${lux_include_files_dir}/support/overload.hpp
	${lux_include_files_dir}/support/path.hpp ${lux_source_files_dir}/support/path.cpp
	${lux_include_files_dir}/support/result.hpp
	${lux_include_files_dir}/support/scoped_value.hpp
	${lux_include_files_dir}/support/strong_typedef.hpp

	# Utils files
	${lux_include_files_dir}/utils/buffer_reader.hpp
	${lux_include_files_dir}/utils/buffer_writer.hpp
	${lux_include_files_dir}/utils/expiring_ref.hpp
	${lux_include_files_dir}/utils/memory_arena.hpp
	${lux_include_files_dir}/utils/lifetime_guard.hpp
	${lux_include_files_dir}/utils/stopwatch.hpp

	${lux_include_files_dir}/fwd.hpp
)
add_library(lux::lux ALIAS lux)
target_link_libraries(lux 
	PUBLIC 
		spdlog::spdlog
		magic_enum::magic_enum
	PRIVATE
		fmt::fmt
)

target_compile_features(lux PUBLIC cxx_std_23)
target_include_directories(lux 
	PUBLIC 
		${lux_SOURCE_DIR}/include
)
set_target_properties(lux PROPERTIES FOLDER lux)

lux_source_group(lux TREE ${lux_SOURCE_DIR})

if(LUX_ENABLE_IO)
    cflex_add_library(lux-io SOURCES 
		# io/net files
		${lux_include_files_dir}/io/net/base/address_v4.hpp
		${lux_include_files_dir}/io/net/base/endpoint.hpp
		${lux_include_files_dir}/io/net/base/http_factory.hpp
		${lux_include_files_dir}/io/net/base/http_method.hpp
		${lux_include_files_dir}/io/net/base/http_request.hpp
		${lux_include_files_dir}/io/net/base/http_response.hpp
		${lux_include_files_dir}/io/net/base/http_server.hpp
		${lux_include_files_dir}/io/net/base/http_status.hpp
		${lux_include_files_dir}/io/net/base/socket_config.hpp
		${lux_include_files_dir}/io/net/base/socket_factory.hpp
		${lux_include_files_dir}/io/net/base/ssl.hpp
		${lux_include_files_dir}/io/net/base/tcp_acceptor.hpp
		${lux_include_files_dir}/io/net/base/tcp_socket.hpp
		${lux_include_files_dir}/io/net/base/udp_socket.hpp

		${lux_source_files_dir}/io/net/asio_impl.cpp
		${lux_source_files_dir}/io/net/utils.hpp
		${lux_include_files_dir}/io/net/http_factory.hpp ${lux_source_files_dir}/io/net/http_factory.cpp
		${lux_include_files_dir}/io/net/http_server.hpp ${lux_source_files_dir}/io/net/http_server.cpp
		${lux_include_files_dir}/io/net/socket_factory.hpp ${lux_source_files_dir}/io/net/socket_factory.cpp
		${lux_include_files_dir}/io/net/tcp_acceptor.hpp ${lux_source_files_dir}/io/net/tcp_acceptor.cpp
		${lux_include_files_dir}/io/net/tcp_socket.hpp ${lux_source_files_dir}/io/net/tcp_socket.cpp
		${lux_include_files_dir}/io/net/tcp_inbound_socket.hpp ${lux_source_files_dir}/io/net/tcp_inbound_socket.cpp
		${lux_include_files_dir}/io/net/udp_socket.hpp ${lux_source_files_dir}/io/net/udp_socket.cpp

		# io/time files
		${lux_include_files_dir}/io/time/base/timer.hpp
		${lux_include_files_dir}/io/time/base/retry_policy.hpp

		${lux_include_files_dir}/io/time/interval_timer.hpp
		${lux_include_files_dir}/io/time/retry_executor.hpp ${lux_source_files_dir}/io/time/retry_executor.cpp
		${lux_include_files_dir}/io/time/timer_factory.hpp

		# io/coro files
		${lux_include_files_dir}/io/coro/algorithms.hpp
		${lux_include_files_dir}/io/coro/awaitable_event.hpp
		${lux_include_files_dir}/io/coro/common.hpp

		# general io files
		${lux_include_files_dir}/io/promise.hpp
    )
	add_library(lux::io ALIAS lux-io)

	if(TARGET Boost::disable_autolinking)
		target_link_libraries(lux-io PRIVATE Boost::disable_autolinking)
	endif()

    target_link_libraries(lux-io
		PUBLIC
			lux::lux
			ssl
			crypto
			Boost::boost
    )

	target_include_directories(lux-io
		PRIVATE
			${lux_SOURCE_DIR}/src
	)

	target_compile_definitions(lux-io
		PUBLIC
			BOOST_ASIO_SEPARATE_COMPILATION
	)

    lux_source_group(lux TREE ${lux_SOURCE_DIR})
endif()
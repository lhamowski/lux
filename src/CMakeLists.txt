set(lux_include_files_dir ${lux_SOURCE_DIR}/include/lux)
set(lux_source_files_dir ${lux_SOURCE_DIR}/src/lux)

cflex_add_library(lux SOURCES 
	# Logger files
	${lux_include_files_dir}/logger/log_level.hpp ${lux_source_files_dir}/logger/log_level.cpp
	${lux_include_files_dir}/logger/logger.hpp
	${lux_include_files_dir}/logger/logger_manager.hpp ${lux_source_files_dir}/logger/logger_manager.cpp

	# Support files
	${lux_include_files_dir}/support/assert.hpp
	${lux_include_files_dir}/support/move.hpp
	${lux_include_files_dir}/support/overload.hpp
	${lux_include_files_dir}/support/path.hpp ${lux_source_files_dir}/support/path.cpp
	${lux_include_files_dir}/support/strong_typedef.hpp

	# Utils files
	${lux_include_files_dir}/utils/memory_arena.hpp
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

target_compile_features(lux PUBLIC cxx_std_20)
target_include_directories(lux 
	PUBLIC 
		${lux_SOURCE_DIR}/include
)
set_target_properties(lux PROPERTIES FOLDER lux)

lux_source_group(lux TREE ${lux_SOURCE_DIR})

if(LUX_ENABLE_IO)
    cflex_add_library(lux-io SOURCES 
		${lux_include_files_dir}/io/net/base/address_v4.hpp
		${lux_include_files_dir}/io/net/base/endpoint.hpp
		${lux_include_files_dir}/io/net/base/socket_factory.hpp
		${lux_include_files_dir}/io/net/base/udp_socket.hpp
		${lux_include_files_dir}/io/net/base/tcp_socket.hpp
		${lux_include_files_dir}/io/net/base/tcp_acceptor.hpp

		${lux_include_files_dir}/io/time/interval_timer.hpp
		${lux_include_files_dir}/io/time/delayed_retry_executor.hpp ${lux_source_files_dir}/io/time/delayed_retry_executor.cpp
		${lux_include_files_dir}/io/time/timer_factory.hpp

		${lux_source_files_dir}/io/net/utils.hpp
		${lux_include_files_dir}/io/net/socket_factory.hpp ${lux_source_files_dir}/io/net/socket_factory.cpp
        ${lux_include_files_dir}/io/net/tcp_acceptor.hpp ${lux_source_files_dir}/io/net/tcp_acceptor.cpp
        ${lux_include_files_dir}/io/net/tcp_socket.hpp ${lux_source_files_dir}/io/net/tcp_socket.cpp
		${lux_include_files_dir}/io/net/udp_socket.hpp ${lux_source_files_dir}/io/net/udp_socket.cpp
    )
	add_library(lux::io ALIAS lux-io)

	if(TARGET Boost::disable_autolinking)
		target_link_libraries(lux-io PRIVATE Boost::disable_autolinking)
	endif()

    target_link_libraries(lux-io
		PUBLIC
			lux::lux
			Boost::boost
    )

	target_include_directories(lux-io
		PRIVATE
			${lux_SOURCE_DIR}/src
	)

    lux_source_group(lux TREE ${lux_SOURCE_DIR})
endif()
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
target_include_directories(lux PUBLIC ${lux_SOURCE_DIR}/include)
set_target_properties(lux PROPERTIES FOLDER lux)

lux_source_group(lux TREE ${lux_SOURCE_DIR})

if(LUX_ENABLE_NET)
    cflex_add_library(lux-net SOURCES 
		${lux_include_files_dir}/net/base/address_v4.hpp
		${lux_include_files_dir}/net/base/endpoint.hpp
		${lux_include_files_dir}/net/base/socket_factory.hpp
		${lux_include_files_dir}/net/base/udp_socket.hpp
		${lux_include_files_dir}/net/base/tcp_socket.hpp
		${lux_include_files_dir}/net/base/tcp_acceptor.hpp

        ${lux_include_files_dir}/net/udp_socket.hpp ${lux_source_files_dir}/net/udp_socket.cpp
		${lux_include_files_dir}/net/socket_factory.hpp ${lux_source_files_dir}/net/socket_factory.cpp
    )
	add_library(lux::net ALIAS lux-net)

	if(TARGET Boost::disable_autolinking)
		target_link_libraries(lux-net PRIVATE Boost::disable_autolinking)
	endif()

    target_link_libraries(lux-net 
		PUBLIC
			lux::lux
			Boost::boost
    )
    lux_source_group(lux TREE ${lux_SOURCE_DIR})
endif()
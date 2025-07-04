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

	${lux_include_files_dir}/fwd.hpp
)
add_library(lux::lux ALIAS lux)
target_link_libraries(lux PRIVATE fmt::fmt spdlog::spdlog)

target_compile_features(lux PUBLIC cxx_std_20)
target_include_directories(lux PUBLIC ${lux_SOURCE_DIR}/include)
set_target_properties(lux PROPERTIES FOLDER lux)

include(SourceGroup)
lux_source_group(lux TREE ${lux_SOURCE_DIR})

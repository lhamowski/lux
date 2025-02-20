cflex_add_library(lux SOURCES ${lux_SOURCE_DIR}/include/lux/lux.hpp lux.cpp)
add_library(lux::lux ALIAS lux)

target_compile_features(lux PUBLIC cxx_std_20)
target_include_directories(lux PUBLIC ${lux_SOURCE_DIR}/include)
set_target_properties(lux PROPERTIES FOLDER lux)

include(SourceGroup)
kl_source_group(lux TREE ${lux_SOURCE_DIR})

# LuxSourceGroup
#
# lux_source_group(<target> TREE <tree_root>)
#
# Creates source groups for the given target based on the file tree structure.
# This organizes files in IDEs like Visual Studio in a hierarchical folder structure.
#
# Arguments:
#   target    - The target to organize source files for
#   TREE      - The root directory to use as the base for the tree structure
#
# Example:
#   lux_source_group(my_target TREE ${CMAKE_SOURCE_DIR})

function(lux_source_group _target)
    cmake_parse_arguments(arg "" "TREE" "" ${ARGN})
    
    if(NOT arg_TREE)
        message(FATAL_ERROR "Required argument TREE not given for lux_source_group")
    endif()
    
    # Check if target exists
    if(NOT TARGET ${_target})
        message(FATAL_ERROR "Target '${_target}' does not exist")
    endif()
    
    # Get sources and check if any exist
    get_target_property(sources_list ${_target} SOURCES)
    if(NOT sources_list)
        message(WARNING "Target '${_target}' has no sources to group")
        return()
    endif()
    
    # Create the source group
    source_group(TREE ${arg_TREE} FILES ${sources_list})
endfunction()
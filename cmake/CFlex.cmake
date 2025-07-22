cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CFLEX_VERSION 0.1.0)

macro(cflex_message mode)
    message(${mode} "CFlex: ${ARGN}")
endmacro()

# Check if CFlex has already been initialized
get_property(CFLEX_ALREADY_INITIALIZED GLOBAL PROPERTY CFLEX_INITIALIZED SET)
if(CFLEX_ALREADY_INITIALIZED)

    # Retrieve the previously initialized version of CFlex
    get_property(
        CFLEX_ALREADY_INITIALIZED_VERSION
        GLOBAL
        PROPERTY CFLEX_INITIALIZED_VERSION
    )

    # Check if a newer version is being used by dependent projects
    if(CFLEX_ALREADY_INITIALIZED_VERSION VERSION_LESS CFLEX_VERSION)
        cflex_message(STATUS "\
A newer version of CFlex (${CFLEX_VERSION}) has been detected in a dependent project.\n\
However, only one version can be active at a time, so the first loaded version (${CFLEX_ALREADY_INITIALIZED_VERSION}) will be used.\n\
It is recommended to update CFlex to the newer version (${CFLEX_VERSION}) to maintain consistency.")
    endif()
    return()
endif()

set_property(GLOBAL PROPERTY CFLEX_INITIALIZED TRUE)
set_property(GLOBAL PROPERTY CFLEX_INITIALIZED_VERSION "${CFLEX_VERSION}")

cflex_message(STATUS "${CFLEX_VERSION} initialized successfully.")

set(CFLEX_LOG_BUILD_OPTIONS ON CACHE BOOL "Log build options")

function(_cflex_get_unique_project_id project_id)
    string(MD5 project_hash "${PROJECT_SOURCE_DIR}")
    string(SUBSTRING ${project_hash} 0 8 project_hash)
    set(project_id_value "${PROJECT_NAME}_${project_hash}")
    string(TOUPPER "${project_id_value}" project_id_value)
    set(${project_id} "${project_id_value}" PARENT_SCOPE)
endfunction()

macro(_cflex_list_to_string list)
    string(REPLACE ";" " " ${list} "${${list}}")
endmacro()

macro(_cflex_set_build_options flag_name)
    set(multi_value_args PUBLIC PRIVATE INTERFACE GLOBAL)
    set(one_value_arg BUILD_TYPE)
    cmake_parse_arguments(
        arg
        ""
        "${one_value_arg}"
        "${multi_value_args}"
        ${ARGN}
    )

    string(TOUPPER "${arg_BUILD_TYPE}" build_type_upper)
    _cflex_list_to_string(arg_GLOBAL)

    # Generate a unique flag name based on the project hash and project name
    _cflex_get_unique_project_id(project_id)
    set(internal_flag_name "${project_id}_${flag_name}")

    # Apply INTERNAL flags
    foreach(scope IN ITEMS PUBLIC PRIVATE INTERFACE)
        if(DEFINED arg_${scope})
            if(build_type_upper)
                set(var_name
                    "CFLEX_${internal_flag_name}_${build_type_upper}_${scope}"
                )
            else()
                set(var_name "CFLEX_${internal_flag_name}_${scope}")
            endif()
            set(${var_name}
                ${arg_${scope}}
                CACHE STRING
                "${flag_name} for ${build_type_upper} (${scope})"
            )
            mark_as_advanced(${var_name})
        endif()
    endforeach()

    # Apply GLOBAL flags (overwrite existing values)
    if(DEFINED arg_GLOBAL)
        if(build_type_upper)
            set(var_name "CMAKE_${flag_name}_${build_type_upper}")
        else()
            set(var_name "CMAKE_${flag_name}")
        endif()
        set(${var_name} ${arg_GLOBAL} CACHE STRING "" FORCE)
    endif()
endmacro()

macro(_cflex_dispatch_build_options flag_name)
    set(multi_value_args
        BUILD_TYPES
        PUBLIC
        PRIVATE
        INTERFACE
        GLOBAL
    )

    cmake_parse_arguments(arg "" "" "${multi_value_args}" ${ARGN})

    # Apply flags for each build type if specified, otherwise apply to all build types
    if(NOT arg_BUILD_TYPES)
        _cflex_set_build_options(${flag_name} ${ARGN})
    else()
        foreach(build_type IN ITEMS ${arg_BUILD_TYPES})
            _cflex_set_build_options(${flag_name} BUILD_TYPE ${build_type} ${ARGN})
        endforeach()
    endif()
endmacro()

# cflex_set_c_flags(
#   [BUILD_TYPES <build_types>...] 
#   [PUBLIC <flags>...] 
#   [PRIVATE <flags>...] 
#   [INTERFACE <flags>...] 
#   [GLOBAL <flags>...]
# )
#
#   Set C compiler flags for targets. Flags can be applied to specific scopes (PUBLIC, PRIVATE, INTERFACE) or globally (GLOBAL).
#   If BUILD_TYPES is specified, flags will only apply to the specified builds type (e.g., DEBUG, RELEASE).
function(cflex_set_c_flags)
    _cflex_dispatch_build_options("C_FLAGS" ${ARGN})
endfunction()

# cflex_set_cxx_flags(
#   [BUILD_TYPES <build_types>...] 
#   [PUBLIC <flags>...] 
#   [PRIVATE <flags>...] 
#   [INTERFACE <flags>...] 
#   [GLOBAL <flags>...]
# )
#
#   Set C++ compiler flags for targets. Flags can be applied to specific scopes (PUBLIC, PRIVATE, INTERFACE) or globally (GLOBAL).
#   If BUILD_TYPES is specified, flags will only apply to the specified builds type (e.g., DEBUG, RELEASE).
function(cflex_set_cxx_flags)
    _cflex_dispatch_build_options("CXX_FLAGS" ${ARGN})
endfunction()

# cflex_set_linker_flags(
#   [BUILD_TYPES <build_types>...] 
#   [PUBLIC <flags>...] 
#   [PRIVATE <flags>...] 
#   [INTERFACE <flags>...] 
#   [GLOBAL <flags>...]
# )
#
#   Set linker flags for all target types (module, shared, static):
#     - PUBLIC, PRIVATE, INTERFACE: Target-specific linker flags.
#     - GLOBAL: Overwrites global CMake linker flags.
#     - BUILD_TYPE: Optional, applies flags only for the specified build type.
function(cflex_set_linker_flags)
    cflex_set_exe_linker_flags(${ARGN})
    cflex_set_module_linker_flags(${ARGN})
    cflex_set_shared_linker_flags(${ARGN})
    cflex_set_static_linker_flags(${ARGN})
endfunction()

# cflex_set_exe_linker_flags(
#   [BUILD_TYPES <build_types>...] 
#   [PUBLIC <flags>...] 
#   [PRIVATE <flags>...] 
#   [INTERFACE <flags>...] 
#   [GLOBAL <flags>...]
# )
#
#   Set linker flags for executable targets:
#     - PUBLIC, PRIVATE, INTERFACE: Target-specific linker flags.
#     - GLOBAL: Overwrites global CMake linker flags.
#     - BUILD_TYPE: Optional, applies flags only for the specified build type.
function(cflex_set_exe_linker_flags)
    _cflex_dispatch_build_options("EXE_LINK_OPTIONS" ${ARGN})
endfunction()

# cflex_set_module_linker_flags(
#   [BUILD_TYPES <build_types>...] 
#   [PUBLIC <flags>...] 
#   [PRIVATE <flags>...] 
#   [INTERFACE <flags>...] 
#   [GLOBAL <flags>...]
# )
#
#   Set linker flags for module targets:
#     * PUBLIC, PRIVATE, INTERFACE: Target-specific linker flags.
#     * GLOBAL: Overwrites global CMake linker flags.
#     * BUILD_TYPE: Optional, applies flags only for the specified build type.
function(cflex_set_module_linker_flags)
    _cflex_dispatch_build_options("MODULE_LINK_OPTIONS" ${ARGN})
endfunction()

# cflex_set_shared_linker_flags(
#   [BUILD_TYPES <build_types>...] 
#   [PUBLIC <flags>...] 
#   [PRIVATE <flags>...] 
#   [INTERFACE <flags>...] 
#   [GLOBAL <flags>...]
# )
#
#   Set linker flags for shared library targets:
#     * PUBLIC, PRIVATE, INTERFACE: Target-specific linker flags.
#     * GLOBAL: Overwrites global CMake linker flags.
#     * BUILD_TYPE: Optional, applies flags only for the specified build type.
function(cflex_set_shared_linker_flags)
    _cflex_dispatch_build_options("SHARED_LINK_OPTIONS" ${ARGN})
endfunction()

# cflex_set_static_linker_flags(
#   [BUILD_TYPES <build_types>...] 
#   [PUBLIC <flags>...] 
#   [PRIVATE <flags>...] 
#   [INTERFACE <flags>...] 
#   [GLOBAL <flags>...]
# )
#
#   Set linker flags for static library targets:
#     - PUBLIC, PRIVATE, INTERFACE: Target-specific linker flags.
#     - GLOBAL: Overwrites global CMake linker flags.
#     - BUILD_TYPE: Optional, applies flags only for the specified build type.
function(cflex_set_static_linker_flags)
    _cflex_dispatch_build_options("STATIC_LINK_OPTIONS" ${ARGN})
endfunction()

# cflex_configure_target(<target>)
#
#   Configure an existing target with the flags set by cflex_set_* functions.
#   This function is automatically called by cflex_add_library and cflex_add_executable.
function(cflex_configure_target target)
    if(NOT TARGET ${target})
        cflex_message(WARNING "Target ${target} does not exist.")
        return()
    endif()

    get_target_property(target_type ${target} TYPE)

    # Map CMake target types to our flag types
    if(target_type STREQUAL "EXECUTABLE")
        set(target_type "EXE")
    elseif(target_type STREQUAL "MODULE_LIBRARY")
        set(target_type "MODULE")
    elseif(target_type STREQUAL "SHARED_LIBRARY")
        set(target_type "SHARED")
    elseif(target_type STREQUAL "STATIC_LIBRARY")
        set(target_type "STATIC")
    elseif(target_type STREQUAL "INTERFACE_LIBRARY")
        set(target_type "INTERFACE")
    else()
        cflex_message(WARNING "Unsupported target type: ${target_type} for target ${target}")
        return()
    endif()

    # Convert CMAKE_BUILD_TYPE to UPPER_CASE
    if(CMAKE_BUILD_TYPE)
        string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type_upper)
    endif()

    _cflex_get_unique_project_id(project_id)

    # Apply flags for each scope
    foreach(scope IN ITEMS PUBLIC PRIVATE INTERFACE)
        # Skip PRIVATE scope for INTERFACE libraries
        if(target_type STREQUAL "INTERFACE" AND scope STREQUAL "PRIVATE")
            # Check if there are any PRIVATE flags defined and warn about them
            set(c_private_flags "CFLEX_${project_id}_C_FLAGS_PRIVATE")
            set(cxx_private_flags "CFLEX_${project_id}_CXX_FLAGS_PRIVATE")
            set(linker_private_flags "CFLEX_${project_id}_INTERFACE_LINK_OPTIONS_PRIVATE")
            
            if(CMAKE_BUILD_TYPE)
                set(c_private_flags_build "CFLEX_${project_id}_C_FLAGS_${build_type_upper}_PRIVATE")
                set(cxx_private_flags_build "CFLEX_${project_id}_CXX_FLAGS_${build_type_upper}_PRIVATE")
                set(linker_private_flags_build "CFLEX_${project_id}_INTERFACE_LINK_OPTIONS_${build_type_upper}_PRIVATE")
            endif()
            
            if(DEFINED ${c_private_flags} OR DEFINED ${cxx_private_flags} OR DEFINED ${linker_private_flags} OR 
               (CMAKE_BUILD_TYPE AND (DEFINED ${c_private_flags_build} OR DEFINED ${cxx_private_flags_build} OR DEFINED ${linker_private_flags_build})))
                cflex_message(WARNING "Target ${target} is an INTERFACE library, but PRIVATE flags were specified. PRIVATE flags will be ignored for INTERFACE libraries.")
            endif()
            continue()
        endif()

        # General C compile flags
        set(c_compile_flags "CFLEX_${project_id}_C_FLAGS_${scope}")
        if(DEFINED ${c_compile_flags})
            target_compile_options(${target} ${scope} $<$<COMPILE_LANGUAGE:C>:${${c_compile_flags}}>)
        endif()

        # General C++ compile flags
        set(cxx_compile_flags "CFLEX_${project_id}_CXX_FLAGS_${scope}")
        if(DEFINED ${cxx_compile_flags})
            target_compile_options(${target} ${scope} $<$<COMPILE_LANGUAGE:CXX>:${${cxx_compile_flags}}>)
        endif()

        # General linker options
        set(linker_flags
            "CFLEX_${project_id}_${target_type}_LINK_OPTIONS_${scope}"
        )
        if(DEFINED ${linker_flags})
            target_link_options(${target} ${scope} ${${linker_flags}})
        endif()

        # Build-specific flags if CMAKE_BUILD_TYPE is set
        if(CMAKE_BUILD_TYPE)
            # Build-specific C compile flags
            set(c_compile_flags_build
                "CFLEX_${project_id}_C_FLAGS_${build_type_upper}_${scope}"
            )
            if(DEFINED ${c_compile_flags_build})
                target_compile_options(
                    ${target}
                    ${scope}
                    $<$<COMPILE_LANGUAGE:C>:${${c_compile_flags_build}}>
                )
            endif()

            # Build-specific C++ compile flags
            set(cxx_compile_flags_build
                "CFLEX_${project_id}_CXX_FLAGS_${build_type_upper}_${scope}"
            )
            if(DEFINED ${cxx_compile_flags_build})
                target_compile_options(
                    ${target}
                    ${scope}
                    $<$<COMPILE_LANGUAGE:CXX>:${${cxx_compile_flags_build}}>
                )
            endif()

            # Build-specific linker options
            set(linker_flags_build
                "CFLEX_${project_id}_${target_type}_LINK_OPTIONS_${build_type_upper}_${scope}"
            )
            if(DEFINED ${linker_flags_build})
                target_link_options(${target} ${scope} ${${linker_flags_build}})
            endif()
        endif()
    endforeach()

    # Warn if CMAKE_BUILD_TYPE is not set
    if(NOT CMAKE_BUILD_TYPE)
        cflex_message(
            WARNING
            "CMAKE_BUILD_TYPE is not set. Skipping build-specific flags for target ${target}."
        )
    endif()

    # Log build options if enabled
    if(CFLEX_LOG_BUILD_OPTIONS)
        get_target_property(compile_options ${target} COMPILE_OPTIONS)
        get_target_property(link_options ${target} LINK_OPTIONS)
        
        # Log compile options
        if(compile_options AND NOT compile_options STREQUAL "NOTFOUND")
            cflex_message(STATUS "Target ${target} - Compile options: ${compile_options}")
        else()
            cflex_message(STATUS "Target ${target} - No compile options set")
        endif()
        
        # Log link options
        if(link_options AND NOT link_options STREQUAL "NOTFOUND")
            cflex_message(STATUS "Target ${target} - Link options: ${link_options}")
        else()
            cflex_message(STATUS "Target ${target} - No link options set")
        endif()
        
        # Log target type for context
        cflex_message(STATUS "Target ${target} - Type: ${target_type}")
    endif()
endfunction()

# cflex_add_library(<target> [STATIC | SHARED | MODULE] SOURCES <sources>...)
#
#   Add a library target called <target> to be built from the source files listed in the command invocation.
#   The optional <type> specifies the type of library to be created:
#     - STATIC: A static library (archive of object files).
#     - SHARED: A shared library (dynamic library loaded at runtime).
#     - MODULE: A module library (plugin loaded at runtime using dlopen-like functionality).
#   If no <type> is specified, the default is STATIC.
#   The function will automatically apply CFlex configurations to the target after creation.
function(cflex_add_library target)
    # Check if target already exists
    if(TARGET ${target})
        cflex_message(WARNING "Target ${target} already exists.")
        return()
    endif()

    set(options STATIC SHARED MODULE)
    set(multi_value_args SOURCES)
    cmake_parse_arguments(arg "${options}" "" "${multi_value_args}" ${ARGN})

    if(NOT arg_SOURCES)
        cflex_message(FATAL_ERROR "No SOURCES provided for target ${target}.")
    endif()

    # Check if multiple library types are specified
    set(lib_type_count 0)
    foreach(type IN ITEMS STATIC SHARED MODULE)
        if(arg_${type})
            math(EXPR lib_type_count "${lib_type_count} + 1")
            set(lib_type ${type})
        endif()
    endforeach()

    if(lib_type_count GREATER 1)
        cflex_message(
            FATAL_ERROR
            "Multiple library types specified for target ${target}. Please specify only one of: STATIC, SHARED, MODULE."
        )
    endif()

    # Default to STATIC if no type is specified
    if(lib_type_count EQUAL 0)
        set(lib_type "STATIC")
    endif()

    # Create library target
    add_library(${target} ${lib_type} ${arg_SOURCES})

    # Configure target
    cflex_configure_target(${target} ${ARGN})
endfunction()

# cflex_add_executable(<target> SOURCES <sources>...)
#
#   Add an executable target called <target> to be built from the source files listed in the command invocation.
#   The function will automatically apply CFlex configurations (e.g., compiler flags, linker flags) to the target after creation.
function(cflex_add_executable target)
    set(multi_value_args SOURCES)
    cmake_parse_arguments(arg "" "" "${multi_value_args}" ${ARGN})

    # Check if target already exists
    if(TARGET ${target})
        cflex_message(WARNING "Target ${target} already exists.")
        return()
    endif()

    # Create executable target
    add_executable(${target} ${arg_SOURCES})

    # Configure target
    cflex_configure_target(${target} ${ARGN})
endfunction()

# CMakeLists.txt for BCPL Runtime
cmake_minimum_required(VERSION 3.12)

# Define the runtime library targets
set(RUNTIME_SOURCES
    runtime_bridge.cpp
    heap_c_bridge.cpp
    RuntimeBridge.cpp
)

# Define include paths
set(RUNTIME_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Add the C++ runtime library for JIT compilation
add_library(bcpl_runtime_jit STATIC ${RUNTIME_SOURCES})
target_include_directories(bcpl_runtime_jit PUBLIC ${RUNTIME_INCLUDE_DIRS})
target_link_libraries(bcpl_runtime_jit PRIVATE heap_manager)
set_target_properties(bcpl_runtime_jit PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# Add the C runtime library for standalone compilation
# This is only built if BCPL_BUILD_STANDALONE_RUNTIME is set to ON
option(BCPL_BUILD_STANDALONE_RUNTIME "Build the standalone C runtime" OFF)
if(BCPL_BUILD_STANDALONE_RUNTIME)
    add_library(bcpl_runtime_c STATIC runtime.c)
    target_include_directories(bcpl_runtime_c PUBLIC ${RUNTIME_INCLUDE_DIRS})
    set_target_properties(bcpl_runtime_c PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON
    )

    # Option to build a standalone runtime test executable
    option(BCPL_BUILD_RUNTIME_TEST "Build a test executable for the standalone runtime" OFF)
    if(BCPL_BUILD_RUNTIME_TEST)
        add_executable(runtime_test runtime_test.c)
        target_link_libraries(runtime_test PRIVATE bcpl_runtime_c)
        target_include_directories(runtime_test PRIVATE ${RUNTIME_INCLUDE_DIRS})
    endif()
endif()

# Install rules
install(FILES runtime.h DESTINATION include)
install(TARGETS bcpl_runtime_jit DESTINATION lib)
if(BCPL_BUILD_STANDALONE_RUNTIME)
    install(TARGETS bcpl_runtime_c DESTINATION lib)
endif()

# Add a simple debug target
add_custom_target(print_runtime_vars
    COMMAND ${CMAKE_COMMAND} -E echo "RUNTIME_SOURCES: ${RUNTIME_SOURCES}"
    COMMAND ${CMAKE_COMMAND} -E echo "RUNTIME_INCLUDE_DIRS: ${RUNTIME_INCLUDE_DIRS}"
)

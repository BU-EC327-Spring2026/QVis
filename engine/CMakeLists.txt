cmake_minimum_required(VERSION 3.14)
project(qvis_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Engine static library ---------------------------------------------------
file(GLOB_RECURSE ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_library(qvis_engine STATIC ${ENGINE_SOURCES})
target_include_directories(qvis_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# --- GoogleTest via FetchContent ----------------------------------------------
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --- pybind11 bindings (built by scikit-build-core during pip install) --------
option(BUILD_PYTHON_BINDINGS "Build pybind11 Python extension module" OFF)
if(BUILD_PYTHON_BINDINGS)
    find_package(pybind11 CONFIG REQUIRED)
    pybind11_add_module(qvis_engine_ext "${CMAKE_CURRENT_SOURCE_DIR}/../bindings/qvis_engine_ext.cpp")
    target_link_libraries(qvis_engine_ext PRIVATE qvis_engine)
    install(TARGETS qvis_engine_ext DESTINATION .)
endif()

# --- GoogleTest via FetchContent ----------------------------------------------
if(NOT BUILD_PYTHON_BINDINGS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    include(GoogleTest)

    file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
    add_executable(qvis_engine_tests ${TEST_SOURCES})
    target_link_libraries(qvis_engine_tests PRIVATE qvis_engine GTest::gtest_main)
    gtest_discover_tests(qvis_engine_tests)
endif()

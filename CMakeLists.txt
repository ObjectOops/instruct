cmake_minimum_required (VERSION 3.12)

project(instruct
    LANGUAGES CXX
    VERSION 1.0.0
    DESCRIPTION "Automated coding pedagogy augmentation gizmo."
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -Werror -fsanitize=address")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} -s -static")

include(FetchContent)

# FTXUI -----------------------
FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
    GIT_TAG v5.0.0
)

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
    message(STATUS "Fetching FTXUI...")
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
# YAML-CPP --------------------
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp
    GIT_TAG 0.8.0
)
FetchContent_GetProperties(yaml-cpp)

if(NOT yaml-cpp_POPULATED)
    message(STATUS "Fetching yaml-cpp...")
    FetchContent_Populate(yaml-cpp)
    add_subdirectory(${yaml-cpp_SOURCE_DIR} ${yaml-cpp_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
# -----------------------------

add_executable(instruct
    src/main.cpp
    src/setup.cpp
)
target_include_directories(instruct PRIVATE src)

target_link_libraries(instruct
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component
    PRIVATE yaml-cpp::yaml-cpp
)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(instruct PRIVATE -DDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(instruct PRIVATE -DNDEBUG)
endif()

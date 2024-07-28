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

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -pg -Wall -Wextra -Werror -fsanitize=address")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} -s -static")

include(FetchContent)

# FTXUI -----------------------
FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG v5.0.0
)

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
    message(STATUS "Fetching FTXUI...")
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
# YAML-CPP --------------------
FetchContent_Declare(yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
)

FetchContent_GetProperties(yaml-cpp)
if(NOT yaml-cpp_POPULATED)
    message(STATUS "Fetching yaml-cpp...")
    FetchContent_Populate(yaml-cpp)
    add_subdirectory(${yaml-cpp_SOURCE_DIR} ${yaml-cpp_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
# PICOSHA2 --------------------
FetchContent_Declare(picosha2
    GIT_REPOSITORY https://github.com/okdshin/PicoSHA2.git
    GIT_TAG 27fcf6979298949e8a462e16d09a0351c18fcaf2
)

FetchContent_GetProperties(picosha2)
if(NOT picosha2_POPULATED)
    message(STATUS "Fetching picosha2...")
    FetchContent_Populate(picosha2)
    add_subdirectory(${picosha2_SOURCE_DIR} ${picosha2_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
# LOGURU ----------------------
FetchContent_Declare(loguru
    GIT_REPOSITORY https://github.com/emilk/loguru.git
    GIT_TAG 4adaa185883e3c04da25913579c451d3c32cfac1
)

FetchContent_GetProperties(loguru)

set(LOGURU_WITH_STREAMS TRUE) # For stream logging.

if(NOT loguru_POPULATED)
    message(STATUS "Fetching loguru...")
    FetchContent_Populate(loguru)
    add_subdirectory(${loguru_SOURCE_DIR} ${loguru_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
# CPP-HTTPLIB -----------------
FetchContent_Declare(cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.16.0
)

FetchContent_GetProperties(cpp-httplib)
if(NOT cpp-httplib_POPULATED)
    message(STATUS "Fetching cpp-httplib...")
    FetchContent_Populate(cpp-httplib)
    # add_subdirectory(${cpp-httplib_SOURCE_DIR} ${cpp-httplib_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
# STDUUID ---------------------
FetchContent_Declare(stduuid
    GIT_REPOSITORY https://github.com/mariusbancila/stduuid.git
    GIT_TAG v1.2.3
)

FetchContent_GetProperties(stduuid)
if(NOT stduuid_POPULATED)
    message(STATUS "Fetching stduuid...")
    FetchContent_Populate(stduuid)
    add_subdirectory(${stduuid_SOURCE_DIR} ${stduuid_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
# VINCE'S CSV PARSER ----------
FetchContent_Declare(csv-parser
    GIT_REPOSITORY https://github.com/vincentlaucsb/csv-parser.git
    GIT_TAG 2.3.0
    GIT_SUBMODULES ""
)

FetchContent_GetProperties(csv-parser)
if(NOT csv-parser_POPULATED)
    message(STATUS "Fetching csv-parser...")
    FetchContent_Populate(csv-parser)
    add_subdirectory(${csv-parser_SOURCE_DIR} ${csv-parser_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
# LIBARCHIVE ------------------
FetchContent_Declare(libarchive
    GIT_REPOSITORY https://github.com/libarchive/libarchive.git
    GIT_TAG v3.7.4
)

FetchContent_GetProperties(libarchive)
if(NOT libarchive_POPULATED)
    message(STATUS "Fetching libarchive...")
    FetchContent_Populate(libarchive)
    
    cmake_policy(PUSH)
    set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
    
    add_subdirectory(${libarchive_SOURCE_DIR} ${libarchive_BINARY_DIR} EXCLUDE_FROM_ALL)
    
    cmake_policy(POP)
endif()
# -----------------------------

add_executable(instruct
    src/notification.cpp
    src/security.cpp
    src/logging.cpp
    src/setup.cpp
    src/main.cpp
    src/data.cpp
    src/ui.cpp
)

# Some libraries need be added manually...
target_include_directories(instruct PRIVATE src ${cpp-httplib_SOURCE_DIR})
target_include_directories(instruct PRIVATE src "${csv-parser_SOURCE_DIR}/single_include")
target_include_directories(instruct PRIVATE src "${libarchive_SOURCE_DIR}/libarchive")

target_link_libraries(instruct
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component
    PRIVATE yaml-cpp::yaml-cpp
    PRIVATE picosha2 # Single header file.
    PRIVATE loguru::loguru
    # PRIVATE cpp-httplib # Single header file.
    PRIVATE stduuid # Just headers.
    # PRIVATE csv-parser # Single header file.
    # PRIVATE libarchive
    PRIVATE ${CMAKE_DL_LIBS}
)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(instruct PRIVATE -DDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(instruct PRIVATE -DNDEBUG)
endif()

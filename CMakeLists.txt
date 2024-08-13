cmake_minimum_required(VERSION 3.16)

project(instruct
    LANGUAGES CXX
    VERSION 1.0.0
    DESCRIPTION "Automated coding pedagogy augmentation gizmo."
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -pg -Og -fsanitize=address -fsanitize=leak -fsanitize=undefined")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fsanitize=leak -fsanitize=undefined --coverage")
# Note: --coverage is only being linked for csv-parser in the debug build.

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} -s")

include(FetchContent)

# FTXUI -----------------------
FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG v5.0.0
)
set(FTXUI_QUIET ON) # Disable CMake messages.
set(FTXUI_ENABLE_INSTALL OFF) # Disable library installation target.
# YAML-CPP --------------------
FetchContent_Declare(yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
)
add_library(uninstall INTERFACE) # Disable library uninstall target via workaround.
set(BUILD_TESTING OFF) # Disable CTest targets.
set(YAML_CPP_BUILD_CONTRIB OFF) # Disable library extension for Visual Studio.
set(YAML_CPP_BUILD_TOOLS OFF) # Disable building executable utilities.
set(YAML_CPP_FORMAT_SOURCE OFF) # Disable clang-format target if it's installed.
# PICOSHA2 --------------------
FetchContent_Declare(picosha2
    GIT_REPOSITORY https://github.com/okdshin/PicoSHA2.git
    GIT_TAG 27fcf6979298949e8a462e16d09a0351c18fcaf2
)
# LOGURU ----------------------
FetchContent_Declare(loguru
    GIT_REPOSITORY https://github.com/emilk/loguru.git
    GIT_TAG 4adaa185883e3c04da25913579c451d3c32cfac1
)
set(LOGURU_WITH_STREAMS TRUE) # Enable stream logging.
# CPP-HTTPLIB -----------------
FetchContent_Declare(cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.16.0
)
set(HTTPLIB_REQUIRE_OPENSSL ON) # Enable OpenSSL support.
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF) # Disable automatically including OpenSSL.
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF) # Disable automatically including zlib.
find_package(Python3)
if(Python3_FOUND)
    # Enable splitting the single header file into a header and source file.
    set(HTTPLIB_COMPILE ON)
endif()
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF) # Disable automatically including brotli.
set(HTTPLIB_INSTALL OFF) # Disable library installation target.
set(OPENSSL_USE_STATIC_LIBS ON) # Enable static libraries.
# STDUUID ---------------------
FetchContent_Declare(stduuid
    GIT_REPOSITORY https://github.com/mariusbancila/stduuid.git
    GIT_TAG v1.2.3
)
# VINCE'S CSV PARSER ----------
FetchContent_Declare(csv-parser
    GIT_REPOSITORY https://github.com/vincentlaucsb/csv-parser.git
    GIT_TAG 2.3.0
    GIT_SUBMODULES "" # Don't initialize submodules.
)
set(CSV_BUILD_PROGRAMS OFF) # Disable building executable utilities.
# -----------------------------

FetchContent_MakeAvailable(
    ftxui
    yaml-cpp
    picosha2
    loguru
    cpp-httplib
    stduuid
    csv-parser
)

add_executable(instruct
    src/ui/util/terminal.cpp
    src/ui/util/spinner.cpp
    src/ui/util/input.cpp
    src/notification.cpp
    src/security.cpp
    src/logging.cpp
    src/ui/ui.cpp
    src/setup.cpp
    src/main.cpp
    src/data.cpp
)

# Precompile large header files.
target_precompile_headers(instruct
    PRIVATE "\"yaml-cpp/yaml.h\""
    PRIVATE "\"picosha2.h\""
    PRIVATE "\"loguru.hpp\""
    PRIVATE "\"httplib.h\""
    PRIVATE "\"uuid.h\""
    PRIVATE "\"csv.hpp\""
)

find_package(LibArchive REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)

find_library(LIBACL_STATIC "libacl.a" REQUIRED)

target_link_libraries(instruct
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component
    PRIVATE yaml-cpp::yaml-cpp
    PRIVATE picosha2
    PRIVATE loguru::loguru
    PRIVATE httplib::httplib
    PRIVATE stduuid
    PRIVATE csv

    # Shared libraries.
    # PRIVATE LibArchive::LibArchive
    # PRIVATE ZLIB::ZLIB # Required by libarchive.
    # PRIVATE OpenSSL::SSL # Required by httplib.
    # PRIVATE OpenSSL::Crypto # Required by httplib.

    # Static libraries.
    PRIVATE libarchive.a
    PRIVATE libz.a # Required by libarchive.
    PRIVATE libacl.a # Required by libarchive.
    PRIVATE libssl.a # Required by httplib.
    PRIVATE libcrypto.a # Required by httplib.
)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(instruct PRIVATE -DDEBUG -Wall -Wextra -Wpedantic -Werror)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(instruct PRIVATE -DNDEBUG)
endif()

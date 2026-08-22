cmake_minimum_required(VERSION 3.20)
project(cppgit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
if(WIN32)
  find_package(CURL REQUIRED)
  set(CPPGIT_CURL_TARGET CURL::libcurl)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(CURL REQUIRED IMPORTED_TARGET libcurl)
  set(CPPGIT_CURL_TARGET PkgConfig::CURL)
endif()

add_library(cppgit_lib
    src/util.cpp
    src/repository.cpp
    src/object_store.cpp
    src/index.cpp
    src/tree.cpp
    src/refs.cpp
    src/diff.cpp
    src/pack.cpp
    src/network.cpp
)

target_include_directories(cppgit_lib PUBLIC include)
target_link_libraries(cppgit_lib PUBLIC OpenSSL::Crypto ZLIB::ZLIB ${CPPGIT_CURL_TARGET})
if (MSVC)
  target_compile_options(cppgit_lib PRIVATE /W4)
else()
  target_compile_options(cppgit_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(cppgit src/main.cpp)
target_link_libraries(cppgit PRIVATE cppgit_lib)

enable_testing()
add_executable(cppgit_tests tests/tests.cpp)
target_link_libraries(cppgit_tests PRIVATE cppgit_lib)
add_test(NAME cppgit_tests COMMAND cppgit_tests)

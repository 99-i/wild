cmake_minimum_required(VERSION 3.24)
project(wild)

# required for clangd to work.
set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES 0)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES 0)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 23)

find_package(ZLIB)

include_directories(
  SYSTEM
  C:/msys64/mingw64/bin/../lib/gcc/x86_64-w64-mingw32/13.1.0/../../../../include/c++/13.1.0
)
include_directories(
  SYSTEM
  C:/msys64/mingw64/bin/../lib/gcc/x86_64-w64-mingw32/13.1.0/../../../../include/c++/13.1.0/x86_64-w64-mingw32
)
include_directories(
  SYSTEM
  C:/msys64/mingw64/bin/../lib/gcc/x86_64-w64-mingw32/13.1.0/../../../../include/c++/13.1.0/backward
)
include_directories(
  SYSTEM C:/msys64/mingw64/bin/../lib/gcc/x86_64-w64-mingw32/13.1.0/include)
include_directories(
  SYSTEM
  C:/msys64/mingw64/bin/../lib/gcc/x86_64-w64-mingw32/13.1.0/../../../../include
)
include_directories(
  SYSTEM
  C:/msys64/mingw64/bin/../lib/gcc/x86_64-w64-mingw32/13.1.0/include-fixed)


include_directories(SYSTEM include)
include_directories(SYSTEM lib/plog-1.1.10/include)
include_directories(SYSTEM lib/asio-1.28.0/include)

add_subdirectory(src)
add_subdirectory(tests)

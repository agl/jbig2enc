cmake_minimum_required(VERSION 3.10)
project(jbig2enc CXX)
set(Version "0.30")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

# ##############################################################################
# jbig2enc library
# ##############################################################################
add_definitions(-DVERSION="${Version}")

if(WIN32)
  set(LIBS ${LIBS} Ws2_32)
  if(MSVC)
    add_definitions(-DWIN32)
    add_definitions(-D_LIB)
    add_definitions(-D_CONSOLE)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MD")
    # turn off warnings in Release
    # C4267: '=': conversion from 'size_t' to 'int', possible loss of data
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /wd4267")
    # C4996: 'x': The POSIX name for this item is deprecated. Instead,
    # use the ISO C and C++ conformant name:
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /wd4996")
    # C4334: '<<': result of 32-bit shift implicitly converted to 64
    # bits (was 64-bit shift intended?)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /wd4334")
    # C4244: 'x': conversion from 'a' to 'b', possible loss of data
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /wd4244")
    # C4305: 'initializing': truncation from 'double' to 'float'
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /wd4305")
    # Enable multiprocessor builds on Visual Studio
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  endif()
else()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG
      "${CMAKE_CXX_FLAGS_DEBUG} -DUNIFICATION_DEBUGGING -Wall -DDEBUG -pedantic -Og -g -noalign"
  )
endif()

if(NOT DEFINED CMAKE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX "d")
endif()

find_package(Leptonica REQUIRED)
message(STATUS "Found Leptonica library: ${Leptonica_VERSION}")
include_directories(${Leptonica_INCLUDE_DIRS})
link_directories(${Leptonica_LIBRARY_DIRS})

set(libjbig2enc_src
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jbig2arith.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jbig2comparator.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jbig2enc.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jbig2sym.cc")
set(libjbig2enc_hdr
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jbig2arith.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jbig2comparator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jbig2enc.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jbig2segments.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jbig2structs.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jbig2sym.h")
add_library(libjbig2enc ${libjbig2enc_src} ${libjbig2enc_hdr})
set_target_properties(libjbig2enc PROPERTIES DEBUG_POSTFIX
                                             ${CMAKE_DEBUG_POSTFIX})

install(
  TARGETS libjbig2enc
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/jbig2enc.h" DESTINATION include/)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/jbig2topdf.py" DESTINATION bin/)

# ##############################################################################
# Programs
# ##############################################################################

add_executable(jbig2 "${CMAKE_CURRENT_SOURCE_DIR}/src/jbig2.cc")
set_target_properties(jbig2 PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_link_libraries(jbig2 PRIVATE ${LIBS} libjbig2enc)
target_link_libraries(jbig2 PUBLIC ${Leptonica_LIBRARIES})
install(
  TARGETS jbig2
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

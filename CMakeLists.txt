cmake_minimum_required (VERSION 2.6)
project (contourpp)

set(CMAKE_USE_RELATIVE_PATHS ON)

include_directories (/usr/local/include)
include_directories (${contourpp_SOURCE_DIR}/include)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost COMPONENTS date_time)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
find_package(Hidapi REQUIRED)
include_directories(${HIDAPI_INCLUDE_DIRS})
set(LIBS ${LIBS} ${HIDAPI_LIBRARIES})

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -fexceptions -fcxx-exceptions")
  if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

elseif (CMAKE_COMPILER_IS_GNUCXX)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
  if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_OSX_SYSROOT "")
  endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

endif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")


add_subdirectory(src)

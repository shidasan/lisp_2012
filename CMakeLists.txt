cmake_minimum_required(VERSION 2.4)
set(CMAKE_C_FLAGS_RELEASE "-W -Wall -O2")
set(CMAKE_C_FLAGS_DEBUG "-g")
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

project(lisp)
set(serial "1.0")

if(UNIX)
	set(K_OSDLLEXT ".so")
endif(UNIX)
if(APPLE)
	set(K_OSDLLEXT ".dylib")
endif(APPLE)
if(CYGWIN)
	set(K_OSDLLEXT ".so")
endif(CYGWIN)
if(MSVC)
	set(K_OSDLLEXT ".dll")
endif(MSVC)
if(MINGW)
	set(K_OSDLLEXT ".dll")
endif(MINGW)

add_definitions(-DHAVE_CONFIG_H)
configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.cmake
		${CMAKE_CURRENT_SOURCE_DIR}/include/config.h)
include_directories("${PROJECT_SOURCE_DIR}/include")

set(LISP_SRC
		src/eval.cpp
		src/generator.cpp
		src/hash.cpp
		src/parser.cpp
		src/mtddata.cpp
   )
set(LISP_BIN src/main.cpp)
set(GC_BIN src/gc.cpp)
add_library(lisp SHARED ${LISP_SRC})
add_executable(lisp ${LISP_BIN})
add_executable(gc_test ${GC_BIN})

enable_testing()
set(ktest ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lisp)
set(test_dir ${CMAKE_CURRENT_SOURCE_DIR}/test)
file(GLOB_RECURSE files "${test_dir}/*")
foreach(f ${files})
	if("${f}" MATCHES ".lisp$")
		string(REGEX REPLACE ".*/ac/([^ ]+).k" "\\1" res "${f}")
		add_test(NAME ${res} COMMAND ${ktest} "${f}" "--testing")
	endif()
endforeach()

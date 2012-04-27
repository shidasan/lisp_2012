cmake_minimum_required(VERSION 2.4)
set(CMAKE_C_FLAGS_RELEASE "-W -Wall -O2")
set(CMAKE_C_FLAGS_DEBUG "-g -W -Wall")
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

set(GC_TEST
		src/gc_test.c
   )
set(LISP_SRC
		src/shell.c
		src/vm.c
		src/codegen.c
		src/hash.c
		src/parser.c
		src/mtddata.c
		src/memory.c
		src/cons.c
   )
set(LISP_BIN src/main.c)
set(GC_BIN src/memory.c)
add_library(lisp ${LISP_SRC})
add_executable(lisp ${LISP_BIN})
add_library(gc_test ${LISP_SRC})
add_executable(gc_test ${GC_TEST})

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

cmake_minimum_required(VERSION 2.6)
set(CMAKE_C_FLAGS_RELEASE "-g0 -W -Wall -O2")
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

set(LISP_BIN  src/main.c)
set(TEST_BIN  src/test.c)
set(LISP_SRC  src/shell.c
		      src/vm.c
		      src/codegen.c
		      src/hash.c
		      src/parser.c
		      src/mtddata.c
		      src/memory.c
		      src/cons.c
		      src/bootstrap.c)

add_library(lisp ${LISP_SRC})
add_executable(lisp1 ${LISP_BIN})
target_link_libraries(lisp1 lisp m)
add_executable(test1 ${TEST_BIN})
target_link_libraries(test1 lisp m)

set_target_properties(lisp1 PROPERTIES OUTPUT_NAME lisp)
set_target_properties(test1 PROPERTIES OUTPUT_NAME test)

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

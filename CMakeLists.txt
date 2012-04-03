cmake_minimum_required(VERSION 2.4)
set(CMAKE_C_FLAGS_RELEASE "-W -Wall -O2")
set(CMAKE_C_FLAGS_DEBUG "-g")
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

project(lisp)
set(serial "1.0")
include_directories("${PROJECT_SOURCE_DIR}/include")
set(LISP_SRC
		src/eval.cpp
		src/generator.cpp
		src/hash.cpp
		src/parser.cpp
   )
set(LISP_BIN src/main.cpp)
add_library(lisp SHARED ${LISP_SRC})
add_executable(lisp ${LISP_BIN})

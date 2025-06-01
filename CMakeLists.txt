cmake_minimum_required(VERSION 3.10)
project(htop_clone)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

include_directories(${PROJECT_SOURCE_DIR}/src)

file(GLOB_RECURSE SRC_FILES "src/*.cpp" "src/*.hpp")

add_executable(htop_clone ${SRC_FILES})

find_package(Curses REQUIRED)
if(CURSES_FOUND)
	include_directories(${CURSES_INCLUDE_DIR})
	target_link_libraries(htop_clone ${CURSES_LIBRARIES})
endif()

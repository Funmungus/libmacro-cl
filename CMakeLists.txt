cmake_minimum_required(VERSION 3.5)

# Use C++, do not use QT
set(MCR_NOEXTRAS OFF CACHE BOOL "Do not include extra functionality, or any C++." FORCE)
set(MCR_NOQT ON CACHE BOOL "Do not include any QT functionality." FORCE)
# Defines LIBMACRO_TARGET
add_subdirectory(libmacro)
# Defines MCR_VER, MCR_VER_MAJ, MCR_VER_MIN, MCR_DEBUG, MCR_PLATFORM
include(libmacro/libmacro.cmake)

project(libmacro-cl LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Include files in libmacro project
include_directories(libmacro)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Maybe C11 is not required for C++ application
#set(CMAKE_C_STANDARD 11)
#set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
#find_package(OpenSSL REQUIRED)

# Windows resolve libmacro at runtime, and make testing easier.
if (windows)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
	set_target_properties(libmacro
		PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
	)
endif (windows)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE mcr::Libmacro)
add_dependencies(${PROJECT_NAME} mcr::Libmacro)

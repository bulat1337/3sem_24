cmake_minimum_required(VERSION 3.16)

option(ENABLE_LOGGING "Enable logging" OFF)
set(ENABLE_LOGGING ${ENABLE_LOGGING} CACHE BOOL "Enable logging" FORCE)

project(server)

add_executable(server.x src/server.c main.c)

target_include_directories(server.x PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../util
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(ENABLE_LOGGING)
    target_compile_definitions(server.x PRIVATE ENABLE_LOGGING)
endif()

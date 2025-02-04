cmake_minimum_required(VERSION 3.16)

option(ENABLE_LOGGING "Enable logging" OFF)
set(ENABLE_LOGGING ${ENABLE_LOGGING} CACHE BOOL "Enable logging" FORCE)

project(client)

add_executable(client.x src/client.c main.c)

target_include_directories(client.x PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../util
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(ENABLE_LOGGING)
    target_compile_definitions(client.x PRIVATE ENABLE_LOGGING)
endif()

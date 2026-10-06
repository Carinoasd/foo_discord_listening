# foo_discord_listening
# Copyright (C) 2026 Carinoasd
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.25)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" FDL_VERSION)
string(STRIP "${FDL_VERSION}" FDL_VERSION)

project(foo_discord_listening VERSION ${FDL_VERSION} LANGUAGES CXX RC)

if(NOT MSVC)
    message(FATAL_ERROR "foo_discord_listening 只支援 MSVC 建置")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

include(cmake/deps.cmake)

set(FDL_SOURCES
    src/component.cpp
    src/config.cpp
    src/discord/activity.cpp
    src/discord/client.cpp
    src/discord/ipc_connection.cpp
    src/lifecycle.cpp
    src/playback_callback.cpp
    src/prefs_page.cpp
    src/presence.cpp
    src/foo_discord_listening.rc
)

add_library(foo_discord_listening SHARED ${FDL_SOURCES})
target_include_directories(foo_discord_listening PRIVATE src)
target_compile_definitions(foo_discord_listening PRIVATE
    UNICODE _UNICODE
    FDL_VERSION="${FDL_VERSION}"
)
target_compile_options(foo_discord_listening PRIVATE /W4 /utf-8 /permissive- /fp:fast)
target_link_libraries(foo_discord_listening PRIVATE fb2k_sdk nlohmann_json::nlohmann_json)
target_precompile_headers(foo_discord_listening PRIVATE src/stdafx.h)

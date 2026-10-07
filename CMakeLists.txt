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
    src/art/http.cpp
    src/art/local_art.cpp
    src/art/musicbrainz.cpp
    src/art/musicbrainz_query.cpp
    src/art/service.cpp
    src/art/uploader.cpp
    src/art/uploader_output.cpp
    src/component.cpp
    src/config.cpp
    src/discord/activity.cpp
    src/discord/client.cpp
    src/discord/ipc_connection.cpp
    src/lifecycle.cpp
    src/log.cpp
    src/menus.cpp
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
    FDL_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    FDL_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    FDL_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)
target_compile_options(foo_discord_listening PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4 /utf-8 /permissive- /fp:fast>)
target_link_libraries(foo_discord_listening PRIVATE fb2k_sdk nlohmann_json::nlohmann_json)
target_precompile_headers(foo_discord_listening PRIVATE src/stdafx.h)

# 本機測試工具：以元件同一份 IPC 程式碼探測 Discord（不進交付包）。
option(FDL_BUILD_TOOLS "Build local test tools" OFF)
if(FDL_BUILD_TOOLS)
    add_executable(ipc_probe tests/ipc_probe.cpp src/discord/ipc_connection.cpp src/discord/activity.cpp)
    target_include_directories(ipc_probe PRIVATE tests/win_stub src)
    target_compile_definitions(ipc_probe PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_compile_options(ipc_probe PRIVATE /W4 /utf-8 /permissive-)
    target_link_libraries(ipc_probe PRIVATE nlohmann_json::nlohmann_json)
endif()

# 第三方依賴：全部以固定版本與 SHA256 下載，不放進 repo。
# Copyright (C) 2026 Carinoasd
# SPDX-License-Identifier: MIT

include(FetchContent)

FetchContent_Declare(fb2k_sdk_src
    URL https://www.foobar2000.org/downloads/SDK-2026-10-01.7z
    URL_HASH SHA256=d4c55077336fae81bf8df0259b5b2748fa45ea84132c656ead93eb123cbcdc26
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Declare(wtl_src
    URL https://api.nuget.org/v3-flatcontainer/wtl/10.1.0/wtl.10.1.0.nupkg
    URL_HASH SHA256=28aabecf3f32ca6299e94f3d41043058114d8c341e47b3d9117dfe95da6af37d
    DOWNLOAD_NAME wtl.10.1.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Declare(json_src
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(JSON_Install OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(fb2k_sdk_src wtl_src json_src)

set(SDK_ROOT "${fb2k_sdk_src_SOURCE_DIR}")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(FDL_ARCH x64)
else()
    set(FDL_ARCH Win32)
endif()
set(WTL_INCLUDE "${wtl_src_SOURCE_DIR}/lib/native/include")

# SDK 各子專案編譯成靜態庫，檔案清單與 SDK 內附的 vcxproj 一致（pfc 另外排除非 Windows 的檔案）。
function(fdl_sdk_lib name dir)
    file(GLOB srcs "${SDK_ROOT}/${dir}/*.cpp")
    list(FILTER srcs EXCLUDE REGEX "_nix\\.cpp$")
    add_library(${name} STATIC ${srcs})
    target_include_directories(${name} PUBLIC "${SDK_ROOT}" "${SDK_ROOT}/foobar2000" "${WTL_INCLUDE}")
    target_compile_definitions(${name} PUBLIC UNICODE _UNICODE)
    target_compile_options(${name} PRIVATE /W0 /utf-8 /fp:fast /permissive-)
endfunction()

fdl_sdk_lib(fb2k_pfc pfc)
fdl_sdk_lib(fb2k_sdk_core foobar2000/SDK)
fdl_sdk_lib(fb2k_helpers foobar2000/helpers)
fdl_sdk_lib(fb2k_libppui libPPUI)
fdl_sdk_lib(fb2k_component_client foobar2000/foobar2000_component_client)

add_library(fb2k_sdk INTERFACE)
target_link_libraries(fb2k_sdk INTERFACE
    fb2k_component_client fb2k_helpers fb2k_libppui fb2k_sdk_core fb2k_pfc
    "${SDK_ROOT}/foobar2000/shared/shared-${FDL_ARCH}.lib"
)

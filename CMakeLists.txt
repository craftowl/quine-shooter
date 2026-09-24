cmake_minimum_required(VERSION 3.25)

project(2DShooting LANGUAGES C CXX)

# 構成を1つだけ持つジェネレータ（Makefiles など）で構成の指定がなければ、Debug にする
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT is_multi_config AND NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE が指定されていないので、Debug でビルドする")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "ビルドの構成（Debug、RelWithDebInfo、Release）" FORCE)
endif()

# 依存（ADR 0001）。SYSTEM を付けて、外部のヘッダでは警告を出さない
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.0
    GIT_SHALLOW TRUE
    SYSTEM
)
FetchContent_MakeAvailable(raylib)

add_executable(2DShooting
    src/main.cpp
    src/core/fixed_step.cpp
)

target_compile_features(2DShooting PRIVATE cxx_std_20)
set_target_properties(2DShooting PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(2DShooting PRIVATE src)

# デバッグ機能は Debug と RelWithDebInfo でだけ有効にする（AGENT.md）
target_compile_definitions(2DShooting PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:DEBUG>)

# 警告をエラーとして扱い、例外を無効にする（AGENT.md、ADR 0006）。自分のコードだけに付ける
if(MSVC)
    # MSVC の設定は、Windows 版の CI（T10）で確かめる
    target_compile_options(2DShooting PRIVATE /W4 /WX /EHs-c-)
    target_compile_definitions(2DShooting PRIVATE _HAS_EXCEPTIONS=0)
else()
    target_compile_options(2DShooting PRIVATE -Wall -Wextra -Wpedantic -Werror -fno-exceptions)
endif()

target_link_libraries(2DShooting PRIVATE raylib)

# Web 版（emcmake でビルドしたとき）。HTML を出力し、web/shell.html をひな形に使う
if(EMSCRIPTEN)
    set(WEB_SHELL ${CMAKE_SOURCE_DIR}/web/shell.html)
    set_target_properties(2DShooting PROPERTIES SUFFIX ".html")
    target_link_options(2DShooting PRIVATE --shell-file ${WEB_SHELL})
    set_property(TARGET 2DShooting APPEND PROPERTY LINK_DEPENDS ${WEB_SHELL})
endif()

# ソースの検査（AGENT.md の「検査」）。ビルドのたびに src/ を探し直し、違反があればビルドを止める
add_custom_target(check_sources ALL
    COMMAND ${CMAKE_COMMAND} -DPROJECT_ROOT=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/cmake/check_sources.cmake
    COMMENT "ソースを検査する"
    VERBATIM
)
add_dependencies(2DShooting check_sources)

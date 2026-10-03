cmake_minimum_required(VERSION 3.25)

project(2DShooting LANGUAGES C CXX)

# 構成を1つだけ持つジェネレータ（Makefiles など）で構成の指定がなければ、Debug にする
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT is_multi_config AND NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE が指定されていないので、Debug でビルドする")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "ビルドの構成（Debug、RelWithDebInfo、Release）" FORCE)
endif()

# MSVC では、C++ のランタイムを3つの構成すべてで静的にリンクする（Debug は /MTd、ほかは /MT。tasks.md の T10）。
# 開発ツールの入っていない Windows でも起動できるようにする。raylib などの依存もそろえる必要があるので、
# 依存を取り込む前に決める。MSVC 以外のコンパイラでは使われない
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

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

# Dear ImGui と rlImGui（ADR 0001）。どちらも CMake の設定を持たないので、ここでライブラリにする
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.9b
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    rlimgui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG Raylib_6_0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui rlimgui)

# nlohmann/json（ADR 0001）。例外を使わずに読む（JSON_NOEXCEPTION。ADR 0006）
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
    SYSTEM
)
FetchContent_MakeAvailable(nlohmann_json)

# Debug と RelWithDebInfo でだけリンクする（EXCLUDE_FROM_ALL なので、Release ではビルドもしない）
add_library(rlimgui STATIC EXCLUDE_FROM_ALL
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp
)
target_compile_features(rlimgui PRIVATE cxx_std_20)
# SYSTEM を付けて、外部のヘッダでは警告を出さない
target_include_directories(rlimgui SYSTEM PUBLIC ${imgui_SOURCE_DIR} ${rlimgui_SOURCE_DIR})
# アイコン用のフォント（Font Awesome）を埋め込まない
target_compile_definitions(rlimgui PUBLIC NO_FONT_AWESOME)
target_link_libraries(rlimgui PUBLIC raylib)
# 例外を無効にする（警告の設定は付けない。AGENT.md）
if(MSVC)
    target_compile_options(rlimgui PRIVATE /EHs-c-)
    target_compile_definitions(rlimgui PRIVATE _HAS_EXCEPTIONS=0)
else()
    target_compile_options(rlimgui PRIVATE -fno-exceptions)
endif()

add_executable(2DShooting
    src/main.cpp
    src/core/fixed_step.cpp
    src/core/input.cpp
    src/core/load_errors.cpp
    src/core/window.cpp
    src/debug/allocation_counter.cpp
    src/debug/debug_overlay.cpp
    src/debug/params_reloader.cpp
    src/game/game.cpp
    src/game/params.cpp
    src/game/player.cpp
    src/render/error_screen.cpp
    src/render/text_renderer.cpp
)

target_compile_features(2DShooting PRIVATE cxx_std_20)
set_target_properties(2DShooting PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(2DShooting PRIVATE src)

# デバッグ機能は Debug と RelWithDebInfo でだけ有効にする（AGENT.md）
target_compile_definitions(2DShooting PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:DEBUG>)

# 警告をエラーとして扱い、例外を無効にする（AGENT.md、ADR 0006）。自分のコードだけに付ける
if(MSVC)
    # /utf-8：ソースを UTF-8 として読む。日本語のコメントが、コードページ（日本語の Windows では 932）で
    # 表せない文字として警告（C4819）になり、/WX でビルドが止まるのを防ぐ
    target_compile_options(2DShooting PRIVATE /W4 /WX /EHs-c- /utf-8)
    target_compile_definitions(2DShooting PRIVATE _HAS_EXCEPTIONS=0)
    # Release だけ、コンソールのウィンドウを出さない（ウィンドウのアプリにする。tasks.md の T10）。
    # 入口は main() のままにするので、WinMain ではなく mainCRTStartup から始める
    set_target_properties(2DShooting PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
    target_link_options(2DShooting PRIVATE $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>)
else()
    target_compile_options(2DShooting PRIVATE -Wall -Wextra -Wpedantic -Werror -fno-exceptions)
endif()

target_link_libraries(2DShooting PRIVATE raylib nlohmann_json::nlohmann_json $<$<CONFIG:Debug,RelWithDebInfo>:rlimgui>)
target_compile_definitions(2DShooting PRIVATE JSON_NOEXCEPTION)

# Web 版（emcmake でビルドしたとき）。HTML を出力し、web/shell.html をひな形に使う
if(EMSCRIPTEN)
    set(WEB_SHELL ${CMAKE_SOURCE_DIR}/web/shell.html)
    set_target_properties(2DShooting PROPERTIES SUFFIX ".html")
    target_link_options(2DShooting PRIVATE --shell-file ${WEB_SHELL})
    set_property(TARGET 2DShooting APPEND PROPERTY LINK_DEPENDS ${WEB_SHELL})
endif()

# 素材（assets/）を、実行ファイルの場所から開けるところに置く（ADR 0006 §6）。
# デスクトップ版は実行ファイルの横へコピーし、Web 版は仮想ファイルシステムの /assets に入れる
file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/assets/*)
if(EMSCRIPTEN)
    target_link_options(2DShooting PRIVATE "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets@/assets")
    set_property(TARGET 2DShooting APPEND PROPERTY LINK_DEPENDS ${ASSET_FILES})
else()
    add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:2DShooting>/assets
        COMMENT "素材を実行ファイルの横へコピーする"
        VERBATIM
    )
    add_dependencies(2DShooting copy_assets)
endif()

# 調整値など（data/）を、実行ファイルの場所から開けるところに置く（ADR 0006 §6、tasks.md の T12）。
# Web 版は仮想ファイルシステムの /data に入れる（ホットリロードはしない）。
# デスクトップ版は、Debug と RelWithDebInfo では、ホットリロードで編集したファイルをそのまま読めるように、
# 実行ファイルの横の data/ をリポジトリの data/ へのシンボリックリンクにする。Release はコピーする
file(GLOB_RECURSE DATA_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/data/*)
if(EMSCRIPTEN)
    target_link_options(2DShooting PRIVATE "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/data@/data")
    set_property(TARGET 2DShooting APPEND PROPERTY LINK_DEPENDS ${DATA_FILES})
else()
    # ビルドのたびに、実行ファイルの横の data/ を消して置き直す。ソースのディレクトリでビルドすると、
    # 実行ファイルの横の data/ がリポジトリの data/ そのものになり、消してしまうので止める
    if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
        message(FATAL_ERROR "ソースのディレクトリではビルドしない。-B で別のディレクトリを指定する（README）")
    endif()
    # シンボリックリンクを作れるかを、設定の段階で確かめる（Windows は開発者モードでないと作れない）
    set(DATA_LINK_TEST ${CMAKE_BINARY_DIR}/data_link_test)
    file(REMOVE_RECURSE ${DATA_LINK_TEST})
    file(CREATE_LINK ${CMAKE_SOURCE_DIR}/data ${DATA_LINK_TEST} RESULT DATA_LINK_RESULT SYMBOLIC)
    if(DATA_LINK_RESULT EQUAL 0)
        file(REMOVE_RECURSE ${DATA_LINK_TEST})
        # リンクかコピーかは、構成を複数持つジェネレータのために、ビルドの段階で選ぶ
        set(DATA_PLACE_COMMAND $<IF:$<CONFIG:Debug,RelWithDebInfo>,create_symlink,copy_directory>)
    else()
        message(WARNING "data/ のシンボリックリンクを作れないので、どの構成でもコピーにする（${DATA_LINK_RESULT}）。"
                        "ホットリロードで編集するのは build/ の中のコピーになり、再ビルドで上書きされる")
        set(DATA_PLACE_COMMAND copy_directory)
    endif()
    # 置く前に、前の構成で置いたリンクかディレクトリを消す。消すパスの末尾に / を付けない
    # （付けると、リンクのときにリンク先のリポジトリの data/ まで消える）
    add_custom_target(place_data ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:2DShooting>
        COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:2DShooting>/data
        COMMAND ${CMAKE_COMMAND} -E ${DATA_PLACE_COMMAND} ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:2DShooting>/data
        COMMENT "data/ を実行ファイルの横へ置く"
        VERBATIM
    )
    add_dependencies(2DShooting place_data)
endif()

# ソースの検査（AGENT.md の「検査」）。ビルドのたびに src/ を探し直し、違反があればビルドを止める
add_custom_target(check_sources ALL
    COMMAND ${CMAKE_COMMAND} -DPROJECT_ROOT=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/cmake/check_sources.cmake
    COMMENT "ソースを検査する"
    VERBATIM
)
add_dependencies(2DShooting check_sources)

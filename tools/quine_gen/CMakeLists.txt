# ボスの Quine の生成ツール（ADR 0009、tasks.md の T15c2）。ゲームとは別の小さなプロジェクトで、raylib を取りに行かない。
# 作り方: cmake -S tools/quine_gen -B build/quine_gen && cmake --build build/quine_gen（README）
cmake_minimum_required(VERSION 3.25)

project(quine_gen LANGUAGES CXX)

add_executable(quine_gen quine_gen.cpp)
target_compile_features(quine_gen PRIVATE cxx_std_20)
set_target_properties(quine_gen PROPERTIES CXX_EXTENSIONS OFF)

# ゲームと同じく、警告をエラーとして扱い、例外を無効にする（AGENT.md、ADR 0006）
if(MSVC)
    # /utf-8 はゲームと同じ理由（日本語のコメント）。std::fopen を使うので、MSVC の「安全でない関数」の警告（C4996）は
    # 出さない（使う関数は AGENT.md の規則のとおり std::fopen だけ）
    target_compile_options(quine_gen PRIVATE /W4 /WX /EHs-c- /utf-8)
    target_compile_definitions(quine_gen PRIVATE _HAS_EXCEPTIONS=0 _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(quine_gen PRIVATE -Wall -Wextra -Wpedantic -Werror -fno-exceptions)
endif()

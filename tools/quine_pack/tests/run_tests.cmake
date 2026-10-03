# quine_pack のテスト（tasks.md の T15a）
#
# 使い方: cmake [-DWORK_DIR=<作業用のディレクトリ>] -P tools/quine_pack/tests/run_tests.cmake
#
# このディレクトリの下のディレクトリを1つずつ、quine_pack に渡して確かめる（enemies/ と masks/ が入力）。
#   expected_errors.txt があるもの：quine_pack が失敗し、各行の文がメッセージに含まれ、生成物を書かないこと
#   ないもの：quine_pack が成功すること。expected.cpp があれば、生成物が1字も違わないこと
# WORK_DIR を省くと、リポジトリの build/quine_pack_tests に生成物を書く（.gitignore の対象）。
# WORK_DIR の中で消すのは、テストごとの生成物（<テスト名>.cpp）だけ。ほかのファイルには触らない。
# 1つでも合わなければ失敗する。

cmake_minimum_required(VERSION 3.25)

set(TESTS_DIR "${CMAKE_CURRENT_LIST_DIR}")
get_filename_component(QUINE_PACK "${TESTS_DIR}/../quine_pack.cmake" ABSOLUTE)
if(NOT DEFINED WORK_DIR)
    get_filename_component(WORK_DIR "${TESTS_DIR}/../../../build/quine_pack_tests" ABSOLUTE)
endif()
file(MAKE_DIRECTORY "${WORK_DIR}")

file(GLOB entries LIST_DIRECTORIES true "${TESTS_DIR}/*")
list(SORT entries)

set(failures "")
set(failure_count 0)
set(case_count 0)

foreach(case_dir IN LISTS entries)
    # enemies/ か masks/ を持つディレクトリだけをテストとみなす（ほかのツールが作ったディレクトリを数えない）
    if(NOT IS_DIRECTORY "${case_dir}/enemies" AND NOT IS_DIRECTORY "${case_dir}/masks")
        continue()
    endif()
    get_filename_component(name "${case_dir}" NAME)
    math(EXPR case_count "${case_count} + 1")
    set(output "${WORK_DIR}/${name}.cpp")
    # 前の回の生成物を消す（失敗したのに生成物を書いたかを、正しく確かめるため）
    file(REMOVE "${output}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            "-DENEMY_DIR=${case_dir}/enemies"
            "-DMASK_DIR=${case_dir}/masks"
            "-DOUTPUT=${output}"
            "-DBASE_DIR=${case_dir}"
            -P "${QUINE_PACK}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
    )

    set(problems "")
    if(EXISTS "${case_dir}/expected_errors.txt")
        if(result EQUAL 0)
            string(APPEND problems "    失敗するはずが成功した\n")
        endif()
        file(STRINGS "${case_dir}/expected_errors.txt" expected_lines ENCODING UTF-8)
        foreach(expected IN LISTS expected_lines)
            string(FIND "${stderr}" "${expected}" found)
            if(found EQUAL -1)
                string(APPEND problems "    メッセージに出ていない: ${expected}\n")
            endif()
        endforeach()
        if(EXISTS "${output}")
            string(APPEND problems "    失敗したのに生成物を書いた\n")
        endif()
    else()
        if(NOT result EQUAL 0)
            string(APPEND problems "    成功するはずが失敗した\n${stderr}\n")
        elseif(EXISTS "${case_dir}/expected.cpp")
            file(READ "${case_dir}/expected.cpp" expected_cpp)
            file(READ "${output}" actual_cpp)
            if(NOT expected_cpp STREQUAL actual_cpp)
                string(APPEND problems "    生成物が expected.cpp と違う（diff ${case_dir}/expected.cpp ${output}）\n")
            endif()
        endif()
    endif()

    if(problems STREQUAL "")
        message(STATUS "ok  ${name}")
    else()
        message(STATUS "NG  ${name}")
        string(APPEND failures "  ${name}\n${problems}")
        math(EXPR failure_count "${failure_count} + 1")
    endif()
endforeach()

if(failure_count GREATER 0)
    message(FATAL_ERROR "quine_pack のテスト: ${case_count} 件のうち ${failure_count} 件が合わない\n${failures}")
endif()
message(STATUS "quine_pack のテスト: ${case_count} 件すべて合った")

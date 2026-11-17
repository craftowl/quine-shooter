# ボスの Quine を確かめる（ADR 0009、tasks.md の T15c2）。CI（web.yml）と手元で同じ確かめをする
#
# 使い方（リポジトリの直下で）: cmake -P tools/quine_gen/check_quine.cmake
#
# 1. 生成ツールを build/quine_gen に作る（README の手順と同じ場所に、assert が働く Debug で作る）
# 2. quines/ の .cpp ごとに（.editorconfig などは見ない）、次の2つを確かめる
#    - data/masks/<敵ID>.txt から生成し直すと、元のファイルと1バイトも違わない
#    - ADR 0009 の警告の設定でコンパイルして実行すると、元のファイルと1バイトも違わないものを出力する
# コンパイラは c++（macOS では clang、ubuntu では g++）。一時的なファイルは build/quine_gen/check/ に置く。
# 1つでも合わなければ、どのファイルのどの確かめで合わなかったかをすべて出して失敗する。macOS と Linux で動かす。

cmake_minimum_required(VERSION 3.25)

get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(TOOL_DIR "${ROOT}/build/quine_gen")
set(TOOL "${TOOL_DIR}/quine_gen")
set(WORK_DIR "${TOOL_DIR}/check")
# ゲームと同じ設定に、形に並べることで避けられない字下げの警告を外すものだけを足す（ADR 0009 の決定5）
set(QUINE_FLAGS -std=c++20 -Wall -Wextra -Wpedantic -Werror -fno-exceptions -Wno-misleading-indentation)
# Quine を実行するときの時間の上限（秒）。壊れた Quine が止まらなくても、CI が待ち続けないようにする
set(RUN_TIMEOUT_SECONDS 10)

# どこかで失敗したら、その場で止める（生成ツールを作るところ）
function(run_or_fail what)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "check_quine: ${what}に失敗した（${result}）")
    endif()
endfunction()

message(STATUS "check_quine: 生成ツールを作る")
# build/quine_gen は README の手順と同じ場所なので、前の設定（Release など）が残っていても、assert が働く Debug に決め直す
run_or_fail("生成ツールの設定" "${CMAKE_COMMAND}" -S "${ROOT}/tools/quine_gen" -B "${TOOL_DIR}" -DCMAKE_BUILD_TYPE=Debug)
run_or_fail("生成ツールのビルド" "${CMAKE_COMMAND}" --build "${TOOL_DIR}")

find_program(QUINE_CXX c++ REQUIRED)
execute_process(COMMAND "${QUINE_CXX}" --version OUTPUT_VARIABLE version_text ERROR_QUIET)
string(REGEX MATCH "^[^\n]*" version_line "${version_text}")
message(STATUS "check_quine: Quine をコンパイルするコンパイラ: ${QUINE_CXX}（${version_line}）")

file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")
file(GLOB quine_files LIST_DIRECTORIES false "${ROOT}/quines/*.cpp")
list(SORT quine_files)

set(failures "")
set(failure_count 0)
set(quine_count 0)
foreach(path IN LISTS quine_files)
    get_filename_component(id "${path}" NAME_WLE)
    set(rel "quines/${id}.cpp")
    set(mask "${ROOT}/data/masks/${id}.txt")
    math(EXPR quine_count "${quine_count} + 1")
    set(problems "")

    # 生成し直して比べる
    if(NOT EXISTS "${mask}")
        string(APPEND problems "    生成し直し: マスクがない（data/masks/${id}.txt）\n")
    else()
        execute_process(
            COMMAND "${TOOL}" "${mask}" "${WORK_DIR}/${id}.cpp"
            RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
        )
        if(NOT result EQUAL 0)
            string(APPEND problems "    生成し直し: 生成ツールが失敗した（${result}）\n${stderr}")
        else()
            execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files "${path}" "${WORK_DIR}/${id}.cpp"
                            RESULT_VARIABLE result)
            if(NOT result EQUAL 0)
                string(APPEND problems "    生成し直し: data/masks/${id}.txt から生成し直したものと合わない（${WORK_DIR}/${id}.cpp）\n")
            endif()
        endif()
    endif()

    # コンパイルして実行し、出力を比べる
    execute_process(
        COMMAND "${QUINE_CXX}" ${QUINE_FLAGS} "${path}" -o "${WORK_DIR}/${id}"
        RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
    )
    if(NOT result EQUAL 0)
        string(APPEND problems "    コンパイル: 警告かエラーが出た（${result}）\n${stderr}")
    else()
        execute_process(
            COMMAND "${WORK_DIR}/${id}"
            RESULT_VARIABLE result OUTPUT_FILE "${WORK_DIR}/${id}.out"
            TIMEOUT ${RUN_TIMEOUT_SECONDS}
        )
        # 時間切れのとき、result は終了コードではなく、timeout を含む文字列になる
        if(result MATCHES "timeout")
            string(APPEND problems "    実行: ${RUN_TIMEOUT_SECONDS} 秒たっても終わらない\n")
        elseif(NOT result EQUAL 0)
            string(APPEND problems "    実行: 終了コードが 0 でない（${result}）\n")
        else()
            execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files "${path}" "${WORK_DIR}/${id}.out"
                            RESULT_VARIABLE result)
            if(NOT result EQUAL 0)
                string(APPEND problems "    実行: 出力が自分と合わない（${WORK_DIR}/${id}.out）\n")
            endif()
        endif()
    endif()

    if(problems STREQUAL "")
        message(STATUS "ok  ${rel}（生成し直しと、コンパイル・実行の出力が一致した）")
    else()
        message(STATUS "NG  ${rel}")
        string(APPEND failures "  ${rel}\n${problems}")
        math(EXPR failure_count "${failure_count} + 1")
    endif()
endforeach()

if(failure_count GREATER 0)
    message(FATAL_ERROR "check_quine: ${quine_count} 件のうち ${failure_count} 件が合わない\n${failures}")
endif()
message(STATUS "check_quine: ${quine_count} 件すべて合った")

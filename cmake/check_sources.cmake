# ソースの検査（AGENT.md の「検査」、tasks.md の T02）
#
# 使い方: cmake -DPROJECT_ROOT=<プロジェクトの場所> -P cmake/check_sources.cmake
#
# src/ の .h と .cpp を毎回探し直し、次を検査する。違反があればファイル名と行番号を出して失敗する。
#   1. プラットフォームを判定するマクロが、main.cpp の1つの #if（#elif と #else を含む）の外にある
#   2. 使わない関数（ADR 0006 の表のうち、確かめ方が「スクリプト」の行）を使っている
#   3. 入力・時間・乱数の関数（AGENT.md の「構造」の一覧）を、main.cpp、src/core/、src/debug/ の外で使っている
#   4. QUINE マーカー（行の先頭の // QUINE-BEGIN と // QUINE-END）を、src/enemies/ の外に書いている（tasks.md の T15b）。
#      マーカーはコメントなので、この検査だけは、コメントを取り除く前の行で行う。
#      マーカーと認める行は、quine_pack（tools/quine_pack/quine_pack.cmake）と同じ
# 500行を超えたファイルには警告だけを出す（wc -l と同じく、改行の数で数える）。
# コメントと、文字列と文字のリテラルの中身は、検査の前に取り除く（生文字列 R"(...)" は扱わない）。

cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED PROJECT_ROOT)
    message(FATAL_ERROR "PROJECT_ROOT を指定する")
endif()

set(MAX_LINES 500)

# QUINE マーカーと認める行（quine_pack の MARKER_REGEX と同じ）
set(QUINE_MARKER_REGEX "^[ \t]*//[ \t]*QUINE-(BEGIN|END)([^A-Za-z0-9_-]|$)")

# 識別子の一部でないことを表す、前後の文字
set(B "(^|[^A-Za-z0-9_])")
set(E "([^A-Za-z0-9_]|$)")
set(WS "[ \t]*")

set(PLATFORM_MACROS "PLATFORM_WEB|__EMSCRIPTEN__|_WIN32|_MSC_VER|__MINGW32__|__APPLE__|__linux__")
set(PLATFORM_REGEX "${B}(${PLATFORM_MACROS})${E}")

# 使わない関数（ADR 0006）
set(TO_NUMBER_FUNCS "stoi|stol|stoll|stoul|stoull|stof|stod|stold|atoi|atol|atoll|atof|strtol|strtoll|strtoul|strtoull|strtof|strtod|strtold")
set(FORBIDDEN_RULES
    "(\\.|->)${WS}at${WS}\\(|.at()"
    "(\\.|->)${WS}value${WS}\\(|.value()"
    "std${WS}::${WS}get${WS}[<(]|std::get"
    "(^|[^A-Za-z0-9_:])get${WS}<|get<（std:: なしの std::get）"
    "${B}(${TO_NUMBER_FUNCS})${WS}\\(|文字列を数値に変える関数"
    "std${WS}::${WS}filesystem|std::filesystem"
    "#${WS}include${WS}<filesystem>|<filesystem>"
)
# nlohmann/json の j.get<T>() は許す（ADR 0006 で、レビューで確かめることになっている）
set(ALLOWED_MEMBER_GET "(\\.|->)${WS}(template[ \t]+)?get${WS}<")

# 入力・時間・乱数の関数（AGENT.md の「構造」の一覧）
set(INPUT_FUNCS "IsKey[A-Za-z0-9_]*|GetKeyPressed|GetCharPressed|IsMouse[A-Za-z0-9_]*|GetMouse[A-Za-z0-9_]*|IsGamepad[A-Za-z0-9_]*|GetGamepad[A-Za-z0-9_]*")
set(RESTRICTED_RULES
    "${B}(${INPUT_FUNCS})${WS}\\(|入力の関数"
    "${B}(GetFrameTime|GetTime|time|clock)${WS}\\(|時間の関数"
    "(std${WS}::${WS}chrono${E}|${B}chrono${WS}::)|std::chrono"
    "#${WS}include${WS}<(chrono|ctime|time\\.h)>|時間のヘッダ"
    "${B}(GetRandomValue|rand|srand)${WS}\\(|乱数の関数"
    "${B}random_device${E}|std::random_device"
    "#${WS}include${WS}<random>|<random>"
)
# メンバー関数の time() と clock()（stage.time() など）は対象外
set(ALLOWED_MEMBER_TIME "(\\.|->)${WS}(time|clock)${WS}\\(")

set(violations "")
set(violation_count 0)

function(add_violation rel line_no kind text)
    string(STRIP "${text}" text)
    set(violations "${violations}  ${rel}:${line_no}: ${kind}: ${text}\n" PARENT_SCOPE)
    math(EXPR count "${violation_count} + 1")
    set(violation_count ${count} PARENT_SCOPE)
endfunction()

# コメントと、文字列と文字のリテラルの中身を取り除く。行番号が変わらないように、改行は残す
function(strip_comments_and_literals input out_var)
    set(rest "${input}")
    set(out "")
    string(LENGTH "${rest}" n)
    while(n GREATER 0)
        string(REGEX MATCH "^[^/\"']+" plain "${rest}")
        string(LENGTH "${plain}" plain_len)
        if(plain_len GREATER 0)
            string(APPEND out "${plain}")
            string(SUBSTRING "${rest}" ${plain_len} -1 rest)
        else()
            string(SUBSTRING "${rest}" 0 2 head2)
            string(SUBSTRING "${rest}" 0 1 head1)
            if(head2 STREQUAL "//")
                string(FIND "${rest}" "\n" nl)
                if(nl EQUAL -1)
                    set(rest "")
                else()
                    string(SUBSTRING "${rest}" ${nl} -1 rest)
                endif()
            elseif(head2 STREQUAL "/*")
                string(SUBSTRING "${rest}" 2 -1 body)
                string(FIND "${body}" "*/" close)
                if(close EQUAL -1)
                    set(comment "${rest}")
                    set(rest "")
                else()
                    math(EXPR comment_len "${close} + 4")
                    string(SUBSTRING "${rest}" 0 ${comment_len} comment)
                    string(SUBSTRING "${rest}" ${comment_len} -1 rest)
                endif()
                string(REGEX REPLACE "[^\n]" "" newlines "${comment}")
                string(APPEND out " ${newlines}")
            elseif(head1 STREQUAL "\"")
                string(REGEX MATCH "^\"([^\"\\\\\n]|\\\\.)*\"" literal "${rest}")
                string(LENGTH "${literal}" literal_len)
                if(literal_len EQUAL 0)
                    string(APPEND out "\"")
                    string(SUBSTRING "${rest}" 1 -1 rest)
                else()
                    string(REGEX REPLACE "[^\n]" "" newlines "${literal}")
                    string(APPEND out "\"${newlines}\"")
                    string(SUBSTRING "${rest}" ${literal_len} -1 rest)
                endif()
            elseif(head1 STREQUAL "'")
                # 直前が英数字なら、数値の桁区切り（1'000）とみなす
                string(REGEX MATCH "[A-Za-z0-9_]$" after_word "${out}")
                if(NOT after_word STREQUAL "")
                    set(literal "")
                else()
                    string(REGEX MATCH "^'([^'\\\\\n]|\\\\.)*'" literal "${rest}")
                endif()
                string(LENGTH "${literal}" literal_len)
                if(literal_len EQUAL 0)
                    string(APPEND out "'")
                    string(SUBSTRING "${rest}" 1 -1 rest)
                else()
                    string(APPEND out "''")
                    string(SUBSTRING "${rest}" ${literal_len} -1 rest)
                endif()
            else()
                string(APPEND out "/")
                string(SUBSTRING "${rest}" 1 -1 rest)
            endif()
        endif()
        string(LENGTH "${rest}" n)
    endwhile()
    set(${out_var} "${out}" PARENT_SCOPE)
endfunction()

file(GLOB_RECURSE source_files LIST_DIRECTORIES false
    "${PROJECT_ROOT}/src/*.h"
    "${PROJECT_ROOT}/src/*.cpp"
)
list(SORT source_files)

foreach(path IN LISTS source_files)
    file(RELATIVE_PATH rel "${PROJECT_ROOT}" "${path}")
    file(READ "${path}" content)

    # 4. src/enemies/ の外の QUINE マーカー。コメントを取り除く前の行で探す
    if(NOT rel MATCHES "^src/enemies/")
        set(raw "${content}")
        string(REPLACE ";" " " raw "${raw}")
        string(REPLACE "[" " " raw "${raw}")
        string(REPLACE "]" " " raw "${raw}")
        string(REPLACE "\n" ";" raw_lines "${raw}")
        set(raw_line_no 0)
        foreach(raw_line IN LISTS raw_lines)
            math(EXPR raw_line_no "${raw_line_no} + 1")
            if(raw_line MATCHES "${QUINE_MARKER_REGEX}")
                add_violation("${rel}" ${raw_line_no} "QUINE マーカーは src/enemies/ の中にだけ書く（AGENT.md の「Quine の敵」）" "${raw_line}")
            endif()
        endforeach()
    endif()

    # 行数（wc -l と同じく、改行の数）
    string(REGEX REPLACE "[^\n]" "" all_newlines "${content}")
    string(LENGTH "${all_newlines}" line_count)
    if(line_count GREATER MAX_LINES)
        message(WARNING "${rel}: ${line_count} 行（${MAX_LINES} 行を超えている。分割を検討する）")
    endif()

    strip_comments_and_literals("${content}" clean)
    # リストに分けるときに邪魔になる文字を空白に置き換える（検査の対象の形には含まれない）
    string(REPLACE ";" " " clean "${clean}")
    string(REPLACE "[" " " clean "${clean}")
    string(REPLACE "]" " " clean "${clean}")
    string(REPLACE "\n" ";" lines "${clean}")

    set(is_main FALSE)
    if(rel STREQUAL "src/main.cpp")
        set(is_main TRUE)
    endif()
    set(may_call_restricted FALSE)
    if(is_main OR rel MATCHES "^src/(core|debug)/")
        set(may_call_restricted TRUE)
    endif()

    # main.cpp のプラットフォームの #if の状態
    set(pp_depth 0)
    set(platform_if_count 0)
    set(platform_group_depth -1)

    set(line_no 0)
    foreach(line IN LISTS lines)
        math(EXPR line_no "${line_no} + 1")

        # 1. プラットフォームを判定するマクロ
        set(opens_if FALSE)
        set(closes_if FALSE)
        if(line MATCHES "^[ \t]*#[ \t]*(if|ifdef|ifndef)${E}")
            set(opens_if TRUE)
        elseif(line MATCHES "^[ \t]*#[ \t]*endif${E}")
            set(closes_if TRUE)
        endif()
        if(opens_if)
            math(EXPR pp_depth "${pp_depth} + 1")
        endif()
        if(line MATCHES "${PLATFORM_REGEX}")
            if(NOT is_main)
                add_violation("${rel}" ${line_no} "プラットフォームのマクロは main.cpp の先頭の #if の中だけに書く" "${line}")
            elseif(opens_if)
                math(EXPR platform_if_count "${platform_if_count} + 1")
                if(platform_if_count EQUAL 1)
                    set(platform_group_depth ${pp_depth})
                else()
                    add_violation("${rel}" ${line_no} "プラットフォームのマクロを使う #if が2つ以上ある" "${line}")
                endif()
            elseif(platform_group_depth EQUAL -1)
                add_violation("${rel}" ${line_no} "プラットフォームのマクロが、main.cpp の #if の外にある" "${line}")
            endif()
        endif()
        if(closes_if)
            if(pp_depth EQUAL platform_group_depth)
                set(platform_group_depth -1)
            endif()
            if(pp_depth GREATER 0)
                math(EXPR pp_depth "${pp_depth} - 1")
            endif()
        endif()

        # 2. 使わない関数（ADR 0006）
        string(REGEX REPLACE "${ALLOWED_MEMBER_GET}" " " line_for_forbidden "${line}")
        foreach(rule IN LISTS FORBIDDEN_RULES)
            string(FIND "${rule}" "|" sep REVERSE)
            string(SUBSTRING "${rule}" 0 ${sep} pattern)
            math(EXPR name_start "${sep} + 1")
            string(SUBSTRING "${rule}" ${name_start} -1 name)
            if(line_for_forbidden MATCHES "${pattern}")
                add_violation("${rel}" ${line_no} "使わない関数（ADR 0006）: ${name}" "${line}")
            endif()
        endforeach()

        # 3. 入力・時間・乱数の関数
        if(NOT may_call_restricted)
            string(REGEX REPLACE "${ALLOWED_MEMBER_TIME}" " " line_for_restricted "${line}")
            foreach(rule IN LISTS RESTRICTED_RULES)
                string(FIND "${rule}" "|" sep REVERSE)
                string(SUBSTRING "${rule}" 0 ${sep} pattern)
                math(EXPR name_start "${sep} + 1")
                string(SUBSTRING "${rule}" ${name_start} -1 name)
                if(line_for_restricted MATCHES "${pattern}")
                    add_violation("${rel}" ${line_no} "入力・時間・乱数の関数は main.cpp、src/core/、src/debug/ でだけ使う: ${name}" "${line}")
                endif()
            endforeach()
        endif()
    endforeach()
endforeach()

list(LENGTH source_files file_count)
if(violation_count GREATER 0)
    message(FATAL_ERROR "ソースの検査: ${violation_count} 件の違反（${file_count} ファイルを検査した）\n${violations}")
endif()
message(STATUS "ソースの検査: 違反なし（${file_count} ファイルを検査した）")

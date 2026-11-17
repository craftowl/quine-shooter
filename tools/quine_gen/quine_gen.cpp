// quine_gen：マスクから、ボスの体にする C++ の Quine を生成する（ADR 0009、tasks.md の T15c2）
//
// 使い方: quine_gen <マスク> <出力>（例：quine_gen data/masks/boss_dummy.txt quines/boss_dummy.cpp）
//
// できあがるプログラム F の並び（T15q の試作と同じ方式）：
//   F = P + R"( + D + )" + Q
//   D = P + R"( + @ + Q + @ + M + 余りを埋める *
// P と Q は自分を出力するコード（出力部）、M はマスクのランレングス。F をマスクの # に左上から1文字ずつ置き、
// . には空白、$ には $ を置く。出力部は、生文字列から空白と改行と $ を取り除いて D を取り出し、F を組み立て直して、
// M のとおりに並べて出力する。
// 出力部の字句が、穴や行の切れ目や $ をまたがないように、コメント /*…*/ を詰め物として入れる（コメントの中は
// またいでよい）。詰め方は、いちばん短く終わるものを探す。D は生文字列の中なので、どこで切れてもよい。
// 失敗したら、理由を英語で標準エラーに出し、終了コード 1 で終わる。出力のファイルは、生成がすべて成功してから開く。

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace stg {
namespace {

// マスクの大きさの上限（ボス。ADR 0002 のルール6）
constexpr int MAX_COLUMNS = 60;
constexpr int MAX_ROWS = 24;

// 出力部の前半と後半。字句を空白で区切って書く（空白は、つなぐときに取り除く）。Q の COLUMNS は、マスクの列数に置き換える。
// gcc の -Wparentheses を出さないように、数える変数は int(c)=0 のような宣言にせず、ポインタにする（g と a）。
// clang の -Wempty-body を出さないように、空の for は書かない。Q の手順：
//   1. s から空白と改行と $（36）を取り除き、D にする（d は D の終わり）
//   2. 1つ目と2つ目の @（64）を a と b にする
//   3. f に F を組み立てる：P と R"(（s から a まで）、D、)"（41 と 34）、Q（a と b の間）
//   4. M（b のあと）のとおりに出力する。英大文字は F の文字、英小文字は空白、数字は $ で、数は文字の値から求める。
//      COLUMNS マスごとに改行する。余りを埋める *（42）は、数が負になるので何も出さない
constexpr std::string_view P_CODE = "extern \"C\" { int ( putchar ) ( int ) ; } char ( s ) [ ] =";
constexpr std::string_view Q_CODE =
    "; int ( main ) ( ) { char * d = s , * e = s , f [ 2048 ] , * k = f , * a , * b , * g = f ; "
    "for ( ; * e ; e ++ ) if ( * e > 32 && * e - 36 ) * d ++ = * e ; "
    "for ( a = s ; * a - 64 ; ) a ++ ; for ( b = a + 1 ; * b - 64 ; ) b ++ ; "
    "for ( e = s ; e < a ; ) * k ++ = * e ++ ; for ( e = s ; e < d ; ) * k ++ = * e ++ ; "
    "* k ++ = 41 ; * k ++ = 34 ; for ( e = a + 1 ; e < b ; ) * k ++ = * e ++ ; "
    "for ( k = f , e = b + 1 ; e < d ; e ++ ) for ( a = f ; a - f < ( * e > 64 ? * e % 32 : * e - 48 ) ; a ++ ) "
    "{ putchar ( * e > 96 ? 32 : * e > 64 ? * k ++ : 36 ) ; if ( ( ++ g - f ) % COLUMNS < 1 ) putchar ( 10 ) ; } }";
// Q の f [ 2048 ] には F（マスクの # の数と同じ字数）が入る。また g は f から、出力したマスの数（. と $ も含む）だけ進む。
// どちらも全マスの数（列数 × 行数）を超えないので、それが 2048 以下であることを確かめる。
// マスクの上限を上げて収まらなくなったら、ここで止める
static_assert(Q_CODE.find("f [ 2048 ]") != std::string_view::npos);
static_assert(MAX_COLUMNS * MAX_ROWS <= 2048);
constexpr std::string_view RAW_OPEN = "R\"(";
constexpr std::string_view RAW_CLOSE = ")\"";
// 字句が入る横の並びの長さ（いちばん長い字句 putchar の長さ。エラーのメッセージに出す）
constexpr int LONGEST_TOKEN = 7;
// ランレングスの1文字で表せる数（# と . は英字、$ は数字）
constexpr int MAX_LETTER_RUN = 26;
constexpr int MAX_DIGIT_RUN = 9;

struct Mask {
    int columns = 0;
    int rows = 0;
    std::string cells;  // 行をつなげたもの（columns * rows 文字）
};

// 生成した Quine と、ログに出す数字
struct Quine {
    std::string source;
    int hash_cells = 0;
    int weak_cells = 0;
    int p_length = 0;  // P（詰め物のコメントを含む）
    int q_length = 0;  // Q（詰め物のコメントを含む）
    int m_length = 0;
    int comment_padding = 0;  // P と Q に入れた詰め物のコメントの文字数
    int spare = 0;            // D の余りを埋めた * の数
};

void report(const char* path, const std::string& reason) {
    std::fprintf(stderr, "quine_gen: error: %s: %s\n", path, reason.c_str());
}

// 読み書きの失敗を、理由（errno の文。例：No such file or directory）を付けて出す。
// error は、失敗した関数のすぐあとに控えた errno（あとの fclose などで書き換わるため）。
// 呼ぶ側は、関数を呼ぶ前に errno を 0 にしておく（前の失敗の理由を出さないため）。
// 関数が errno を変えずに失敗したとき（error が 0）は、理由を付けない
void report_io_error(const char* path, const char* what, int error) {
    report(path, error != 0 ? std::string(what) + ": " + std::strerror(error) : std::string(what));
}

[[nodiscard]] std::optional<std::string> read_file(const char* path) {
    errno = 0;
    std::FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        report_io_error(path, "cannot open the mask", errno);
        return std::nullopt;
    }
    std::string text;
    char buffer[4096];
    std::size_t count = 0;
    errno = 0;
    while ((count = std::fread(buffer, 1, sizeof(buffer), file)) > 0) {
        text.append(buffer, count);
    }
    const bool failed = std::ferror(file) != 0;
    const int read_error = errno;
    std::fclose(file);
    if (failed) {
        report_io_error(path, "cannot read the mask", read_error);
        return std::nullopt;
    }
    return text;
}

[[nodiscard]] bool write_file(const char* path, const std::string& text) {
    errno = 0;
    std::FILE* file = std::fopen(path, "wb");
    if (file == nullptr) {
        report_io_error(path, "cannot open the output for writing", errno);
        return false;
    }
    errno = 0;
    const bool written = std::fwrite(text.data(), 1, text.size(), file) == text.size();
    const int write_error = errno;
    errno = 0;
    const bool closed = std::fclose(file) == 0;
    const int close_error = errno;
    if (!written || !closed) {
        report_io_error(path, "cannot write the output", written ? close_error : write_error);
        return false;
    }
    return true;
}

// quine_pack と同じ規則で読む（# と $ と . だけ、行の長さがそろう、行末の CR は無視する、60列×24行まで）
[[nodiscard]] std::optional<Mask> parse_mask(std::string_view text, const char* path) {
    Mask mask;
    bool ok = true;
    std::size_t position = 0;
    while (position < text.size()) {
        std::size_t end = text.find('\n', position);
        if (end == std::string_view::npos) {
            end = text.size();
        }
        assert(position <= end && end <= text.size());
        std::string_view row = text.substr(position, end - position);
        position = end + 1;
        if (!row.empty() && row.back() == '\r') {
            row.remove_suffix(1);
        }
        ++mask.rows;
        const std::string where = "row " + std::to_string(mask.rows);
        const std::size_t bad = row.find_first_not_of("#$.");
        if (bad != std::string_view::npos) {
            report(path, where + ", column " + std::to_string(bad + 1) + ": invalid character (only #, $ and . are allowed)");
            ok = false;
        }
        const int length = static_cast<int>(row.size());
        if (mask.rows == 1) {
            mask.columns = length;
        } else if (length != mask.columns) {
            report(path, where + ": the row length " + std::to_string(length) + " differs from the first row (" +
                             std::to_string(mask.columns) + ")");
            ok = false;
        }
        mask.cells.append(row);
    }
    if (mask.columns > MAX_COLUMNS || mask.rows > MAX_ROWS) {
        report(path, "the mask is " + std::to_string(mask.columns) + "x" + std::to_string(mask.rows) +
                         " (columns x rows), larger than the limit " + std::to_string(MAX_COLUMNS) + "x" +
                         std::to_string(MAX_ROWS));
        ok = false;
    }
    if (!ok) {
        return std::nullopt;
    }
    return mask;
}

[[nodiscard]] std::vector<std::string> split_tokens(std::string_view code) {
    std::vector<std::string> tokens;
    std::size_t position = 0;
    while (position < code.size()) {
        std::size_t end = code.find(' ', position);
        if (end == std::string_view::npos) {
            end = code.size();
        }
        if (end > position) {
            assert(end <= code.size());
            tokens.emplace_back(code.substr(position, end - position));
        }
        position = end + 1;
    }
    return tokens;
}

[[nodiscard]] std::string reversed(std::string_view text) {
    return std::string(text.rbegin(), text.rend());
}

// 状態に来たときに足したもの
enum class Piece : char { Token, CommentOpen, CommentStar, CommentClose };

// 字句を、# のマスの先頭から詰める。字句は1つの並び（行の中で穴をはさまない # の並び）に収め、収まらないところは
// コメントで埋める。$ のマスは、コメントの中でだけ越えてよい（字句のすぐ前やあとの $ は、字句の一部に読まれうる）。
// run_ids は # のマスごとの並びの番号、weak_before は # のマスごとに、すぐ前に $ があるか（末尾は最後のマスのあと）。
// いちばん短く終わる詰め方を返す。詰められなければ nullopt
[[nodiscard]] std::optional<std::string> lay_out(const std::vector<std::string>& tokens, const std::vector<int>& run_ids,
                                                 const std::vector<bool>& weak_before) {
    const int token_count = static_cast<int>(tokens.size());
    const int cell_count = static_cast<int>(run_ids.size());
    assert(static_cast<int>(weak_before.size()) == cell_count + 1);
    // 状態は（マスの位置、次の字句、コメントの中か）。遷移はマスの位置を必ず進めるので、位置の小さい順に広げる
    auto index = [token_count](int cell, int token, bool inside) {
        return (cell * (token_count + 1) + token) * 2 + (inside ? 1 : 0);
    };
    constexpr int NOT_REACHED = -1;
    constexpr int START = -2;
    const int state_count = (cell_count + 1) * (token_count + 1) * 2;
    std::vector<int> from(static_cast<std::size_t>(state_count), NOT_REACHED);
    std::vector<Piece> pieces(static_cast<std::size_t>(state_count), Piece::Token);
    from[static_cast<std::size_t>(index(0, 0, false))] = START;

    auto same_run = [&](int cell, int length) {
        return cell + length <= cell_count && run_ids[static_cast<std::size_t>(cell)] ==
                                                  run_ids[static_cast<std::size_t>(cell + length - 1)];
    };
    auto visit = [&](int state, int cell, int token, bool inside, Piece piece) {
        assert(cell <= cell_count);
        if (cell < cell_count && weak_before[static_cast<std::size_t>(cell)] && !inside) {
            return;
        }
        const auto next = static_cast<std::size_t>(index(cell, token, inside));
        if (from[next] == NOT_REACHED) {
            from[next] = state;
            pieces[next] = piece;
        }
    };

    for (int cell = 0; cell <= cell_count; ++cell) {
        for (int token = 0; token <= token_count; ++token) {
            for (const bool inside : {false, true}) {
                const int state = index(cell, token, inside);
                if (from[static_cast<std::size_t>(state)] == NOT_REACHED) {
                    continue;
                }
                if (!inside && token == token_count && !weak_before[static_cast<std::size_t>(cell)]) {
                    // 来た道を逆にたどって、詰めた文字列を組み立てる
                    std::string text;
                    for (int current = state; from[static_cast<std::size_t>(current)] != START;
                         current = from[static_cast<std::size_t>(current)]) {
                        switch (pieces[static_cast<std::size_t>(current)]) {
                            case Piece::Token: {
                                const int next_token = current / 2 % (token_count + 1);
                                assert(next_token >= 1 && next_token <= token_count);
                                text += reversed(tokens[static_cast<std::size_t>(next_token - 1)]);
                                break;
                            }
                            case Piece::CommentOpen: text += "*/"; break;
                            case Piece::CommentStar: text += "*"; break;
                            case Piece::CommentClose: text += "/*"; break;
                        }
                    }
                    return reversed(text);
                }
                if (inside) {
                    // 最後のマスのあとには進めない（コメントを閉じられないので、そこで行き止まり）
                    if (cell < cell_count) {
                        visit(state, cell + 1, token, true, Piece::CommentStar);
                    }
                    if (same_run(cell, 2)) {
                        visit(state, cell + 2, token, false, Piece::CommentClose);
                    }
                } else {
                    if (token < token_count) {
                        const int length = static_cast<int>(tokens[static_cast<std::size_t>(token)].size());
                        if (same_run(cell, length)) {
                            visit(state, cell + length, token + 1, false, Piece::Token);
                        }
                    }
                    if (same_run(cell, 2)) {
                        visit(state, cell + 2, token, true, Piece::CommentOpen);
                    }
                }
            }
        }
    }
    return std::nullopt;
}

// マスクのランレングス。# は英大文字、. は英小文字、$ は数字で、文字の値が数を表す（A と a と 1 が1つ）
[[nodiscard]] std::string run_lengths(const std::string& cells) {
    std::string runs;
    std::size_t position = 0;
    while (position < cells.size()) {
        const char cell = cells[position];
        const int limit = cell == '$' ? MAX_DIGIT_RUN : MAX_LETTER_RUN;
        int count = 0;
        while (position < cells.size() && cells[position] == cell && count < limit) {
            ++count;
            ++position;
        }
        const char base = cell == '#' ? '@' : cell == '.' ? '`' : '0';
        runs += static_cast<char>(base + count);
    }
    return runs;
}

[[nodiscard]] std::optional<Quine> generate(const Mask& mask, const char* path) {
    // parse_mask は行の長さをそろえるので、cells は columns * rows 文字（下の mask.cells[] の前提）
    assert(mask.cells.size() == static_cast<std::size_t>(mask.columns) * static_cast<std::size_t>(mask.rows));
    Quine quine;
    std::vector<int> run_ids;
    std::vector<bool> weak_before;
    bool weak = false;
    int run_id = -1;
    for (int row = 0; row < mask.rows; ++row) {
        bool in_run = false;
        for (int column = 0; column < mask.columns; ++column) {
            const char cell = mask.cells[static_cast<std::size_t>(row * mask.columns + column)];
            if (cell == '#') {
                if (!in_run) {
                    ++run_id;
                }
                run_ids.push_back(run_id);
                weak_before.push_back(weak);
                weak = false;
            } else if (cell == '$') {
                weak = true;
                ++quine.weak_cells;
            }
            in_run = cell == '#';
        }
    }
    weak_before.push_back(weak);
    quine.hash_cells = static_cast<int>(run_ids.size());
    if (weak_before.front() || weak_before.back()) {
        report(path, "a weak point ($) must not come before the first # or after the last #");
        return std::nullopt;
    }

    std::string q_code(Q_CODE);
    const std::size_t columns_at = q_code.find("COLUMNS");
    assert(columns_at != std::string::npos);
    q_code.replace(columns_at, std::string_view("COLUMNS").size(), std::to_string(mask.columns));
    std::vector<std::string> p_tokens = split_tokens(P_CODE);
    p_tokens.emplace_back(RAW_OPEN);
    std::vector<std::string> q_tokens = split_tokens(q_code);
    q_tokens.insert(q_tokens.begin(), std::string(RAW_CLOSE));
    const std::string m = run_lengths(mask.cells);

    // 詰め物がなくても要るマス：F = P + R"( + D + )" + Q、D = P + R"( + @ + Q + @ + M なので、2 * (字句の文字数) + M
    std::size_t bare_length = 0;
    for (const std::string& token : p_tokens) {
        bare_length += token.size();
    }
    for (const std::string& token : q_tokens) {
        bare_length += token.size();
    }
    const int least_cells = static_cast<int>(2 * bare_length + m.size());
    if (quine.hash_cells < least_cells) {
        report(path, "the shape is too small for the quine: " + std::to_string(quine.hash_cells) + " # cells, at least " +
                         std::to_string(least_cells - quine.hash_cells) + " more are needed");
        return std::nullopt;
    }

    // P は前から、Q は後ろから詰める（字句と並びを逆にして同じ探し方を使い、結果をもとに戻す）
    const std::optional<std::string> p_laid = lay_out(p_tokens, run_ids, weak_before);
    std::vector<std::string> q_tokens_reversed;
    for (auto it = q_tokens.rbegin(); it != q_tokens.rend(); ++it) {
        q_tokens_reversed.push_back(reversed(*it));
    }
    const std::vector<int> run_ids_reversed(run_ids.rbegin(), run_ids.rend());
    const std::vector<bool> weak_before_reversed(weak_before.rbegin(), weak_before.rend());
    const std::optional<std::string> q_laid_reversed = lay_out(q_tokens_reversed, run_ids_reversed, weak_before_reversed);
    if (!p_laid.has_value() || !q_laid_reversed.has_value()) {
        report(path, "the shape is too narrow for the quine: the output code cannot be laid out (it needs rows with runs of " +
                         std::to_string(LONGEST_TOKEN) +
                         " or more # cells; a weak point ($) can also block it, because it must fall inside a comment)");
        return std::nullopt;
    }
    const std::string p_laid_text = *p_laid;
    const std::string q_laid_text = reversed(*q_laid_reversed);
    // lay_out は最後の字句を置いたところで終わるので、P は R"( で終わり、Q は )" で始まる（下の substr の前提）
    assert(p_laid_text.ends_with(RAW_OPEN));
    assert(q_laid_text.starts_with(RAW_CLOSE));
    const std::string p = p_laid_text.substr(0, p_laid_text.size() - RAW_OPEN.size());
    const std::string q = q_laid_text.substr(RAW_CLOSE.size());

    std::string d = p + std::string(RAW_OPEN) + "@" + q + "@" + m;
    const int d_cells = quine.hash_cells - static_cast<int>(p_laid_text.size() + q_laid_text.size());
    if (d_cells < static_cast<int>(d.size())) {
        report(path, "the shape is too small for the quine: " + std::to_string(quine.hash_cells) + " # cells, " +
                         std::to_string(static_cast<int>(d.size()) - d_cells) + " more are needed");
        return std::nullopt;
    }
    quine.spare = d_cells - static_cast<int>(d.size());
    d.append(static_cast<std::size_t>(quine.spare), '*');
    // D は生文字列の中身なので、閉じる区切りを含んではいけない。@ は区切りの2つだけ
    assert(d.find(RAW_CLOSE) == std::string::npos);
    const std::string f = p + std::string(RAW_OPEN) + d + std::string(RAW_CLOSE) + q;
    assert(static_cast<int>(f.size()) == quine.hash_cells);

    std::size_t next = 0;
    for (int row = 0; row < mask.rows; ++row) {
        for (int column = 0; column < mask.columns; ++column) {
            const char cell = mask.cells[static_cast<std::size_t>(row * mask.columns + column)];
            quine.source += cell == '#' ? f[next++] : cell == '$' ? '$' : ' ';
        }
        quine.source += '\n';
    }
    quine.p_length = static_cast<int>(p.size());
    quine.q_length = static_cast<int>(q.size());
    quine.m_length = static_cast<int>(m.size());
    quine.comment_padding = static_cast<int>(p_laid_text.size() + q_laid_text.size() - bare_length);
    return quine;
}

[[nodiscard]] bool run(const char* mask_path, const char* output_path) {
    // 引数の打ち間違いで、マスクを Quine で上書きしないようにする。比べるのはパスの文字列だけ
    // （書き方が違う同じファイルは見分けない）
    if (std::strcmp(mask_path, output_path) == 0) {
        report(output_path, "the output is the same path as the mask");
        return false;
    }
    const std::optional<std::string> text = read_file(mask_path);
    if (!text.has_value()) {
        return false;
    }
    const std::optional<Mask> mask = parse_mask(*text, mask_path);
    if (!mask.has_value()) {
        return false;
    }
    const std::optional<Quine> quine = generate(*mask, mask_path);
    if (!quine.has_value()) {
        return false;
    }
    if (!write_file(output_path, quine->source)) {
        return false;
    }
    std::printf("quine_gen: wrote %s (%dx%d, # %d, $ %d; P %d, Q %d, M %d characters; comment padding %d; spare %d)\n",
                output_path, mask->columns, mask->rows, quine->hash_cells, quine->weak_cells, quine->p_length,
                quine->q_length, quine->m_length, quine->comment_padding, quine->spare);
    return true;
}

}  // namespace
}  // namespace stg

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: quine_gen <mask> <output>\n");
        return 1;
    }
    return stg::run(argv[1], argv[2]) ? 0 : 1;
}

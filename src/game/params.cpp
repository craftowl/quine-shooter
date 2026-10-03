#include "game/params.h"

#include <cmath>
#include <initializer_list>
#include <string_view>
#include <utility>

#include "core/window.h"
#include "nlohmann/json.hpp"

namespace stg {
namespace {

using nlohmann::json;

// 速さの上限（px/秒）。画面の幅の10倍/秒（design.md §5）。上限がないと、float にしたときに無限大になりうる
constexpr double MAX_SPEED = 4800.0;

// 失敗の理由を error に入れ、TraceLog に出す（ADR 0006）
void fail(std::string& error, std::string reason) {
    TraceLog(LOG_WARNING, "PARAMS: %s", reason.c_str());
    error = std::move(reason);
}

// object の中で、names にないキーを警告する。失敗にはしない（tasks.md の T12）
void warn_unknown_keys(const json& object, const char* prefix, std::initializer_list<std::string_view> names) {
    for (auto it = object.begin(); it != object.end(); ++it) {
        bool known = false;
        for (const std::string_view name : names) {
            if (it.key() == name) {
                known = true;
            }
        }
        if (!known) {
            TraceLog(LOG_WARNING, "PARAMS: Unknown key %s%s is ignored", prefix, it.key().c_str());
        }
    }
}

// object の中の name を、object として取り出す。path はエラーに出すキーの名前
const json* find_object(const json& object, const char* name, const char* path, std::string& error) {
    const auto it = object.find(name);
    if (it == object.end()) {
        fail(error, std::string("Missing key: ") + path);
        return nullptr;
    }
    if (!it->is_object()) {
        fail(error, std::string("Not an object: ") + path);
        return nullptr;
    }
    return &*it;
}

// object の中の name を、有限の数として取り出す
std::optional<double> find_number(const json& object, const char* name, const char* path, std::string& error) {
    const auto it = object.find(name);
    if (it == object.end()) {
        fail(error, std::string("Missing key: ") + path);
        return std::nullopt;
    }
    if (!it->is_number()) {
        fail(error, std::string("Not a number: ") + path);
        return std::nullopt;
    }
    const double number = it->get<double>();
    if (!std::isfinite(number)) {
        fail(error, std::string("Not a finite number: ") + path);
        return std::nullopt;
    }
    return number;
}

// 範囲（min より大きく max 以下、または min 以上 max 以下）に入っているかを確かめる
bool check_range(double number, double min, bool min_inclusive, double max, const char* path, std::string& error) {
    const bool above_min = min_inclusive ? number >= min : number > min;
    if (above_min && number <= max) {
        return true;
    }
    fail(error, TextFormat("Out of range: %s is %g (must be %s %g and <= %g)", path, number, min_inclusive ? ">=" : ">",
                           min, max));
    return false;
}

std::optional<float> read_speed(const json& player, const char* name, const char* path, std::string& error) {
    const std::optional<double> speed = find_number(player, name, path, error);
    if (!speed.has_value() || !check_range(*speed, 0.0, false, MAX_SPEED, path, error)) {
        return std::nullopt;
    }
    return static_cast<float>(*speed);
}

std::optional<Vector2> read_position(const json& player, const char* name, const char* path, std::string& error) {
    const json* position = find_object(player, name, path, error);
    if (position == nullptr) {
        return std::nullopt;
    }
    warn_unknown_keys(*position, "player.start_position.", {"x", "y"});
    const std::optional<double> x = find_number(*position, "x", "player.start_position.x", error);
    if (!x.has_value() ||
        !check_range(*x, 0.0, true, static_cast<double>(SCREEN_WIDTH), "player.start_position.x", error)) {
        return std::nullopt;
    }
    const std::optional<double> y = find_number(*position, "y", "player.start_position.y", error);
    if (!y.has_value() ||
        !check_range(*y, 0.0, true, static_cast<double>(SCREEN_HEIGHT), "player.start_position.y", error)) {
        return std::nullopt;
    }
    return Vector2{static_cast<float>(*x), static_cast<float>(*y)};
}

}  // namespace

std::optional<Params> parse_params(const char* text, std::string& error) {
    // 例外を使わずに読む。書式が壊れていたら、discarded の値が返る（ADR 0006）
    const json root = json::parse(text, nullptr, false);
    if (root.is_discarded()) {
        fail(error, "Invalid JSON (syntax error)");
        return std::nullopt;
    }
    if (!root.is_object()) {
        fail(error, "The top level is not an object");
        return std::nullopt;
    }
    warn_unknown_keys(root, "", {"player"});

    const json* player = find_object(root, "player", "player", error);
    if (player == nullptr) {
        return std::nullopt;
    }
    warn_unknown_keys(*player, "player.", {"normal_speed", "slow_speed", "start_position"});

    Params params;
    const std::optional<float> normal_speed = read_speed(*player, "normal_speed", "player.normal_speed", error);
    if (!normal_speed.has_value()) {
        return std::nullopt;
    }
    const std::optional<float> slow_speed = read_speed(*player, "slow_speed", "player.slow_speed", error);
    if (!slow_speed.has_value()) {
        return std::nullopt;
    }
    const std::optional<Vector2> start_position =
        read_position(*player, "start_position", "player.start_position", error);
    if (!start_position.has_value()) {
        return std::nullopt;
    }
    params.player.normal_speed = *normal_speed;
    params.player.slow_speed = *slow_speed;
    params.player.start_position = *start_position;
    return params;
}

}  // namespace stg

#include "enemies/enemy_text.h"

#include "raylib.h"

namespace stg {

std::optional<EnemyText> find_enemy_text(std::string_view id) {
    for (const EnemyText& enemy : all_enemy_texts()) {
        if (enemy.id == id) {
            return enemy;
        }
    }
    TraceLog(LOG_WARNING, "ENEMY: No embedded text for enemy id %.*s", static_cast<int>(id.size()), id.data());
    return std::nullopt;
}

}  // namespace stg

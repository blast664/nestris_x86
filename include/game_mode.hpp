#pragma once

#include <array>
#include <string>

namespace nestris_x86 {

enum class GameMode {
  Normal,    // Regular A-type game.
  Maxout,    // The game starts with a set score.
  Quicktap,  // Only long bars, a tower in the second column, each bar is removed after locking.
  Garbage,   // A permanent floor of a set height that is never cleared.
  Drought,   // Long bars appear at a set percentage of their normal rate.
  COUNT
};

constexpr int game_mode_count = static_cast<int>(GameMode::COUNT);
const std::array<std::string, game_mode_count> game_mode_names{"NORMAL", "MAXOUT", "QUICKTAP",
                                                               "GARBAGE", "DROUGHT"};

inline std::string gameModeToString(const GameMode& mode) {
  return game_mode_names.at(static_cast<int>(mode));
}

inline GameMode gameModeFromString(const std::string& name) {
  for (int i = 0; i < game_mode_count; ++i) {
    if (game_mode_names[i] == name) {
      return static_cast<GameMode>(i);
    }
  }
  return GameMode::Normal;
}

}  // namespace nestris_x86

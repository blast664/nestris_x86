#pragma once

#include <optional>
#include <string>
#include <vector>

#include "game_mode.hpp"

namespace nestris_x86 {

struct GameRecord {
  std::string timestamp;  // Local time the game ended, "YYYY-MM-DD HH:MM:SS".
  GameMode mode{GameMode::Normal};
  int mode_setting{};  // Start score, tower height, garbage height or long bar percentage.
  int start_level{};
  int end_level{};
  int lines{};
  int score{};
  int start_score{};
  int tetris_rate{};  // Percent of the cleared lines cleared with tetrises.
  int pieces{};
  double duration_s{};
  int run_ahead_frames{};
  bool topped_out{};  // False if the game was ended with a soft reset.
};

/**
 * All games played, stored as one CSV line per game so the file can also be analysed with a
 * spreadsheet. Topped out and soft reset games count for scores and progression, except games
 * reset before the first line clear, as quickly restarting after a bad start is common.
 */
class GameHistory {
 public:
  explicit GameHistory(const std::string& path);

  // Adds the game and appends it to the file. Sets the timestamp if it is empty.
  void add(GameRecord record);

  // The games that count for scores, optionally only those started at `start_level`.
  std::vector<GameRecord> scoredGames(const GameMode& mode,
                                      const std::optional<int>& start_level = {}) const;
  int bestScore(const GameMode& mode, const int start_level) const;
  // The start levels of the scored games of `mode`, in ascending order.
  std::vector<int> startLevels(const GameMode& mode) const;

 private:
  void load();

  std::string path_;
  std::vector<GameRecord> records_;
};

}  // namespace nestris_x86

#include "frame_processors/history_screen_processor.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "drawing_utils.hpp"

namespace nestris_x86 {

using pdi = PixelDrawingInterface;

namespace {

constexpr int TEXT_WIDTH_PX = 8;
constexpr int X_LEFT_COLUMN = 32;
constexpr int X_VALUE_RIGHT = 220;

constexpr pdi::Coords PLOT_TOP_LEFT{36, 106};
constexpr pdi::Rect PLOT_SIZE{184, 94};
constexpr int MOVING_AVERAGE_GAMES = 10;

void drawRightAligned(const PixelDrawingInterface& drawer, const int y, const std::string& text,
                      const pdi::Color& color = pdi::WHITE()) {
  drawer.drawString(X_VALUE_RIGHT - static_cast<int>(text.size()) * TEXT_WIDTH_PX, y, text,
                    color);
}

// The average score of the last `count` games, or of all games if `count` is 0.
int averageScore(const std::vector<GameRecord>& games, const size_t count) {
  const size_t n = count == 0 ? games.size() : std::min(count, games.size());
  if (n == 0) {
    return 0;
  }
  double sum = 0;
  for (size_t i = games.size() - n; i < games.size(); ++i) {
    sum += games[i].score;
  }
  return static_cast<int>(sum / n + 0.5);
}

// Short score for the plot scale, e.g. 850K or 1.2M.
std::string shortScore(const int score) {
  std::ostringstream ss;
  if (score >= 1000000) {
    ss << std::fixed << std::setprecision(1) << score / 1e6 << "M";
  } else if (score >= 1000) {
    ss << score / 1000 << "K";
  } else {
    ss << score;
  }
  return ss.str();
}

// Rounds up to a round number for the top of the plot scale.
int niceCeiling(const int value) {
  int step = 1000;
  while (step * 10 <= value) {
    step *= 10;
  }
  step = std::max(1000, step / 2);
  return std::max(step, (value + step - 1) / step * step);
}

}  // namespace

HistoryScreenProcessor::HistoryScreenProcessor(
    std::unique_ptr<PixelDrawingInterface>&& drawer,
    const std::shared_ptr<sound::SoundPlayer>& sample_player,
    const std::shared_ptr<SpriteProvider>& sprite_provider,
    const std::shared_ptr<const GameHistory>& history)
    : drawer_(std::move(drawer)),
      sample_player_(sample_player),
      sprite_provider_(sprite_provider),
      history_(history),
      mode_{GameMode::Normal},
      start_level_{},
      frame_counter_{std::make_unique<std::atomic_int>(0)} {}

void HistoryScreenProcessor::setGameMode(const GameMode& mode) {
  mode_ = mode;
  selectLatestStartLevel();
}

void HistoryScreenProcessor::selectLatestStartLevel() {
  const auto games = history_->scoredGames(mode_);
  start_level_ = games.empty() ? std::nullopt : std::optional<int>{games.back().start_level};
}

void HistoryScreenProcessor::selectNextStartLevel(const int direction) {
  // All levels first, then each start level that has games.
  std::vector<std::optional<int>> choices{std::nullopt};
  for (const int level : history_->startLevels(mode_)) {
    choices.emplace_back(level);
  }
  const auto current = std::find(choices.begin(), choices.end(), start_level_);
  const int index = current == choices.end() ? 0 : static_cast<int>(current - choices.begin());
  const int count = static_cast<int>(choices.size());
  start_level_ = choices.at((index + direction + count) % count);
}

void HistoryScreenProcessor::renderPlot(const std::vector<GameRecord>& games) const {
  const int x0 = PLOT_TOP_LEFT.x;
  const int y0 = PLOT_TOP_LEFT.y;
  const int x1 = x0 + PLOT_SIZE.width - 1;
  const int y1 = y0 + PLOT_SIZE.height - 1;
  drawer_->drawLine(x0, y0, x0, y1, pdi::DARK_GREY());
  drawer_->drawLine(x0, y1, x1, y1, pdi::DARK_GREY());

  if (games.empty()) {
    drawer_->drawString(x0 + 12, y0 + 40, "NO SCORED GAMES");
    drawer_->drawString(x0 + 12, y0 + 50, "IN THIS MODE YET.");
    return;
  }

  // The most recent games, one pixel column each.
  const size_t shown = std::min(games.size(), static_cast<size_t>(PLOT_SIZE.width - 2));
  const size_t first = games.size() - shown;
  int max_score = 0;
  for (size_t i = first; i < games.size(); ++i) {
    max_score = std::max(max_score, games[i].score);
  }
  // Headroom above the best score, so its point is not hidden behind the scale label.
  const int scale_top = niceCeiling(max_score + max_score / 10);
  drawer_->drawString(x0 + 3, y0, shortScore(scale_top), pdi::DARK_GREY());

  const int plot_height = PLOT_SIZE.height - 2;
  auto to_x = [&](const size_t i) {
    if (shown == 1) {
      return x0 + 2;
    }
    return x0 + 2 + static_cast<int>((i - first) * (PLOT_SIZE.width - 4) / (shown - 1));
  };
  auto to_y = [&](const double score) {
    return y1 - 1 - static_cast<int>(score / scale_top * plot_height);
  };

  for (size_t i = first; i < games.size(); ++i) {
    drawer_->fillRect(to_x(i), to_y(games[i].score) - 1, 2, 2, pdi::GREY());
  }

  // Moving average over the last MOVING_AVERAGE_GAMES games, including earlier, unshown games.
  int prev_x = 0;
  int prev_y = 0;
  for (size_t i = first; i < games.size(); ++i) {
    const size_t window_start = i + 1 >= MOVING_AVERAGE_GAMES ? i + 1 - MOVING_AVERAGE_GAMES : 0;
    double sum = 0;
    for (size_t j = window_start; j <= i; ++j) {
      sum += games[j].score;
    }
    const int x = to_x(i);
    const int y = to_y(sum / (i + 1 - window_start));
    if (i > first) {
      drawer_->drawLine(prev_x, prev_y, x, y, pdi::WHITE());
    }
    prev_x = x;
    prev_y = y;
  }
}

void HistoryScreenProcessor::renderScreen() const {
  drawer_->drawSprite(0, 0, sprite_provider_->getSprite("options-background"));
  drawer_->drawString(100, 17, "HISTORY");

  // Mode selector.
  const auto mode_name = gameModeToString(mode_);
  const int mode_x = 128 - static_cast<int>(mode_name.size()) * TEXT_WIDTH_PX / 2;
  drawer_->drawString(mode_x, 36, mode_name);
  const auto selector_color = (*frame_counter_)++ % 4 ? pdi::WHITE() : pdi::BLACK();
  drawTriangleSelector(*drawer_, mode_x - 6, 36, 7, selector_color, true);
  drawTriangleSelector(*drawer_, mode_x + static_cast<int>(mode_name.size()) * TEXT_WIDTH_PX + 4,
                       36, 7, selector_color, false);

  drawer_->drawString(X_LEFT_COLUMN, 46, "START LEVEL");
  drawRightAligned(*drawer_, 46, start_level_.has_value() ? std::to_string(*start_level_) : "ALL");

  const auto games = history_->scoredGames(mode_, start_level_);
  const int best = games.empty() ? 0 : std::max_element(games.begin(), games.end(),
                                                         [](const auto& a, const auto& b) {
                                                           return a.score < b.score;
                                                         })->score;
  const std::vector<std::pair<std::string, int>> rows{
      {"GAMES", static_cast<int>(games.size())},
      {"BEST", best},
      {"AVERAGE", averageScore(games, 0)},
      {"AVG LAST 100", averageScore(games, 100)},
      {"AVG LAST 10", averageScore(games, 10)}};
  int y = 58;
  for (const auto& [label, value] : rows) {
    drawer_->drawString(X_LEFT_COLUMN, y, label);
    drawRightAligned(*drawer_, y, std::to_string(value));
    y += 9;
  }

  renderPlot(games);
  drawer_->drawString(X_LEFT_COLUMN, 203, "UP/DOWN: START LEVEL", pdi::DARK_GREY());
}

ProgramFlowSignal HistoryScreenProcessor::processFrame(const KeyEvents& key_events) {
  if (key_events.at(KeyAction::Start).pressed ||
      key_events.at(KeyAction::RotateAntiClockwise).pressed ||
      key_events.at(KeyAction::RotateClockwise).pressed) {
    sample_player_->playSample("menu_select_02");
    *frame_counter_ = 0;
    return ProgramFlowSignal::LevelSelectorScreen;
  }
  if (key_events.at(KeyAction::Left).pressed) {
    sample_player_->playSample("menu_blip");
    mode_ = static_cast<GameMode>((static_cast<int>(mode_) + game_mode_count - 1) %
                                  game_mode_count);
    selectLatestStartLevel();
  }
  if (key_events.at(KeyAction::Right).pressed) {
    sample_player_->playSample("menu_blip");
    mode_ = static_cast<GameMode>((static_cast<int>(mode_) + 1) % game_mode_count);
    selectLatestStartLevel();
  }
  if (key_events.at(KeyAction::Up).pressed) {
    sample_player_->playSample("menu_blip");
    selectNextStartLevel(-1);
  }
  if (key_events.at(KeyAction::Down).pressed) {
    sample_player_->playSample("menu_blip");
    selectNextStartLevel(1);
  }
  renderScreen();
  return ProgramFlowSignal::FrameSuccess;
}

}  // namespace nestris_x86

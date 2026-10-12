
#include "frame_processors/level_screen_processor.hpp"

#include <algorithm>

#include "drawers/pixel_drawing_interface.hpp"
#include "drawing_utils.hpp"

namespace nestris_x86 {

namespace {

// The menu next to the level selector.
const std::vector<std::pair<std::string, ProgramFlowSignal>> MENU_ITEMS{
    {"OPTIONS", ProgramFlowSignal::OptionsScreen},
    {"MODES", ProgramFlowSignal::GameModesScreen},
    {"HISTORY", ProgramFlowSignal::HistoryScreen}};
constexpr int MENU_X = 157;
constexpr int MENU_Y = 80;
constexpr int MENU_ROW_HEIGHT = 12;

}  // namespace

ProgramFlowSignal LevelScreenProcessor::processKeyEvents(const KeyEvents& key_events) {
  auto& level = level_;
  const auto& a_button = key_events.at(KeyAction::RotateClockwise);
  a_held_ = a_button.pressed || a_button.held;
  if (menu_item_ >= 0) {
    const int num_items = static_cast<int>(MENU_ITEMS.size());
    if (key_events.at(KeyAction::Left).pressed) {
      sample_player_->playSample("menu_blip");
      menu_item_ = -1;
      return ProgramFlowSignal::FrameSuccess;
    }
    if (key_events.at(KeyAction::Up).pressed) {
      sample_player_->playSample("menu_blip");
      menu_item_ = (menu_item_ + num_items - 1) % num_items;
    }
    if (key_events.at(KeyAction::Down).pressed) {
      sample_player_->playSample("menu_blip");
      menu_item_ = (menu_item_ + 1) % num_items;
    }
    if (key_events.at(KeyAction::RotateClockwise).pressed ||
        key_events.at(KeyAction::Start).pressed) {
      sample_player_->playSample("menu_select_02");
      return MENU_ITEMS.at(menu_item_).second;
    }
    return ProgramFlowSignal::FrameSuccess;
  }

  if (key_events.at(KeyAction::Start).pressed) {
    sample_player_->playSample("menu_select_01");
    plus_ten_levels_ = (key_events.at(KeyAction::RotateClockwise).held);
    return ProgramFlowSignal::StartGame;
  }

  if (key_events.at(KeyAction::Left).pressed && level > 0) {
    sample_player_->playSample("menu_blip");
    --level;
  }
  if (key_events.at(KeyAction::Right).pressed) {
    if (level == 4 || level == 9) {
      menu_item_ = 0;
    } else {
      ++level;
    }
    sample_player_->playSample("menu_blip");
  }
  if (key_events.at(KeyAction::Down).pressed && level < 5) {
    sample_player_->playSample("menu_blip");
    level += 5;
  }
  if (key_events.at(KeyAction::Up).pressed && level >= 5) {
    sample_player_->playSample("menu_blip");
    level -= 5;
  }

  return ProgramFlowSignal::FrameSuccess;
}

LevelScreenProcessor::LevelScreenProcessor(std::unique_ptr<PixelDrawingInterface>&& drawer,
                                           const std::shared_ptr<sound::SoundPlayer>& sample_player,
                                           const std::shared_ptr<SpriteProvider>& sprite_provider,
                                           const std::shared_ptr<const GameHistory>& history)
    : drawer_(std::move(drawer)),
      sample_player_(sample_player),
      sprite_provider_(sprite_provider),
      history_(history),
      game_mode_{GameMode::Normal},
      level_{},
      a_held_{},
      menu_item_{-1},
      plus_ten_levels_{},
      frame_counter_{std::make_unique<std::atomic_int>(0)} {}

void LevelScreenProcessor::renderMenu() const {
  if (*frame_counter_ == 0) {
    drawer_->drawSprite(0, 0, sprite_provider_->getSprite("a-type-background"));
  }

  // Clear the top area.
  drawer_->fillRect(30, 72, 190, 50, PixelDrawingInterface::BLACK());

  for (size_t i = 0; i < MENU_ITEMS.size(); ++i) {
    drawer_->drawString(MENU_X, MENU_Y + static_cast<int>(i) * MENU_ROW_HEIGHT,
                        MENU_ITEMS[i].first);
  }

  renderTitle();
  renderBestScores();

  if (menu_item_ >= 0) {
    const auto color =
        (*frame_counter_) % 4 ? PixelDrawingInterface::WHITE() : PixelDrawingInterface::BLACK();
    const int y = MENU_Y + menu_item_ * MENU_ROW_HEIGHT;
    drawTriangleSelector(*drawer_, MENU_X - 9, y, 7, color, false);
    drawTriangleSelector(*drawer_, 219, y, 7, color, true);
    renderLevelSelector(-1);
  } else {
    renderLevelSelector(level_);
  }
}

void LevelScreenProcessor::renderTitle() const {
  // The background shows A-TYPE, which is replaced by the name of a trainer mode. The background
  // is drawn again when the screen is entered, which restores A-TYPE.
  if (game_mode_ == GameMode::Normal) {
    return;
  }
  const auto name = gameModeToString(game_mode_);
  drawer_->fillRect(92, 13, 72, 16, PixelDrawingInterface::BLACK());
  drawer_->drawString(128 - static_cast<int>(name.size()) * 4, 17, name);
}

void LevelScreenProcessor::renderBestScores() const {
  // The best scores of games started at the selected level, or ten levels higher while A is held.
  const int start_level = level_ + (a_held_ ? 10 : 0);
  const std::string title = "BEST FROM LEVEL " + std::to_string(start_level);
  drawer_->fillRect(40, 117, 176, 10, PixelDrawingInterface::BLACK());
  drawer_->drawString(128 - static_cast<int>(title.size()) * 4, 119, title);

  // Replace the NAME column header of the background with LINES.
  drawer_->fillRect(72, 136, 48, 9, PixelDrawingInterface::BLACK());
  drawer_->drawString(72, 137, "LINES");

  // Clear the rows, as the selected level changes.
  drawer_->fillRect(72, 152, 137, 42, PixelDrawingInterface::BLACK());
  auto games = history_->scoredGames(game_mode_, start_level);
  std::sort(games.begin(), games.end(),
            [](const GameRecord& a, const GameRecord& b) { return a.score > b.score; });
  constexpr int first_row_y = 153;
  constexpr int row_height = 16;
  for (size_t i = 0; i < std::min<size_t>(3, games.size()); ++i) {
    const int y = first_row_y + static_cast<int>(i) * row_height;
    drawNumber(*drawer_, 80, y, games[i].lines, 3);
    drawNumber(*drawer_, 128, y, games[i].score, 7);
    drawNumber(*drawer_, 192, y, games[i].end_level, 2);
  }
}

void LevelScreenProcessor::setGameMode(const GameMode& mode) {
  game_mode_ = mode;
}

void LevelScreenProcessor::renderLevelSelector(const int level) const {
  constexpr PixelDrawingInterface::Coords level_selector{51, 75};
  constexpr PixelDrawingInterface::Rect level_selector_size{83, 35};
  constexpr PixelDrawingInterface::Rect selector_size{16, 16};
  const PixelDrawingInterface::Color selector_color{252, 151, 56, 255};

  auto get_selector_coords = [&](const int level) -> PixelDrawingInterface::Coords {
    const int y = level_selector.y + ((level % 10 > 4) ? selector_size.height : 0);
    return {(selector_size.width * (level % 5)) + level_selector.x + 1, y + 2};
  };

  // Draw the flashing selector, and then draw the levels on top with transparency.
  const auto coords = get_selector_coords(level);
  const auto color = *frame_counter_ % 4 ? selector_color : PixelDrawingInterface::BLACK();
  if (level >= 0) {
    drawer_->fillRect(coords, selector_size, color);
  }
  drawer_->drawSprite(level_selector, sprite_provider_->getSprite("levels-screen"));

  ++(*frame_counter_);
}

ProgramFlowSignal LevelScreenProcessor::processFrame(const KeyEvents& key_events) {
  auto signal = processKeyEvents(key_events);
  renderMenu();

  if (signal != ProgramFlowSignal::FrameSuccess) {
    *frame_counter_ = 0;
  }
  return signal;
}

int LevelScreenProcessor::getSelectedLevel() const {
  return level_ + (plus_ten_levels_ ? 10 : 0);
}

}  // namespace nestris_x86

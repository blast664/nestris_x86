#include "frame_processors/game_mode_screen_processor.hpp"

#include <vector>

#include "drawing_utils.hpp"
#include "utils/logging.hpp"

namespace nestris_x86 {

using pdi = PixelDrawingInterface;

namespace {

constexpr int TEXT_WIDTH_PX = 8;
constexpr int X_LEFT_COLUMN = 32;
constexpr int X_VALUE_RIGHT = 212;  // Values are right aligned to this column.
constexpr int Y_MODE_ROW = 40;
constexpr int Y_FIRST_SETTING_ROW = 60;

// The settings in display order, with the mode each belongs to.
const std::vector<std::pair<std::string, GameMode>> SETTINGS{
    {"start_score", GameMode::Maxout},
    {"tower_height", GameMode::Quicktap},
    {"garbage_height", GameMode::Garbage},
    {"longbar_percent", GameMode::Drought}};

std::vector<std::string> modeDescription(const GameMode& mode) {
  switch (mode) {
    case GameMode::Normal:
      return {"REGULAR A-TYPE GAME."};
    case GameMode::Maxout:
      return {"THE GAME STARTS WITH THE", "SET SCORE."};
    case GameMode::Quicktap:
      return {"ONLY LONG BARS. TAP THEM", "OVER THE TOWER IN THE", "SECOND COLUMN. EACH BAR",
              "IS REMOVED AFTER IT", "LOCKS."};
    case GameMode::Garbage:
      return {"A PERMANENT FLOOR OF THE", "SET HEIGHT THAT IS NEVER", "CLEARED."};
    case GameMode::Drought:
      return {"LONG BARS APPEAR AT THE", "SET PERCENTAGE OF THEIR", "NORMAL RATE."};
    default:
      return {};
  }
}

}  // namespace

GameModeScreenProcessor::GameModeScreenProcessor(
    std::unique_ptr<PixelDrawingInterface>&& drawer,
    const std::shared_ptr<sound::SoundPlayer>& sample_player,
    const std::shared_ptr<SpriteProvider>& sprite_provider)
    : drawer_(std::move(drawer)),
      sample_player_(sample_player),
      sprite_provider_(sprite_provider),
      options_{},
      setting_selected_{},
      frame_counter_{std::make_unique<std::atomic_int>(0)} {
  options_["mode"] = std::make_unique<StringOption>(
      "MODE", std::vector<std::string>(game_mode_names.begin(), game_mode_names.end()));
  options_["start_score"] =
      std::make_unique<IntOption>("START SCORE", 0, 1500000, 500000, 50000);
  options_["tower_height"] = std::make_unique<IntOption>("TOWER HEIGHT", 0, 18, 10);
  options_["garbage_height"] = std::make_unique<IntOption>("GARBAGE HEIGHT", 1, 15, 8);
  options_["longbar_percent"] =
      std::make_unique<IntOption>("LONGBAR RATE %", 0, 100, 50, 10);
}

std::string GameModeScreenProcessor::settingName(const GameMode& mode) {
  for (const auto& [name, setting_mode] : SETTINGS) {
    if (setting_mode == mode) {
      return name;
    }
  }
  return {};
}

GameMode GameModeScreenProcessor::getGameMode() const {
  return gameModeFromString(options_.at("mode")->getSelectedOptionText());
}

int GameModeScreenProcessor::getModeSetting() const {
  const auto name = settingName(getGameMode());
  return name.empty() ? 0 : getIntOption(*options_.at(name));
}

YAML::Node GameModeScreenProcessor::getOptionsAsYaml() const {
  YAML::Node node;
  for (const auto& [name, option] : options_) {
    node[name] = option->getSelectedOptionText();
  }
  return node;
}

void GameModeScreenProcessor::setOptionsYaml(const YAML::Node& node) try {
  for (const auto& yaml_val : node) {
    const auto name = yaml_val.first.as<std::string>();
    if (options_.count(name) == 0) {
      LOG_ERROR("Bad key in yaml found `" << name << "`.");
      continue;
    }
    options_.at(name)->setOptionFromString(yaml_val.second.as<std::string>());
  }
} catch (const std::exception& e) {
  LOG_ERROR("Exception thrown loading game modes from YAML: `" << e.what() << "`");
}

OptionInterface& GameModeScreenProcessor::getSelectedOption() {
  const auto name = settingName(getGameMode());
  if (setting_selected_ && not name.empty()) {
    return *options_.at(name);
  }
  return *options_.at("mode");
}

ProgramFlowSignal GameModeScreenProcessor::processKeyEvents(const KeyEvents& key_events) {
  if (key_events.at(KeyAction::Start).pressed ||
      key_events.at(KeyAction::RotateAntiClockwise).pressed) {
    sample_player_->playSample("menu_select_02");
    return ProgramFlowSignal::LevelSelectorScreen;
  }
  const bool has_setting = not settingName(getGameMode()).empty();
  if (key_events.at(KeyAction::Up).pressed || key_events.at(KeyAction::Down).pressed) {
    sample_player_->playSample("menu_blip");
    setting_selected_ = has_setting && not setting_selected_;
  }
  if (key_events.at(KeyAction::Left).pressed) {
    sample_player_->playSample("menu_blip");
    getSelectedOption().selectPrevOption();
  }
  if (key_events.at(KeyAction::Right).pressed) {
    sample_player_->playSample("menu_blip");
    getSelectedOption().selectNextOption();
  }
  if (key_events.at(KeyAction::RotateClockwise).pressed) {
    sample_player_->playSample("menu_blip");
    getSelectedOption().selectNextOptionWrapAround();
  }
  return ProgramFlowSignal::FrameSuccess;
}

void GameModeScreenProcessor::renderScreen() const {
  drawer_->drawSprite(0, 0, sprite_provider_->getSprite("options-background"));
  drawer_->drawString(108, 17, "MODES");

  const auto mode = getGameMode();
  const auto selector_color = (*frame_counter_)++ % 4 ? pdi::WHITE() : pdi::BLACK();
  auto draw_row = [&](const OptionInterface& option, const int y, const bool active,
                      const bool selected) {
    const auto color = active ? pdi::WHITE() : pdi::DARK_GREY();
    const auto value = option.getSelectedOptionText();
    const int value_x = X_VALUE_RIGHT - static_cast<int>(value.size()) * TEXT_WIDTH_PX;
    drawer_->drawString(X_LEFT_COLUMN, y, option.getDisplayName(), color);
    drawer_->drawString(value_x, y, value, color);
    if (selected) {
      if (option.prevOptionAvailable()) {
        drawTriangleSelector(*drawer_, value_x - 4, y, 7, selector_color, true);
      }
      if (option.nextOptionAvailable()) {
        drawTriangleSelector(*drawer_, X_VALUE_RIGHT + 3, y, 7, selector_color, false);
      }
    }
  };

  draw_row(*options_.at("mode"), Y_MODE_ROW, true, not setting_selected_);
  int y = Y_FIRST_SETTING_ROW;
  for (const auto& [name, setting_mode] : SETTINGS) {
    const bool active = setting_mode == mode;
    draw_row(*options_.at(name), y, active, active && setting_selected_);
    y += 10;
  }

  y = 115;
  for (const auto& line : modeDescription(mode)) {
    drawer_->drawString(X_LEFT_COLUMN, y, line);
    y += 10;
  }
  drawer_->drawString(X_LEFT_COLUMN, 198, "B GOES BACK.");
}

ProgramFlowSignal GameModeScreenProcessor::processFrame(const KeyEvents& key_events) {
  const auto signal = processKeyEvents(key_events);
  if (settingName(getGameMode()).empty()) {
    setting_selected_ = false;
  }
  renderScreen();
  if (signal != ProgramFlowSignal::FrameSuccess) {
    *frame_counter_ = 0;
    setting_selected_ = false;
  }
  return signal;
}

}  // namespace nestris_x86

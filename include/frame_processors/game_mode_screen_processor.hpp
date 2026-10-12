#pragma once

#include <yaml-cpp/yaml.h>

#include <atomic>
#include <map>
#include <memory>
#include <string>

#include "assets.hpp"
#include "drawers/pixel_drawing_interface.hpp"
#include "frame_processor_interface.hpp"
#include "game_mode.hpp"
#include "option.hpp"
#include "sound.hpp"

namespace nestris_x86 {

// Screen for choosing the game mode (normal or one of the trainers) and its setting.
class GameModeScreenProcessor : public FrameProcessorInterface {
 public:
  GameModeScreenProcessor(std::unique_ptr<PixelDrawingInterface>&& drawer,
                          const std::shared_ptr<sound::SoundPlayer>& sample_player,
                          const std::shared_ptr<SpriteProvider>& sprite_provider);

  ProgramFlowSignal processFrame(const KeyEvents& key_events);

  GameMode getGameMode() const;
  // The setting of the selected mode, 0 for the normal mode.
  int getModeSetting() const;

  YAML::Node getOptionsAsYaml() const;
  void setOptionsYaml(const YAML::Node& node);

 private:
  ProgramFlowSignal processKeyEvents(const KeyEvents& key_events);
  void renderScreen() const;

  // The option that holds the setting of `mode`, or an empty string for the normal mode.
  static std::string settingName(const GameMode& mode);
  OptionInterface& getSelectedOption();

  std::unique_ptr<PixelDrawingInterface> drawer_;
  std::shared_ptr<sound::SoundPlayer> sample_player_;
  std::shared_ptr<SpriteProvider> sprite_provider_;
  std::map<std::string, std::unique_ptr<OptionInterface>> options_;
  bool setting_selected_;  // Otherwise the mode itself is selected.

  // See OptionScreenProcessor::frame_counter_ for why this is wrapped.
  std::unique_ptr<std::atomic_int> frame_counter_;
};

}  // namespace nestris_x86

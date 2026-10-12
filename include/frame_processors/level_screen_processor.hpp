#pragma once

#include <memory>

#include "assets.hpp"
#include "drawers/pixel_drawing_interface.hpp"
#include "frame_processor_interface.hpp"
#include "game_history.hpp"
#include "game_mode.hpp"
#include "sound.hpp"

namespace nestris_x86 {

class LevelScreenProcessor : public FrameProcessorInterface {
 public:
  LevelScreenProcessor(std::unique_ptr<PixelDrawingInterface>&& drawer,
                       const std::shared_ptr<sound::SoundPlayer>& sample_player,
                       const std::shared_ptr<SpriteProvider>& sprite_provider,
                       const std::shared_ptr<const GameHistory>& history);

  ProgramFlowSignal processFrame(const KeyEvents& key_events);

  int getSelectedLevel() const;

  // The mode shown in the title and used for the best scores.
  void setGameMode(const GameMode& mode);

 private:
  ProgramFlowSignal processKeyEvents(const KeyEvents& key_events);

  void renderMenu() const;
  void renderLevelSelector(const int level) const;
  void renderTitle() const;
  void renderBestScores() const;

  std::unique_ptr<PixelDrawingInterface> drawer_;
  std::shared_ptr<sound::SoundPlayer> sample_player_;
  std::shared_ptr<SpriteProvider> sprite_provider_;
  std::shared_ptr<const GameHistory> history_;
  GameMode game_mode_;
  int level_;
  // A is held, which adds ten levels when the game is started. The best scores follow it.
  bool a_held_;
  // Index into the menu next to the level selector, or -1 when a level is selected.
  int menu_item_;
  bool plus_ten_levels_;
  std::unique_ptr<std::atomic_int> frame_counter_;
};

}  // namespace nestris_x86

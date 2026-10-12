#pragma once

#include <atomic>
#include <memory>
#include <optional>

#include "assets.hpp"
#include "drawers/pixel_drawing_interface.hpp"
#include "frame_processor_interface.hpp"
#include "game_history.hpp"
#include "game_mode.hpp"
#include "sound.hpp"

namespace nestris_x86 {

// Shows the score progression over the games of one game mode and start level.
class HistoryScreenProcessor : public FrameProcessorInterface {
 public:
  HistoryScreenProcessor(std::unique_ptr<PixelDrawingInterface>&& drawer,
                         const std::shared_ptr<sound::SoundPlayer>& sample_player,
                         const std::shared_ptr<SpriteProvider>& sprite_provider,
                         const std::shared_ptr<const GameHistory>& history);

  ProgramFlowSignal processFrame(const KeyEvents& key_events);

  // The mode shown when the screen is opened.
  void setGameMode(const GameMode& mode);

 private:
  // Selects the start level of the latest game of the mode, or all levels if there is none.
  void selectLatestStartLevel();
  void selectNextStartLevel(const int direction);

  void renderScreen() const;
  void renderPlot(const std::vector<GameRecord>& games) const;

  std::unique_ptr<PixelDrawingInterface> drawer_;
  std::shared_ptr<sound::SoundPlayer> sample_player_;
  std::shared_ptr<SpriteProvider> sprite_provider_;
  std::shared_ptr<const GameHistory> history_;
  GameMode mode_;
  std::optional<int> start_level_;  // All start levels if empty.

  // See OptionScreenProcessor::frame_counter_ for why this is wrapped.
  std::unique_ptr<std::atomic_int> frame_counter_;
};

}  // namespace nestris_x86

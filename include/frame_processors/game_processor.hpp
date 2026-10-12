#pragma once

#include <optional>
#include <random>
#include <stdexcept>

#include "assets.hpp"
#include "das.hpp"
#include "drawers/pixel_drawing_interface.hpp"
#include "frame_processor_interface.hpp"
#include "game_history.hpp"
#include "game_mode.hpp"
#include "game_renderer.hpp"
#include "game_states.hpp"
#include "gravity.hpp"
#include "sound.hpp"
#include "statistics.hpp"
#include "tetris_type.hpp"
#include "tetromino_rng.hpp"

namespace nestris_x86 {

struct GameOptions {
  int level{};
  int das_full_charge{Das::NTSC_FULL_CHARGE};
  int das_min_charge{Das::NTSC_MIN_CHARGE};
  int game_frequency{NTSC_FREQUENCY};
  TetrisType gravity_type{TetrisType::NTSC};
  bool show_controls{true};
  bool show_das_bar{true};
  bool wall_kick{false};
  bool hard_drop{false};
  StatisticsMode statistics_mode{};
  RngType rng_type{RngType::Nes};
  int run_ahead_frames{};
  bool show_background{true};
  GameMode game_mode{GameMode::Normal};
  int mode_setting{};  // Start score, tower height, garbage height or long bar percentage.
  int top_score{};     // Best previous score, shown as TOP.
};

class GameProcessor : public FrameProcessorInterface {
 public:
  GameProcessor(const GameOptions& options, std::unique_ptr<PixelDrawingInterface>&& drawer,
                const std::shared_ptr<sound::SoundPlayer>& sample_player_,
                const std::shared_ptr<SpriteProvider>& sprite_provider);

  ProgramFlowSignal processFrame(const KeyEvents& key_events);

  void reset(const GameOptions& options);

  // The record of the game, once it ended by topping out or a soft reset. Returns it only once.
  std::optional<GameRecord> takeFinishedGame();

 private:
  // Everything that changes while the game is simulated. Saved and restored for run ahead.
  struct Snapshot {
    GameState<> state;
    Statistics statistics;
    std::unique_ptr<TetrominoRNG> tetromino_rng;
    std::mt19937 entry_delay_random_engine;
    LineClearAnimationInfo line_clear_info;
    int top_out_frame_counter;
    std::optional<int> tetris_flash_frame;
  };

  Snapshot saveSnapshot() const;
  void restoreSnapshot(Snapshot&& snapshot);

  ProgramFlowSignal simulateFrame(const KeyEvents& key_events);
  void renderFrame(const KeyEvents& key_events);
  void endFrame();

  void finishGame(const bool topped_out);

  bool spawnNewTetromino(GameState<>& state);
  Tetromino getRandomTetromino();
  GameState<> getNewState(const int level);

  void doGravityStep(const KeyEvents& key_events);
  void doEntryDelayStep(const KeyEvents& key_events);

  GameRenderer renderer_;
  std::shared_ptr<sound::SoundPlayer> sample_player_;
  GameState<> state_;
  Statistics statistics_;
  std::unique_ptr<TetrominoRNG> tetromino_rng_;
  Das das_processor_;
  Gravity gravity_provider_;
  bool show_controls_;
  bool show_das_bar_;
  bool wall_kick_;
  bool hard_drop_;
  StatisticsMode statistics_mode_;
  LineClearAnimationInfo line_clear_info_;
  int top_out_frame_counter_;
  std::mt19937 entry_delay_random_engine_;
  // Set when the last simulated frame was part of a tetris line clear, for the flash animation.
  std::optional<int> tetris_flash_frame_;
  int run_ahead_frames_;

  GameMode game_mode_;
  int mode_setting_;
  int top_score_;
  // The board at the start of a game: empty, or the trainer's garbage.
  GameState<>::Grid base_grid_;

  int start_level_;
  int game_frequency_;
  int frames_played_;
  std::optional<GameRecord> finished_game_;
};

}  // namespace nestris_x86

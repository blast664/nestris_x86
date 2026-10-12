
#include "frame_processors/game_processor.hpp"

#include <iso646.h>

#include <cmath>
#include <iterator>
#include <stdexcept>

#include "assets.hpp"
#include "drawers/pixel_drawing_interface.hpp"
#include "game_logic.hpp"
#include "gravity.hpp"
#include "key_defines.hpp"
#include "sound.hpp"
#include "utils/logging.hpp"

namespace nestris_x86 {

constexpr int GRAVITY_FIRST_FRAME = 100;
// Garbage and the quicktap tower use the white block, like the T, square and long bar.
constexpr int GARBAGE_BLOCK = 1;

namespace {

std::unique_ptr<TetrominoRNG> makeTetrominoRng(const GameOptions& options) {
  if (options.game_mode == GameMode::Quicktap) {
    return std::make_unique<LongbarTetrominoRNG>();
  }
  auto rng = tetrominoRngFactory(options.rng_type);
  if (options.game_mode == GameMode::Drought) {
    return std::make_unique<DroughtTetrominoRNG>(std::move(rng), options.mode_setting);
  }
  return rng;
}

GameState<>::Grid makeBaseGrid(const GameMode& mode, const int height) {
  GameState<>::Grid grid{};
  const int rows = static_cast<int>(grid[0].size());
  if (mode == GameMode::Garbage) {
    for (auto& column : grid) {
      for (int y = rows - height; y < rows; ++y) {
        column[y] = GARBAGE_BLOCK;
      }
    }
  } else if (mode == GameMode::Quicktap) {
    // A tower in the second column, to quicktap over to the first column.
    for (int y = rows - height; y < rows; ++y) {
      grid[1][y] = GARBAGE_BLOCK;
    }
  }
  return grid;
}

}  // namespace

GameProcessor::GameProcessor(const GameOptions& options,
                             std::unique_ptr<PixelDrawingInterface>&& drawer,
                             const std::shared_ptr<sound::SoundPlayer>& sample_player,
                             const std::shared_ptr<SpriteProvider>& sprite_provider)
    : renderer_(std::move(drawer), sprite_provider, "./assets/images"),
      sample_player_(sample_player),
      state_{},
      statistics_{},
      tetromino_rng_{tetrominoRngFactory(RngType::Nes)},
      das_processor_{options.das_full_charge, options.das_min_charge},
      gravity_provider_{options.gravity_type},
      show_controls_{options.show_controls},
      show_das_bar_{options.show_das_bar},
      wall_kick_{options.wall_kick},
      hard_drop_{options.hard_drop},
      statistics_mode_{options.statistics_mode},
      line_clear_info_{},
      top_out_frame_counter_{},
      entry_delay_random_engine_{std::random_device{}()},
      tetris_flash_frame_{},
      run_ahead_frames_{options.run_ahead_frames},
      game_mode_{options.game_mode},
      mode_setting_{options.mode_setting},
      top_score_{options.top_score},
      base_grid_{makeBaseGrid(options.game_mode, options.mode_setting)},
      start_level_{options.level},
      game_frequency_{options.game_frequency},
      frames_played_{},
      finished_game_{} {
  state_ = getNewState(options.level);
  statistics_.update(state_.active_tetromino.tetromino);
}

void GameProcessor::reset(const GameOptions& options) {
  das_processor_ = Das{options.das_full_charge, options.das_min_charge};
  gravity_provider_ = Gravity{options.gravity_type};
  show_controls_ = options.show_controls;
  show_das_bar_ = options.show_das_bar;
  wall_kick_ = options.wall_kick;
  hard_drop_ = options.hard_drop;
  statistics_mode_ = options.statistics_mode;
  line_clear_info_ = {};
  top_out_frame_counter_ = {};
  tetris_flash_frame_ = {};
  run_ahead_frames_ = options.run_ahead_frames;
  game_mode_ = options.game_mode;
  mode_setting_ = options.mode_setting;
  top_score_ = options.top_score;
  base_grid_ = makeBaseGrid(options.game_mode, options.mode_setting);
  start_level_ = options.level;
  game_frequency_ = options.game_frequency;
  frames_played_ = 0;
  finished_game_.reset();
  renderer_.setShowBackground(options.show_background);
  tetromino_rng_ = makeTetrominoRng(options);
  state_ = getNewState(options.level);
  statistics_ = {};
  statistics_.update(state_.active_tetromino.tetromino);
  renderer_.startNewGame();
}

Tetromino GameProcessor::getRandomTetromino() {
  return tetromino_rng_->getRandomTetromino();
}

bool GameProcessor::spawnNewTetromino(GameState<>& state) {
  state.active_tetromino = {state.next_tetromino, 5, 0, 0};
  state.next_tetromino = getRandomTetromino();
  state.spawn_new_tetromino = false;
  return not tetrominoCollision(state.grid, state.active_tetromino);
}

GameState<> GameProcessor::getNewState(const int level) {
  auto state = GameState<>{};
  state.level = level;
  state.grid = base_grid_;
  state.top_score = top_score_;
  if (game_mode_ == GameMode::Maxout) {
    state.score = mode_setting_;
    state.start_score = mode_setting_;
  }
  if (game_mode_ == GameMode::Garbage) {
    state.floor_height = mode_setting_;
  }
  state.active_tetromino = {getRandomTetromino(), 5, 0, 0};
  state.next_tetromino = getRandomTetromino();
  state.gravity_counter = GRAVITY_FIRST_FRAME;
  state.lines_until_next_level = linesToClearFromStartingLevel(state.level);
  state.viz_wall_charge_frame_count = 0;
  return state;
}

void GameProcessor::doGravityStep(const KeyEvents& key_events) {
  if (state_.spawn_new_tetromino) {
    const bool topped_out = not spawnNewTetromino(state_);
    statistics_.update(state_.active_tetromino.tetromino);
    if (topped_out) {
      // Lock the active tetromino and move it off the grid. This is to stop the active
      // tetromino interfering with the 'curtain' animation.
      state_.grid = addTetrominoToGrid(state_.grid, state_.active_tetromino);
      state_.active_tetromino.y = -10;
      state_.topped_out = true;
      sample_player_->playSample("top_out");
    }
  }

  processKeyEvents(key_events, *sample_player_, das_processor_, wall_kick_, hard_drop_, state_);
  if (not das_processor_.dasSoftlyCharged(state_.das_counter)) {
    statistics_.dasResetSignal();
  }

  const bool tetromino_locked = applyGravity(key_events, gravity_provider_, state_);
  if (tetromino_locked) {
    sample_player_->playSample("tetromino_lock");
    state_.press_down_lock = true;
    addPressDownScore(state_);
    if (game_mode_ == GameMode::Quicktap) {
      // Remove the bar again, so the next bar starts on the same board.
      state_.grid = base_grid_;
      return;
    }
    auto lines_cleared = checkForLineClears(state_);
    if (not lines_cleared.empty()) {
      updateEntryDelayForLineClear(state_.entry_delay_counter, entry_delay_random_engine_);
      line_clear_info_ = LineClearAnimationInfo{lines_cleared, state_.entry_delay_counter};
    }
  }
}

void GameProcessor::doEntryDelayStep(const KeyEvents& key_events) {
  if (key_events.at(KeyAction::Start).pressed) {
    state_.paused = true;
    sample_player_->playSample("pause");
  }

  animateLineClear(*sample_player_, state_, line_clear_info_);
  if (line_clear_info_.rows.size() == 4) {
    tetris_flash_frame_ = line_clear_info_.animation_frame;
  }
  // When the animation is almost over, update the score.
  if (line_clear_info_.animation_frame == 4) {
    updateScoreAndLevel(static_cast<int>(line_clear_info_.rows.size()), *sample_player_, state_);
    statistics_.update(static_cast<int>(line_clear_info_.rows.size()), state_.level);
  }
  --state_.entry_delay_counter;
}

GameProcessor::Snapshot GameProcessor::saveSnapshot() const {
  return Snapshot{state_,
                  statistics_,
                  tetromino_rng_->clone(),
                  entry_delay_random_engine_,
                  line_clear_info_,
                  top_out_frame_counter_,
                  tetris_flash_frame_};
}

void GameProcessor::restoreSnapshot(Snapshot&& snapshot) {
  state_ = std::move(snapshot.state);
  statistics_ = std::move(snapshot.statistics);
  tetromino_rng_ = std::move(snapshot.tetromino_rng);
  entry_delay_random_engine_ = snapshot.entry_delay_random_engine;
  line_clear_info_ = std::move(snapshot.line_clear_info);
  top_out_frame_counter_ = snapshot.top_out_frame_counter;
  tetris_flash_frame_ = snapshot.tetris_flash_frame;
}

ProgramFlowSignal GameProcessor::simulateFrame(const KeyEvents& key_events) {
  tetris_flash_frame_.reset();
  if (state_.topped_out) {
    const bool end_game = updateTopOutState(key_events, top_out_frame_counter_, state_);
    if (end_game) {
      return ProgramFlowSignal::LevelSelectorScreen;
    }
  } else if (state_.paused) {
    if (key_events.at(KeyAction::Start).pressed) {
      state_.paused = false;
    }
  } else if (entryDelay(state_)) {
    doEntryDelayStep(key_events);
  } else {
    doGravityStep(key_events);
  }
  return ProgramFlowSignal::FrameSuccess;
}

void GameProcessor::renderFrame(const KeyEvents& key_events) {
  if (tetris_flash_frame_.has_value()) {
    renderer_.doTetrisFlash(*tetris_flash_frame_);
  }
  renderer_.renderGameState(state_, statistics_, show_controls_, show_das_bar_, statistics_mode_,
                            key_events, das_processor_);
}

void GameProcessor::endFrame() {
  state_.viz_wall_charge_frame_count = std::max(state_.viz_wall_charge_frame_count - 1, 0);
}

namespace {

// A + B + Start returns to the title screen, like the soft reset on the NES.
bool softResetPressed(const KeyEvents& key_events) {
  bool all_down = true;
  bool any_pressed = false;
  for (const auto action : {KeyAction::RotateClockwise, KeyAction::RotateAntiClockwise,
                            KeyAction::Start}) {
    const auto& event = key_events.at(action);
    all_down = all_down && (event.pressed || event.held);
    any_pressed = any_pressed || event.pressed;
  }
  return all_down && any_pressed;
}

// The input for frames that have not happened yet: buttons that are down stay held.
KeyEvents predictKeyEvents(const KeyEvents& key_events) {
  KeyEvents predicted;
  for (const auto& [action, event] : key_events) {
    KeyEvent predicted_event{};
    predicted_event.held = event.pressed || event.held;
    predicted[action] = predicted_event;
  }
  return predicted;
}

}  // namespace

ProgramFlowSignal GameProcessor::processFrame(const KeyEvents& key_events) {
  if (softResetPressed(key_events)) {
    finishGame(false);
    return ProgramFlowSignal::LevelSelectorScreen;
  }

  ++frames_played_;
  const auto signal = simulateFrame(key_events);
  if (signal != ProgramFlowSignal::FrameSuccess) {
    finishGame(true);
    return signal;
  }

  if (run_ahead_frames_ == 0) {
    renderFrame(key_events);
    endFrame();
    return signal;
  }

  // Run ahead: show the frame `run_ahead_frames_` into the future, assuming the buttons stay as
  // they are, to compensate for display lag. Then go back to the real frame, so the next real
  // input is applied to the real game state. Sounds only play for real frames.
  endFrame();
  auto real_frame = saveSnapshot();
  const auto predicted_key_events = predictKeyEvents(key_events);
  sample_player_->setMuted(true);
  for (int i = 0; i < run_ahead_frames_; ++i) {
    if (i > 0) {
      endFrame();
    }
    simulateFrame(predicted_key_events);
  }
  sample_player_->setMuted(false);
  renderFrame(key_events);
  restoreSnapshot(std::move(real_frame));
  return signal;
}

void GameProcessor::finishGame(const bool topped_out) {
  GameRecord record;
  record.mode = game_mode_;
  record.mode_setting = mode_setting_;
  record.start_level = start_level_;
  record.end_level = state_.level;
  record.lines = state_.lines;
  record.score = state_.score;
  record.start_score = state_.start_score;
  record.tetris_rate = static_cast<int>(
      std::lround(statistics_.getTetrisRate() * 100));
  for (int i = 0; i < 7; ++i) {
    record.pieces += statistics_.getTetrominoCount(Tetromino{i});
  }
  record.duration_s = static_cast<double>(frames_played_) / std::max(1, game_frequency_);
  record.run_ahead_frames = run_ahead_frames_;
  record.topped_out = topped_out;
  finished_game_ = record;
}

std::optional<GameRecord> GameProcessor::takeFinishedGame() {
  auto record = std::move(finished_game_);
  finished_game_.reset();
  return record;
}

};  // namespace nestris_x86

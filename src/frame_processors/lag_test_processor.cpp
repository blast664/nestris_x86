#include "frame_processors/lag_test_processor.hpp"

#include "drawing_utils.hpp"

namespace nestris_x86 {

using pdi = PixelDrawingInterface;

namespace {

constexpr int BAR_X = 32;
constexpr pdi::Rect BAR_SIZE{192, 20};
// Top, middle and bottom of the screen, to show the screen being drawn from top to bottom.
constexpr int BAR_Y[] = {60, 103, 170};

void drawBar(const PixelDrawingInterface& drawer, const int y, const bool lit) {
  drawer.fillRect(pdi::Coords{BAR_X, y}, BAR_SIZE, lit ? pdi::WHITE() : pdi::DARK_GREY());
  if (not lit) {
    drawer.fillRect(pdi::Coords{BAR_X + 1, y + 1},
                    pdi::Rect{BAR_SIZE.width - 2, BAR_SIZE.height - 2}, pdi::BLACK());
  }
}

}  // namespace

LagTestProcessor::LagTestProcessor(std::unique_ptr<PixelDrawingInterface>&& drawer,
                                   const std::shared_ptr<sound::SoundPlayer>& sample_player,
                                   const std::shared_ptr<SpriteProvider>& sprite_provider)
    : drawer_(std::move(drawer)),
      sample_player_(sample_player),
      sprite_provider_(sprite_provider),
      press_count_{},
      frame_counter_{std::make_unique<std::atomic_int>(0)} {}

void LagTestProcessor::renderLagTestScreen(const bool lit) const {
  drawer_->drawSprite(0, 0, sprite_provider_->getSprite("options-background"));
  drawer_->drawString(96, 17, "LAG TEST");
  drawer_->drawString(BAR_X, 36, "FILM IN SLOW MOTION.");
  drawer_->drawString(BAR_X, 46, "A LIGHTS UP THE BARS.");

  for (const int y : BAR_Y) {
    drawBar(*drawer_, y, lit);
  }

  // A counter that changes every frame, to count game frames in the video.
  drawer_->drawString(BAR_X, 140, "FRAME");
  drawNumber(*drawer_, BAR_X + 48, 140, (*frame_counter_)++ % 10000, 4);
  drawer_->drawString(BAR_X + 112, 140, "PRESS");
  drawNumber(*drawer_, BAR_X + 160, 140, press_count_ % 1000, 3);

  drawer_->drawString(BAR_X, 198, "START GOES BACK.");
}

ProgramFlowSignal LagTestProcessor::processFrame(const KeyEvents& key_events) {
  if (key_events.at(KeyAction::Start).pressed ||
      key_events.at(KeyAction::RotateAntiClockwise).pressed) {
    sample_player_->playSample("menu_blip");
    *frame_counter_ = 0;
    press_count_ = 0;
    return ProgramFlowSignal::OptionsScreen;
  }

  const auto& a_button = key_events.at(KeyAction::RotateClockwise);
  if (a_button.pressed) {
    ++press_count_;
  }
  renderLagTestScreen(a_button.pressed || a_button.held);
  return ProgramFlowSignal::FrameSuccess;
}

}  // namespace nestris_x86

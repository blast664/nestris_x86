#pragma once

#include <atomic>
#include <memory>

#include "assets.hpp"
#include "drawers/pixel_drawing_interface.hpp"
#include "frame_processor_interface.hpp"
#include "sound.hpp"

namespace nestris_x86 {

/**
 * A screen for measuring the lag from button press to picture with a slow motion camera.
 *
 * Three bars at the top, middle and bottom of the screen light up on the frame the game registers
 * the A button, and stay lit while it is held. Filming the button and the screen together, the
 * lag is the number of video frames between the button going down and the bars lighting up. The
 * bars go through the same input and drawing path as the game itself.
 */
class LagTestProcessor : public FrameProcessorInterface {
 public:
  LagTestProcessor(std::unique_ptr<PixelDrawingInterface>&& drawer,
                   const std::shared_ptr<sound::SoundPlayer>& sample_player,
                   const std::shared_ptr<SpriteProvider>& sprite_provider);

  ProgramFlowSignal processFrame(const KeyEvents& key_events);

 private:
  void renderLagTestScreen(const bool lit) const;

  std::unique_ptr<PixelDrawingInterface> drawer_;
  std::shared_ptr<sound::SoundPlayer> sample_player_;
  std::shared_ptr<SpriteProvider> sprite_provider_;
  int press_count_;

  // See OptionScreenProcessor::frame_counter_ for why this is wrapped.
  std::unique_ptr<std::atomic_int> frame_counter_;
};

}  // namespace nestris_x86

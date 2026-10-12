#pragma once

#include <map>
#include <memory>
#include <string>

struct Mix_Chunk;

namespace sound {
class SoundPlayer {
 public:
  SoundPlayer();

  ~SoundPlayer();

  [[nodiscard]] bool loadWavFromFilesystem(const std::string& path, const std::string& sample_name);

  [[nodiscard]] bool loadWavFromMemory(std::unique_ptr<Mix_Chunk>&& sample,
                                       const std::string& sample_name);

  bool playSample(const std::string& sample_name) const;

  // While muted, playSample does nothing. Used for frames that are only simulated, not shown.
  void setMuted(const bool muted) { muted_ = muted; }

 private:
  std::map<std::string, std::unique_ptr<Mix_Chunk>> samples_;
  bool muted_{};
};
}  // namespace sound

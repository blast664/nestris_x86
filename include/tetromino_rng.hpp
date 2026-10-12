#pragma once

#include <array>
#include <map>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

#include "tetromino.hpp"

namespace nestris_x86 {

enum class RngType { Nes, Uniform, SevenBag };

inline RngType rngTypeFromString(const std::string& rng_type) {
  if (rng_type == "NES") {
    return RngType::Nes;
  } else if (rng_type == "UNIF") {
    return RngType::Uniform;
  } else if (rng_type == "7BAG") {
    return RngType::SevenBag;
  }
  throw std::runtime_error("Unknown rng type `" + rng_type + "`.");
  return RngType::Nes;
}

class TetrominoRNG {
 public:
  virtual ~TetrominoRNG() = default;
  virtual Tetromino getRandomTetromino() = 0;
  // Copies the generator including its state, so the same pieces can be generated again.
  virtual std::unique_ptr<TetrominoRNG> clone() const = 0;
};

class UniformTetrominoRNG : public TetrominoRNG {
 public:
  UniformTetrominoRNG();
  Tetromino getRandomTetromino() override;
  std::unique_ptr<TetrominoRNG> clone() const override;

 private:
  std::mt19937 random_number_engine_;
  std::uniform_int_distribution<int> random_generator_;
};

std::vector<Tetromino> createBiasedLookupTable(const std::map<Tetromino, int>& tetromino_counts);
class NesTetrominoRNG : public TetrominoRNG {
 public:
  NesTetrominoRNG();
  Tetromino getRandomTetromino() override;
  std::unique_ptr<TetrominoRNG> clone() const override;

 private:
  Tetromino biasedReroll();
  std::mt19937 random_number_engine_;
  std::uniform_int_distribution<int> random_generator_8_;
  std::uniform_int_distribution<int> random_generator_56_;
  std::vector<Tetromino> biased_lookup_;
  Tetromino last_tetromino_;
};

class SevenBagTetrominoRNG : public TetrominoRNG {
 public:
  SevenBagTetrominoRNG();
  Tetromino getRandomTetromino() override;
  std::unique_ptr<TetrominoRNG> clone() const override;

 private:
  void shuffleBag();

  std::mt19937 random_number_engine_;
  std::array<Tetromino, 7> seven_bag_;
  int idx_;
};

// Only long bars, for the quicktap trainer.
class LongbarTetrominoRNG : public TetrominoRNG {
 public:
  Tetromino getRandomTetromino() override { return Tetromino::Line; }
  std::unique_ptr<TetrominoRNG> clone() const override {
    return std::make_unique<LongbarTetrominoRNG>(*this);
  }
};

// Wraps another generator and keeps only `longbar_percent` percent of its long bars, for the
// drought trainer. A dropped long bar is replaced by a random other piece, so the long bar rate is
// exactly `longbar_percent` percent of the wrapped generator's rate.
class DroughtTetrominoRNG : public TetrominoRNG {
 public:
  DroughtTetrominoRNG(std::unique_ptr<TetrominoRNG>&& base_rng, const int longbar_percent);
  DroughtTetrominoRNG(const DroughtTetrominoRNG& other);
  Tetromino getRandomTetromino() override;
  std::unique_ptr<TetrominoRNG> clone() const override;

 private:
  std::unique_ptr<TetrominoRNG> base_rng_;
  int longbar_percent_;
  std::mt19937 random_number_engine_;
  std::uniform_int_distribution<int> percent_generator_;
  std::uniform_int_distribution<int> other_piece_generator_;
};

inline std::unique_ptr<TetrominoRNG> tetrominoRngFactory(const RngType& rng_type) {
  switch (rng_type) {
    case (RngType::Nes):
      return std::make_unique<NesTetrominoRNG>();
    case (RngType::Uniform):
      return std::make_unique<UniformTetrominoRNG>();
    case (RngType::SevenBag):
      return std::make_unique<SevenBagTetrominoRNG>();
  }
  throw std::runtime_error("Unhandled rng_type in tetrominoFactory.");
  return nullptr;
}

}  // namespace nestris_x86

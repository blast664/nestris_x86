#include "tetromino_rng.hpp"

#include <algorithm>
#include <map>

#include "tetromino.hpp"

namespace nestris_x86 {

// The engines are seeded from hardware randomness when available, so every game is different.
UniformTetrominoRNG::UniformTetrominoRNG()
    : random_number_engine_{std::random_device{}()}, random_generator_{0, 6} {}

std::unique_ptr<TetrominoRNG> UniformTetrominoRNG::clone() const {
  return std::make_unique<UniformTetrominoRNG>(*this);
}

Tetromino UniformTetrominoRNG::getRandomTetromino() {
  return Tetromino{random_generator_(random_number_engine_)};
}

// Taken from https://meatfighter.com/nintendotetrisai/#Spawning_Tetriminos
const std::map<Tetromino, int> BIASED_TETROMINO_COUNTS{{Tetromino::T, 9},       //
                                                       {Tetromino::J, 8},       //
                                                       {Tetromino::Z, 8},       //
                                                       {Tetromino::Square, 8},  //
                                                       {Tetromino::S, 9},       //
                                                       {Tetromino::L, 7},       //
                                                       {Tetromino::Line, 7}};   //

std::vector<Tetromino> createBiasedLookupTable(const std::map<Tetromino, int>& tetromino_counts) {
  std::vector<Tetromino> biased_lookup;
  int idx = 0;
  for (const auto& [tetromino, count] : tetromino_counts) {
    for (int i = 0; i < count; ++i) {
      biased_lookup.push_back(tetromino);
    }
  }
  return biased_lookup;
}

NesTetrominoRNG::NesTetrominoRNG()
    : random_number_engine_{std::random_device{}()},
      random_generator_8_{0, 7},
      random_generator_56_{0, 55},
      biased_lookup_{createBiasedLookupTable(BIASED_TETROMINO_COUNTS)},
      last_tetromino_{} {}

std::unique_ptr<TetrominoRNG> NesTetrominoRNG::clone() const {
  return std::make_unique<NesTetrominoRNG>(*this);
}

Tetromino NesTetrominoRNG::getRandomTetromino() {
  // The following is a rough approximation of how the random tetromino generation code worked
  // in the original nes implementation. A detailed description of the code may be found here:
  // https://meatfighter.com/nintendotetrisai/#Spawning_Tetriminos
  const auto roll_1 = random_generator_8_(random_number_engine_);
  Tetromino ret_val{};
  if (roll_1 == 7 || Tetromino{roll_1} == last_tetromino_) {
    ret_val = biasedReroll();
  } else {
    ret_val = Tetromino{roll_1};
  }
  last_tetromino_ = ret_val;
  return ret_val;
}

Tetromino NesTetrominoRNG::biasedReroll() {
  const auto reroll = random_generator_56_(random_number_engine_);
  return biased_lookup_.at(reroll);
}

SevenBagTetrominoRNG::SevenBagTetrominoRNG()
    : random_number_engine_{std::random_device{}()}, seven_bag_{}, idx_{0} {
  for (int i = 0; i < 7; ++i) {
    seven_bag_[i] = Tetromino{i};
  }
  shuffleBag();
}

std::unique_ptr<TetrominoRNG> SevenBagTetrominoRNG::clone() const {
  return std::make_unique<SevenBagTetrominoRNG>(*this);
}

void SevenBagTetrominoRNG::shuffleBag() {
  std::shuffle(seven_bag_.begin(), seven_bag_.end(), random_number_engine_);
}

Tetromino SevenBagTetrominoRNG::getRandomTetromino() {
  if (idx_ > 6) {
    shuffleBag();
    idx_ = 0;
  }
  return seven_bag_.at(idx_++);
}

DroughtTetrominoRNG::DroughtTetrominoRNG(std::unique_ptr<TetrominoRNG>&& base_rng,
                                         const int longbar_percent)
    : base_rng_{std::move(base_rng)},
      longbar_percent_{longbar_percent},
      random_number_engine_{std::random_device{}()},
      percent_generator_{0, 99},
      other_piece_generator_{0, static_cast<int>(Tetromino::Line) - 1} {}

DroughtTetrominoRNG::DroughtTetrominoRNG(const DroughtTetrominoRNG& other)
    : base_rng_{other.base_rng_->clone()},
      longbar_percent_{other.longbar_percent_},
      random_number_engine_{other.random_number_engine_},
      percent_generator_{other.percent_generator_},
      other_piece_generator_{other.other_piece_generator_} {}

std::unique_ptr<TetrominoRNG> DroughtTetrominoRNG::clone() const {
  return std::make_unique<DroughtTetrominoRNG>(*this);
}

Tetromino DroughtTetrominoRNG::getRandomTetromino() {
  auto tetromino = base_rng_->getRandomTetromino();
  if (tetromino == Tetromino::Line &&
      percent_generator_(random_number_engine_) >= longbar_percent_) {
    tetromino = Tetromino{other_piece_generator_(random_number_engine_)};
  }
  return tetromino;
}

}  // namespace nestris_x86

#pragma once

#include <map>

#include "tetromino.hpp"

namespace nestris_x86 {

class Statistics {
 public:
  Statistics();
  void update(const int lines_cleared, const int level);
  void update(const Tetromino& new_tetromino);
  void dasResetSignal();

  int getTetrominoCount(const Tetromino& tetromino) const;
  // The fraction of cleared lines that were cleared with tetrises.
  double getTetrisRate() const;

  int getBurnCount() const;
  int getLongBarDrought() const;
  int getDasChain() const;

 private:
  std::map<Tetromino, int> tetromino_counts_;
  int lines_from_tetrises_;
  int lines_cleared_;
  int long_bar_drought_;
  int das_chain_counter_;
  int burn_counter_;
};

}  // namespace nestris_x86

#include "game_history.hpp"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

#include "utils/logging.hpp"

namespace nestris_x86 {

namespace {

const std::string CSV_HEADER =
    "timestamp,mode,mode_setting,start_level,end_level,lines,score,start_score,tetris_rate,"
    "pieces,duration_s,run_ahead_frames,end";

std::string currentTimestamp() {
  const std::time_t now = std::time(nullptr);
  std::tm local_time{};
#ifdef _WIN32
  localtime_s(&local_time, &now);
#else
  localtime_r(&now, &local_time);
#endif
  std::ostringstream ss;
  ss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

std::string toCsvLine(const GameRecord& record) {
  std::ostringstream ss;
  ss << record.timestamp << ',' << gameModeToString(record.mode) << ',' << record.mode_setting
     << ',' << record.start_level << ',' << record.end_level << ',' << record.lines << ','
     << record.score << ',' << record.start_score << ',' << record.tetris_rate << ','
     << record.pieces << ',' << std::fixed << std::setprecision(1) << record.duration_s << ','
     << record.run_ahead_frames << ',' << (record.topped_out ? "topout" : "reset");
  return ss.str();
}

bool fromCsvLine(const std::string& line, GameRecord& record) try {
  std::vector<std::string> fields;
  std::stringstream ss(line);
  std::string field;
  while (std::getline(ss, field, ',')) {
    fields.push_back(field);
  }
  if (fields.size() != 13) {
    return false;
  }
  record.timestamp = fields[0];
  record.mode = gameModeFromString(fields[1]);
  record.mode_setting = std::stoi(fields[2]);
  record.start_level = std::stoi(fields[3]);
  record.end_level = std::stoi(fields[4]);
  record.lines = std::stoi(fields[5]);
  record.score = std::stoi(fields[6]);
  record.start_score = std::stoi(fields[7]);
  record.tetris_rate = std::stoi(fields[8]);
  record.pieces = std::stoi(fields[9]);
  record.duration_s = std::stod(fields[10]);
  record.run_ahead_frames = std::stoi(fields[11]);
  record.topped_out = fields[12] == "topout";
  return true;
} catch (const std::exception&) {
  return false;
}

}  // namespace

GameHistory::GameHistory(const std::string& path) : path_{path}, records_{} {
  load();
}

void GameHistory::load() {
  std::ifstream ifs(path_);
  std::string line;
  int line_number = 0;
  while (std::getline(ifs, line)) {
    ++line_number;
    if (line_number == 1 && line == CSV_HEADER) {
      continue;
    }
    GameRecord record;
    if (fromCsvLine(line, record)) {
      records_.push_back(record);
    } else if (not line.empty()) {
      LOG_ERROR("Skipping bad line " << line_number << " in `" << path_ << "`.");
    }
  }
  if (not records_.empty()) {
    LOG_INFO("Loaded " << records_.size() << " games from `" << path_ << "`.");
  }
}

void GameHistory::add(GameRecord record) {
  if (record.timestamp.empty()) {
    record.timestamp = currentTimestamp();
  }
  const bool new_file = not std::ifstream(path_).good();
  std::ofstream ofs(path_, std::ios::app);
  if (not ofs.good()) {
    LOG_ERROR("Could not write game history to `" << path_ << "`.");
  } else {
    if (new_file) {
      ofs << CSV_HEADER << '\n';
    }
    ofs << toCsvLine(record) << '\n';
  }
  records_.push_back(record);
}

std::vector<GameRecord> GameHistory::scoredGames(const GameMode& mode,
                                                 const std::optional<int>& start_level) const {
  std::vector<GameRecord> games;
  std::copy_if(records_.begin(), records_.end(), std::back_inserter(games),
               [&](const GameRecord& record) {
                 return record.mode == mode && (record.topped_out || record.lines > 0) &&
                        (not start_level.has_value() || record.start_level == *start_level);
               });
  return games;
}

int GameHistory::bestScore(const GameMode& mode, const int start_level) const {
  int best = 0;
  for (const auto& record : scoredGames(mode, start_level)) {
    best = std::max(best, record.score);
  }
  return best;
}

std::vector<int> GameHistory::startLevels(const GameMode& mode) const {
  std::set<int> levels;
  for (const auto& record : scoredGames(mode)) {
    levels.insert(record.start_level);
  }
  return {levels.begin(), levels.end()};
}

}  // namespace nestris_x86

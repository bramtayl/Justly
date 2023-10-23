#pragma once

#include <nlohmann/json_fwd.hpp>  // for json
#include <string>

#include "justly/notechord/NoteChord.h"  // for NoteChord

class Note : public NoteChord {
 public:
  Note() = default;
  explicit Note(const nlohmann::json &);
  ~Note() override = default;
  [[nodiscard]] auto symbol_for() const -> std::string override;
  [[nodiscard]] static auto get_schema() -> const nlohmann::json &;
};

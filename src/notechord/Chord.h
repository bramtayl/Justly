#pragma once

#include <memory>  // for unique_ptr

#include "notechord/NoteChord.h"  // for NoteChord

class QString;  // lines 11-11

#include <nlohmann/json_fwd.hpp>  // for json

class Chord : public NoteChord {
 public:
  Chord();
  ~Chord() override = default;
  [[nodiscard]] auto get_level() const -> TreeLevel override;

  [[nodiscard]] auto new_child_pointer() -> std::unique_ptr<NoteChord> override;
  [[nodiscard]] auto symbol_for() const -> QString override;
  [[nodiscard]] static auto get_schema() -> const nlohmann::json &;
};

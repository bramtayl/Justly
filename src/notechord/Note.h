#pragma once

#include <nlohmann/json_fwd.hpp>  // for json

#include "notechord/NoteChord.h"  // for NoteChord

class QString;  // lines 11-11
class Chord;

class Note : public NoteChord {
 public:
  Chord* parent_chord_pointer;
  explicit Note(Chord*);
  Note(Chord*, const nlohmann::json &);
  ~Note() override = default;
  [[nodiscard]] auto symbol_for() const -> QString override;
  [[nodiscard]] static auto get_schema() -> const nlohmann::json &;
};

#pragma once

#include <gsl/pointers>           // for not_null
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>

#include "notechord/NoteChord.h"  // for NoteChord

class Chord;

class Note : public NoteChord {
 public:
  gsl::not_null<Chord*> parent_chord_pointer;
  explicit Note(gsl::not_null<Chord*>);
  Note(gsl::not_null<Chord*>, const nlohmann::json &);
  ~Note() override = default;
  [[nodiscard]] auto symbol_for() const -> std::string override;
  [[nodiscard]] static auto get_schema() -> const nlohmann::json &;
};

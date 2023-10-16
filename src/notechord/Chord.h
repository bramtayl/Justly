#pragma once

#include <memory>  // for unique_ptr
#include <vector>

#include "notechord/NoteChord.h"  // for NoteChord

class QString;  // lines 11-11

#include "Note.h"
#include <nlohmann/json_fwd.hpp>  // for json

class Chord : public NoteChord {
 public:
  Chord() = default;
  explicit Chord(const nlohmann::json &);
  std::vector<std::unique_ptr<Note>> note_pointers;
  ~Chord() override = default;

  [[nodiscard]] auto symbol_for() const -> QString override;
  [[nodiscard]] static auto get_schema() -> const nlohmann::json &;
  [[nodiscard]] auto to_json() const -> nlohmann::json override;
  [[nodiscard]] auto notes_to_json(int, int) const -> nlohmann::json;
  void insert_empty_notes(int, int);
  void remove_notes(int, int);
  void insert_json_notes(int, const nlohmann::json &);
};

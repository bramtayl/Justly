#pragma once

#include <memory>  // for unique_ptr
#include <vector>

#include "notechord/NoteChord.h"  // for NoteChord

class QString;  // lines 11-11

#include "Note.h"
#include <nlohmann/json_fwd.hpp>  // for json

class Chord : public NoteChord {
 public:
  Chord();
  std::vector<std::unique_ptr<Note>> note_pointers;
  ~Chord() override = default;
  [[nodiscard]] auto get_level() const -> TreeLevel override;

  [[nodiscard]] auto new_child_pointer() -> std::unique_ptr<NoteChord> override;
  [[nodiscard]] auto symbol_for() const -> QString override;
  [[nodiscard]] static auto get_schema() -> const nlohmann::json &;
  void save_to(nlohmann::json *) const override;
  [[nodiscard]] auto copy_json_notes(int, int) const -> nlohmann::json;
  void insert_empty_notes(int, int);
  void remove_notes(int, int);
  void insert_json_notes(int, const nlohmann::json &);
};

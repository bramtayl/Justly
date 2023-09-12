#pragma once

#include <memory>  // for unique_ptr

#include "NoteChord.h"  // for NoteChord

class QString;  // lines 11-11

namespace nlohmann::json_schema {
class json_validator;
}

const auto CHORD_COLUMNS = 8;

class Chord : public NoteChord {
 public:
  Chord();
  ~Chord() override = default;
  [[nodiscard]] auto get_level() const -> TreeLevel override;

  [[nodiscard]] auto new_child_pointer() -> std::unique_ptr<NoteChord> override;
  [[nodiscard]] auto symbol_for() const -> QString override;
  [[nodiscard]] static auto get_list_validator()
      -> nlohmann::json_schema::json_validator &;
  [[nodiscard]] static auto get_schema() -> QString &;

  [[nodiscard]] static auto verify_json_items(const QString &chord_text)
      -> bool;
};

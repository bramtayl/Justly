#pragma once

#include <qjsonvalue.h>  // for QJsonObject

#include <memory>  // for unique_ptr

#include "NoteChord.h"  // for NoteChord

class QString;  // lines 11-11
class Song;

const auto NOTE_COLUMNS = 9;

class Note : public NoteChord {
 public:
  Note();
  ~Note() override = default;
  [[nodiscard]] auto get_level() const -> TreeLevel override;
  [[nodiscard]] auto symbol_for() const -> QString override;
  [[nodiscard]] auto new_child_pointer() -> std::unique_ptr<NoteChord> override;

  [[nodiscard]] static auto verify_json(
      const Song& song, const QJsonValue &note_value)
      -> bool;
};

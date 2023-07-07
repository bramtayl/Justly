#pragma once

#include <qjsonvalue.h>  // for QJsonObject

#include <memory>  // for unique_ptr

#include "NoteChord.h"  // for NoteChord

class QString;  // lines 11-11
class Song;

const auto CHORD_COLUMNS = 8;

class Chord : public NoteChord {
 public:
  Chord();
  ~Chord() override = default;
  [[nodiscard]] auto get_level() const -> TreeLevel override;

  [[nodiscard]] auto new_child_pointer() -> std::unique_ptr<NoteChord> override;
  [[nodiscard]] auto symbol_for() const -> QString override;

  [[nodiscard]] static auto verify_json(const Song& song, const QJsonValue &chord_value)
      -> bool;
};

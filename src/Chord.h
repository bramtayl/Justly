#pragma once

#include <qjsonobject.h>  // for QJsonObject
#include <qnamespace.h>   // for ItemFlags
#include <qvariant.h>     // for QVariant

#include <memory>  // for unique_ptr
#include <set>     // for set

#include "NoteChord.h"  // for NoteChord
class QString;

const auto CHORD_COLUMNS = 8;
const auto CHORD_LEVEL = 1;

class Chord : public NoteChord {
 public:
  ~Chord() override = default;
  Chord(const std::set<QString> &instruments);
  [[nodiscard]] auto get_level() const -> int override;

  [[nodiscard]] auto flags(int column) const -> Qt::ItemFlags override;
  [[nodiscard]] auto data(int column, int role) const -> QVariant override;
  void load(const QJsonObject &json_note_chord) override;
  auto save(QJsonObject &json_map) const -> void override;
  auto setData(int column, const QVariant &new_value, int role)
      -> bool override;
  auto copy_pointer() -> std::unique_ptr<NoteChord> override;
};

#pragma once

#include <qjsonobject.h>  // for QJsonObject
#include <qnamespace.h>   // for ItemFlags
#include <qvariant.h>     // for QVariant

#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include "NoteChord.h"  // for NoteChord
class QString;          // lines 11-11

const auto NOTE_COLUMNS = 9;
const auto NOTE_LEVEL = 2;

class Note : public NoteChord {
 public:
  ~Note() override = default;
  Note(const std::vector<std::unique_ptr<const QString>> *const instruments_pointer, const QString& default_instrument);
  [[nodiscard]] auto get_level() const -> int override;

  [[nodiscard]] auto flags(int column) const -> Qt::ItemFlags override;
  void load(const QJsonObject &json_note_chord) override;
  auto save(QJsonObject &json_map) const -> void override;
  [[nodiscard]] auto data(int column, int role) const -> QVariant override;
  auto setData(int column, const QVariant &new_value, int role)
      -> bool override;
  auto copy_pointer() -> std::unique_ptr<NoteChord> override;
};

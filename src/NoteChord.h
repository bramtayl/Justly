#pragma once

#include <qjsonobject.h>  // for QJsonObject
#include <qstring.h>      // for QString
#include <qvariant.h>     // for QVariant

#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include "Interval.h"   // for Interval
#include "Utilities.h"  // for TreeLevel

class Instrument;

const auto DEFAULT_BEATS = 1;
const auto DEFAULT_VOLUME_PERCENT = 100.0;
const auto DEFAULT_TEMPO_PERCENT = 100.0;
const auto DEFAULT_WORDS = "";

const auto NOTE_CHORD_COLUMNS = 7;

enum ChordNoteFields {
  symbol_column = 0,
  words_column = 1,
  interval_column = 2,
  beats_column = 3,
  volume_percent_column = 4,
  tempo_percent_column = 5,
  instrument_column = 6
};

class NoteChord {
 public:
  Interval interval = Interval();
  int beats = 1;
  double volume_percent = DEFAULT_VOLUME_PERCENT;
  double tempo_percent = DEFAULT_TEMPO_PERCENT;
  QString words = DEFAULT_WORDS;
  QString instrument = "";
  virtual ~NoteChord() = default;

  virtual auto copy_pointer() -> std::unique_ptr<NoteChord> = 0;
  [[nodiscard]] virtual auto get_level() const -> TreeLevel = 0;
  void load(const QJsonObject &json_note_chord);
  [[nodiscard]] auto data(int column, int role) const -> QVariant;
  [[nodiscard]] auto setData(int column, const QVariant &value) -> bool;
  void save(QJsonObject &json_map) const;
  [[nodiscard]] virtual auto new_child_pointer()
      -> std::unique_ptr<NoteChord> = 0;
  [[nodiscard]] virtual auto symbol_for() const -> QString = 0;
  [[nodiscard]] static auto verify_json_note_chord_field(
      const QJsonObject &json_note_chord, const QString &field_name,
      const std::vector<Instrument> &instruments) -> bool;
};

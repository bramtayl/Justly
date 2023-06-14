#pragma once

#include <qjsonobject.h>  // for QJsonObject
#include <qnamespace.h>   // for ItemFlags
#include <qstring.h>      // for QString
#include <qvariant.h>     // for QVariant

#include <memory>  // for unique_ptr

const auto DEFAULT_NUMERATOR = 1;
const auto DEFAULT_DENOMINATOR = 1;
const auto DEFAULT_OCTAVE = 0;
const auto DEFAULT_BEATS = 1;
const auto DEFAULT_VOLUME_PERCENT = 100.0;
const auto DEFAULT_TEMPO_PERCENT = 100.0;
const auto OCTAVE_RATIO = 2.0;

enum TreeLevel {
  root_level = 0,
  chord_level = 1,
  note_level = 2,
};

enum ChordNoteFields {
  symbol_column = 0,
  words_column = 1,
  numerator_column = 2,
  denominator_column = 3,
  octave_column = 4,
  beats_column = 5,
  volume_percent_column = 6,
  tempo_percent_column = 7,
  instrument_column = 8
};

class NoteChord {
 public:
  const QString& default_instrument;
  int numerator = DEFAULT_NUMERATOR;
  int denominator = DEFAULT_DENOMINATOR;
  int octave = DEFAULT_OCTAVE;
  int beats = DEFAULT_BEATS;
  double volume_percent = DEFAULT_VOLUME_PERCENT;
  double tempo_percent = DEFAULT_TEMPO_PERCENT;
  QString words;
  QString instrument = default_instrument;

  explicit NoteChord(const QString& default_instrument);
  virtual ~NoteChord() = default;

  virtual auto copy_pointer() -> std::unique_ptr<NoteChord> = 0;

  [[nodiscard]] virtual auto flags(int column) const -> Qt::ItemFlags = 0;
  [[nodiscard]] virtual auto get_level() const -> TreeLevel = 0;
  virtual void load(const QJsonObject& json_note_chord) = 0;
  [[nodiscard]] virtual auto data(int column, int role) const -> QVariant = 0;
  virtual void setData(int column, const QVariant& value) = 0;
  virtual auto save(QJsonObject& json_map) const -> void = 0;
  virtual auto get_instrument() -> QString = 0;
};

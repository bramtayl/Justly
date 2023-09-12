#pragma once

#include <qcolor.h>       // for QColor
#include <qjsonobject.h>  // for QJsonObject
#include <qnamespace.h>   // for black, lightGray
#include <qstring.h>      // for QString
#include <qvariant.h>     // for QVariant

#include <memory>  // for unique_ptr

#include "Interval.h"  // for Interval

const auto MINIMUM_BEATS = 1;
const auto DEFAULT_BEATS = 1;
const auto MAXIMUM_BEATS = 199;

const auto MINIMUM_VOLUME_PERCENT = 1;
const auto DEFAULT_VOLUME_PERCENT = 100.0;
const auto MAXIMUM_VOLUME_PERCENT = 400;

const auto MINIMUM_TEMPO_PERCENT = 1;
const auto DEFAULT_TEMPO_PERCENT = 100.0;
const auto MAXIMUM_TEMPO_PERCENT = 400;

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

const auto NON_DEFAULT_COLOR = QColor(Qt::black);
const auto DEFAULT_COLOR = QColor(Qt::lightGray);

enum TreeLevel {
  root_level = 0,
  chord_level = 1,
  note_level = 2,
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

  [[nodiscard]] virtual auto get_level() const -> TreeLevel = 0;
  void load_from(const QJsonObject &json_note_chord);
  [[nodiscard]] auto data(int column, int role) const -> QVariant;
  void setData(int column, const QVariant &value);
  void save_to(QJsonObject &json_map) const;
  [[nodiscard]] virtual auto new_child_pointer()
      -> std::unique_ptr<NoteChord> = 0;
  [[nodiscard]] virtual auto symbol_for() const -> QString = 0;
  [[nodiscard]] static auto get_properties_schema() -> QString&;
};

void error_level(TreeLevel level);
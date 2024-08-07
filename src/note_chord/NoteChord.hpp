#pragma once

#include <QString>
#include <QVariant>
#include <nlohmann/json.hpp>

#include "cell_values/Interval.hpp"
#include "justly/NoteChordColumn.hpp"
#include "cell_values/Rational.hpp"

class Instrument;

struct NoteChord {
private:
  void replace_cell(const NoteChord &new_note_chord,
                    NoteChordColumn note_chord_column);

public:
  const Instrument *instrument_pointer;
  Interval interval;
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
  QString words;

  NoteChord();
  explicit NoteChord(const nlohmann::json &json_note_chord);
  virtual ~NoteChord() = default;

  [[nodiscard]] virtual auto is_chord() const -> bool;
  [[nodiscard]] virtual auto get_symbol() const -> QString;
  [[nodiscard]] virtual auto to_json() const -> nlohmann::json;

  [[nodiscard]] auto data(NoteChordColumn note_chord_column) const -> QVariant;
  void setData(NoteChordColumn note_chord_column, const QVariant &new_value);
  void replace_cells(const NoteChord &new_note_chord,
                     NoteChordColumn left_column,
                     NoteChordColumn right_column);
};

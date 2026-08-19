#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/QtAssert>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
#include <libxml/parser.h>

#include "cell_types/Interval.hpp"
#include "cell_types/Rational.hpp"
#include "column_numbers/ChordColumn.hpp"
#include "other/helpers.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/Row.hpp"
#include "rows/UnpitchedNote.hpp"
#include "sound/PlayState.hpp"

struct Chord : public Row {
  Rational beats;
  Rational velocity_ratio;
  QString words;

  Interval interval;
  Rational tempo_ratio;
  QList<PitchedNote> pitched_notes;
  QList<UnpitchedNote> unpitched_notes;

  void from_xml(xmlNode &node) override;

  [[nodiscard]] static auto get_clipboard_schema() -> const char *;

  [[nodiscard]] static auto get_xml_field_name() -> const char *;

  [[nodiscard]] static auto get_number_of_columns() -> int;

  [[nodiscard]] static auto get_column_name(int column_number) -> const char *;

  [[nodiscard]] static auto get_cells_mime() -> const char *;

  [[nodiscard]] static auto is_column_editable(int column_number) -> bool;

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override;

  void set_data(const int column_number, const QVariant &new_value) override;

  void copy_column_from(const Chord &template_row, const int column_number);

  void column_to_xml(xmlNode &chord_node,
                     const int column_number) const override;

  void to_xml(xmlNode &chord_node) const override;
};

void modulate(PlayState &play_state, const Chord &chord);

void move_time(PlayState &play_state, const Chord &chord);

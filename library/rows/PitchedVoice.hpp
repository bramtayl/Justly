#pragma once

#include <QtCore/QString>
#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/QtAssert>
#include <libxml/parser.h>

#include "cell_types/Rational.hpp"
#include "column_numbers/PitchedVoiceColumn.hpp"
#include "other/helpers.hpp"
#include "rows/Row.hpp"
#include "rows/Voice.hpp"

static const auto MIDDLE_C_MIDI = 60;

struct PitchedVoice : Voice {
  PitchedVoice();

  [[nodiscard]] static auto get_pitched() -> const char *;

  [[nodiscard]] static auto is_pitched() -> bool;

  [[nodiscard]] static auto get_preview_midi_number() -> short;

  void from_xml(xmlNode &node) override;

  [[nodiscard]] static auto get_clipboard_schema() -> const char *;

  [[nodiscard]] static auto get_xml_field_name() -> const char *;

  [[nodiscard]] static auto get_number_of_columns() -> int;

  [[nodiscard]] static auto get_column_name(int column_number) -> const char *;

  [[nodiscard]] static auto get_cells_mime() -> const char *;

  [[nodiscard]] static auto is_column_editable(int /*column_number*/) -> bool;

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override;

  void set_data(const int column_number, const QVariant &new_value) override;

  void copy_column_from(const PitchedVoice &template_row,
                        const int column_number);

  void column_to_xml(xmlNode &node, const int column_number) const override;

  void to_xml(xmlNode &node) const override;
};

#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/QtAssert>
#include <libxml/parser.h>
#include <optional>

#include "cell_types/Program.hpp"
#include "cell_types/Rational.hpp"
#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "other/helpers.hpp"
#include "rows/Note.hpp"
#include "rows/Row.hpp"
#include "rows/UnpitchedVoice.hpp"
#include "rows/Voice.hpp"

struct UnpitchedNote : Note {
  void from_xml(xmlNode &node) override;

  [[nodiscard]] static auto get_clipboard_schema() -> const char *;

  [[nodiscard]] static auto get_xml_field_name() -> const char *;

  [[nodiscard]] static auto get_number_of_columns() -> int;

  [[nodiscard]] static auto get_column_name(int column_number) -> const char *;

  [[nodiscard]] static auto get_cells_mime() -> const char *;

  [[nodiscard]] static auto get_pitched() -> const char *;

  [[nodiscard]] static auto is_column_editable(int /*column_number*/) -> bool;

  [[nodiscard]] static auto is_pitched() -> bool;

  [[nodiscard]] auto
  get_closest_midi(QWidget & /*parent*/, Player & /*player*/,
                   const QList<UnpitchedVoice> &unpitched_voices,
                   const int /*channel_number*/, int /*chord_number*/,
                   int /*note_number*/) const
      -> std::optional<short> override;

  [[nodiscard]] auto get_program(const QList<PitchedVoice> & /*pitched_voices*/,
                                 const QList<UnpitchedVoice> &unpitched_voices)
      const -> const Program & override;

  [[nodiscard]] auto
  get_voice_velocity_ratio(const QList<PitchedVoice> & /*pitched_voices*/,
                           const QList<UnpitchedVoice> &unpitched_voices) const
      -> const Rational & override;

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override;

  void set_data(const int column_number, const QVariant &new_value) override;

  void copy_column_from(const UnpitchedNote &template_row,
                        const int column_number);

  void column_to_xml(xmlNode &node, const int column_number) const override;

  void to_xml(xmlNode &node) const override;
};

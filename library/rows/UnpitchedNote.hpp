#pragma once

#include <libxml/parser.h>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/QtAssert>
#include <string>

#include "cell_types/Program.hpp"
#include "cell_types/Rational.hpp"
#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "other/helpers.hpp"
#include "rows/Note.hpp"
#include "rows/Row.hpp"
#include "rows/UnpitchedVoice.hpp"
#include "rows/Voice.hpp"

struct UnpitchedNote : Note {
  void from_xml(xmlNode &node) override {
    auto *field_pointer = xmlFirstElementChild(&node);
    while (field_pointer != nullptr) {
      auto &field_node = get_reference(field_pointer);
      const auto name = get_xml_name(field_node);
      if (name == "beats") {
        set_rational_from_xml(beats, field_node);
      } else if (name == "velocity_ratio") {
        set_rational_from_xml(velocity_ratio, field_node);
      } else if (name == "words") {
        words = get_qstring_content(field_node);
      } else if (name == "voice_number") {
        voice_number = xml_to_int(field_node);
      } else {
        Q_ASSERT(false);
      }
      field_pointer = xmlNextElementSibling(field_pointer);
    }
  }

  [[nodiscard]] static auto get_clipboard_schema() -> const char * {
    return "unpitched_notes_clipboard.xsd";
  };

  [[nodiscard]] static auto get_xml_field_name() -> const char * {
    return "unpitched_note";
  };

  [[nodiscard]] static auto get_number_of_columns() -> int {
    return static_cast<int>(UnpitchedNoteColumn::number_of_unpitched_note_columns);
  };

  [[nodiscard]] static auto get_column_name(int column_number) {
    switch (column_number) {
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_voice_number_column):
      return "Voice";
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_beats_column):
      return "Beats";
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_velocity_ratio_column):
      return "Velocity ratio";
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_words_column):
      return "Words";
    default:
      Q_ASSERT(false);
      return "";
    };
  }

  [[nodiscard]] static auto get_cells_mime() {
    return "application/prs.unpitched_notes_cells+xml";
  }

  [[nodiscard]] static auto get_pitched() { return "unpitched"; }

  [[nodiscard]] static auto is_column_editable(int /*column_number*/) -> bool {
    return true;
  }

  [[nodiscard]] static auto is_pitched() { return false; }

  [[nodiscard]] auto
  get_closest_midi(QWidget & /*parent*/, Player & /*player*/,
                   const QList<UnpitchedVoice> &unpitched_voices,
                   const int /*channel_number*/, int /*chord_number*/,
                   int /*note_number*/) const -> short override {
    return unpitched_voices.at(voice_number).midi_number;
  };

  [[nodiscard]] auto get_program(const QList<PitchedVoice> & /*pitched_voices*/,
                                 const QList<UnpitchedVoice> &unpitched_voices)
      const -> const Program & override {
    return get_voice_program(get_some_programs(false), unpitched_voices, voice_number);
  };

  [[nodiscard]] auto
  get_voice_velocity_ratio(const QList<PitchedVoice> & /*pitched_voices*/,
                           const QList<UnpitchedVoice> &unpitched_voices) const
      -> const Rational & override {
    return unpitched_voices.at(voice_number).velocity_ratio;
  }

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override {
    switch (column_number) {
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_voice_number_column):
      return voice_number;
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_beats_column):
      return QVariant::fromValue(beats);
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_velocity_ratio_column):
      return QVariant::fromValue(velocity_ratio);
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_words_column):
      return words;
    default:
      Q_ASSERT(false);
      return {};
    }
  }

  void set_data(const int column_number, const QVariant &new_value) override {
    switch (column_number) {
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_voice_number_column):
      voice_number = variant_to<int>(new_value);
      break;
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_beats_column):
      beats = variant_to<Rational>(new_value);
      break;
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_velocity_ratio_column):
      velocity_ratio = variant_to<Rational>(new_value);
      break;
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_words_column):
      words = variant_to<QString>(new_value);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void copy_column_from(const UnpitchedNote &template_row,
                        const int column_number) {
    switch (column_number) {
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_voice_number_column):
      voice_number = template_row.voice_number;
      break;
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_beats_column):
      beats = template_row.beats;
      break;
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_velocity_ratio_column):
      velocity_ratio = template_row.velocity_ratio;
      break;
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_words_column):
      words = template_row.words;
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void column_to_xml(xmlNode &node, const int column_number) const override {
    switch (column_number) {
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_voice_number_column):
      set_xml_int(node, "voice_number", voice_number);
      break;
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_beats_column):
      maybe_add_rational_to_xml(node, "beats", beats);
      break;
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_velocity_ratio_column):
      maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
      break;
    case static_cast<int>(UnpitchedNoteColumn::unpitched_note_words_column):
      maybe_add_qstring_to_xml(node, "words", words);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void to_xml(xmlNode &node) const override {
    set_xml_int(node, "voice_number", voice_number);
    maybe_add_rational_to_xml(node, "beats", beats);
    maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
    maybe_add_qstring_to_xml(node, "words", words);
  }
};
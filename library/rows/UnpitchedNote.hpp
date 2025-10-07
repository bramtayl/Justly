#pragma once

#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "rows/Note.hpp"
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
      } else if (name == "voice") {
        voice = get_qstring_content(field_node);
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
    return number_of_unpitched_note_columns;
  };

  [[nodiscard]] static auto get_column_name(int column_number) {
    switch (column_number) {
    case unpitched_note_voice_column:
      return "Voice";
    case unpitched_note_beats_column:
      return "Beats";
    case unpitched_note_velocity_ratio_column:
      return "Velocity ratio";
    case unpitched_note_words_column:
      return "Words";
    default:
      Q_ASSERT(false);
      return "";
    };
  }

  [[nodiscard]] static auto get_cells_mime() {
    return "application/prs.unpitched_notes_cells+xml";
  }

  [[nodiscard]] static auto get_description() { return ", unpitched note "; }

  [[nodiscard]] static auto is_column_editable(int /*column_number*/) -> bool {
    return true;
  }

  [[nodiscard]] static auto is_pitched() { return false; }

  [[nodiscard]] auto
  get_closest_midi(QWidget & /*parent*/, Player & /*player*/,
                   const QList<UnpitchedVoice> &unpitched_voices,
                   const int /*channel_number*/, int /*chord_number*/,
                   int /*note_number*/) const -> short override {
    const auto *voice_pointer = &get_named(
        unpitched_voices, voice);
    if (voice_pointer == nullptr) {
      return 0;
    }
    return get_reference(voice_pointer).midi_number;
  };

  [[nodiscard]] auto
  get_program_pointer(QWidget &parent,
                      const QList<PitchedVoice> & /*pitched_voices*/,
                      const QList<UnpitchedVoice> &unpitched_voices,
                      const int chord_number,
                      const int note_number) const -> const Program * override {
    return get_note_program_pointer(parent, unpitched_voices, *this,
                                    chord_number, note_number);
  };

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override {
    switch (column_number) {
    case unpitched_note_voice_column:
      return voice;
    case unpitched_note_beats_column:
      return QVariant::fromValue(beats);
    case unpitched_note_velocity_ratio_column:
      return QVariant::fromValue(velocity_ratio);
    case unpitched_note_words_column:
      return words;
    default:
      Q_ASSERT(false);
      return {};
    }
  }

  void set_data(const int column_number, const QVariant &new_value) override {
    switch (column_number) {
    case unpitched_note_voice_column:
      words = variant_to<QString>(new_value);
      break;
    case unpitched_note_beats_column:
      beats = variant_to<Rational>(new_value);
      break;
    case unpitched_note_velocity_ratio_column:
      velocity_ratio = variant_to<Rational>(new_value);
      break;
    case unpitched_note_words_column:
      words = variant_to<QString>(new_value);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void copy_column_from(const UnpitchedNote &template_row,
                        const int column_number) {
    switch (column_number) {
    case unpitched_note_voice_column:
      voice = template_row.voice;
      break;
    case unpitched_note_beats_column:
      beats = template_row.beats;
      break;
    case unpitched_note_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    case unpitched_note_words_column:
      words = template_row.words;
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void column_to_xml(xmlNode &node, const int column_number) const override {
    switch (column_number) {
    case unpitched_note_voice_column:
      maybe_add_qstring_to_xml(node, "voice", voice);
      break;
    case unpitched_note_beats_column:
      maybe_add_rational_to_xml(node, "beats", beats);
      break;
    case unpitched_note_velocity_ratio_column:
      maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
      break;
    case unpitched_note_words_column:
      maybe_add_qstring_to_xml(node, "words", words);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void to_xml(xmlNode &node) const override {
    maybe_add_qstring_to_xml(node, "voice", voice);
    maybe_add_rational_to_xml(node, "beats", beats);
    maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
    maybe_add_qstring_to_xml(node, "words", words);
  }
};
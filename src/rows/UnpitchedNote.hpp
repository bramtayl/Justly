#pragma once

#include "justly/justly.hpp"
#include "cell_types/PercussionInstrument.hpp"
#include "rows/Note.hpp"

struct UnpitchedNote : Note {
  PercussionInstrument percussion_instrument;

  void from_xml(xmlNode &node) override {
    auto *field_pointer = xmlFirstElementChild(&node);
    while ((field_pointer != nullptr)) {
      auto &field_node = get_reference(field_pointer);
      const auto name = get_xml_name(field_node);
      if (name == "beats") {
        maybe_xml_to_rational(beats, field_node);
      } else if (name == "velocity_ratio") {
        maybe_xml_to_rational(velocity_ratio, field_node);
      } else if (name == "words") {
        words = get_qstring_content(field_node);
      } else if (name == "percussion_instrument") {
        set_percussion_instrument_from_xml(percussion_instrument, field_node);
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
    case unpitched_note_percussion_instrument_column:
      return "Percussion instrument";
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
  get_closest_midi(QWidget & /*parent*/, Player &player,
                   const int /*channel_number*/, int /*chord_number*/,
                   int /*note_number*/) const -> std::optional<short> override {
    if (percussion_instrument_is_default(percussion_instrument)) {
      return player.play_state.current_percussion_instrument.midi_number;
    }
    return percussion_instrument.midi_number;
  };

  [[nodiscard]] auto
  get_program_pointer(QWidget &parent, const PlayState &play_state,
                      const int chord_number,
                      const int note_number) const -> const Program * override {
    if (percussion_instrument_is_default(percussion_instrument)) {
      const auto *current_percussion_set_pointer =
          play_state.current_percussion_instrument.percussion_set_pointer;
      if (current_percussion_set_pointer == nullptr) {
        QString message;
        QTextStream stream(&message);
        stream << QObject::tr("No percussion set");
        add_note_location<UnpitchedNote>(stream, chord_number, note_number);
        QMessageBox::warning(&parent, QObject::tr("Percussion set error"),
                             message);
      }
      return current_percussion_set_pointer;
    }
    return percussion_instrument.percussion_set_pointer;
  };

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override {
    switch (column_number) {
    case unpitched_note_percussion_instrument_column:
      return QVariant::fromValue(percussion_instrument);
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
    case unpitched_note_percussion_instrument_column:
      percussion_instrument = variant_to<PercussionInstrument>(new_value);
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
    case unpitched_note_percussion_instrument_column:
      percussion_instrument = template_row.percussion_instrument;
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
    case unpitched_note_percussion_instrument_column:
      maybe_set_xml_percussion_instrument(node, "percussion_instrument",
                                          percussion_instrument);
      break;
    case unpitched_note_beats_column:
      maybe_set_xml_rational(node, "beats", beats);
      break;
    case unpitched_note_velocity_ratio_column:
      maybe_set_xml_rational(node, "velocity_ratio", velocity_ratio);
      break;
    case unpitched_note_words_column:
      maybe_set_xml_qstring(node, "words", words);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void to_xml(xmlNode &node) const override {
    maybe_set_xml_percussion_instrument(node, "percussion_instrument",
                                        percussion_instrument);
    maybe_set_xml_rational(node, "beats", beats);
    maybe_set_xml_rational(node, "velocity_ratio", velocity_ratio);
    maybe_set_xml_qstring(node, "words", words);
  }
};
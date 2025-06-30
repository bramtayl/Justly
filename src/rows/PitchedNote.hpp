#pragma once

#include "cell_types/Interval.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "rows/Note.hpp"

static const auto BEND_PER_HALFSTEP = 4096;
static const auto CONCERT_A_FREQUENCY = 440;
static const auto CONCERT_A_MIDI = 69;
static const auto HALFSTEPS_PER_OCTAVE = 12;
static const auto MAX_FREQUENCY = 12911.41; // MIDI 127 plus half step
static const auto QUARTER_STEP = 0.5;
static const auto ZERO_BEND_HALFSTEPS = 2;

[[nodiscard]] static inline auto frequency_to_midi_number(const double key) {
  Q_ASSERT(key > 0);
  return HALFSTEPS_PER_OCTAVE * log2(key / CONCERT_A_FREQUENCY) +
         CONCERT_A_MIDI;
}

[[nodiscard]] static inline auto
midi_number_to_frequency(const double midi_number) {
  return pow(OCTAVE_RATIO,
             (midi_number - CONCERT_A_MIDI) / HALFSTEPS_PER_OCTAVE) *
         CONCERT_A_FREQUENCY;
}

[[nodiscard]] static auto to_int(const double value) {
  return static_cast<int>(std::round(value));
}

struct PitchedNote : Note {
  const Program *instrument_pointer = nullptr;
  Interval interval;

  void from_xml(xmlNode &node) override {
    auto *field_pointer = xmlFirstElementChild(&node);
    while ((field_pointer != nullptr)) {
      auto &field_node = get_reference(field_pointer);
      const auto name = get_xml_name(field_node);
      if (name == "beats") {
        set_rational_from_xml(beats, field_node);
      } else if (name == "velocity_ratio") {
        set_rational_from_xml(velocity_ratio, field_node);
      } else if (name == "words") {
        words = get_qstring_content(field_node);
      } else if (name == "interval") {
        set_interval_from_xml(interval, field_node);
      } else if (name == "instrument") {
        instrument_pointer =
            &set_program_from_xml(get_some_programs(true), field_node);
      } else {
        Q_ASSERT(false);
      }
      field_pointer = xmlNextElementSibling(field_pointer);
    }
  }

  [[nodiscard]] static auto get_clipboard_schema() -> const char * {
    return "pitched_notes_clipboard.xsd";
  };

  [[nodiscard]] static auto get_xml_field_name() -> const char * {
    return "pitched_note";
  };

  [[nodiscard]] static auto get_number_of_columns() -> int {
    return number_of_pitched_note_columns;
  };

  [[nodiscard]] static auto get_column_name(int column_number) {
    switch (column_number) {
    case pitched_note_instrument_column:
      return "Instrument";
    case pitched_note_interval_column:
      return "Interval";
    case pitched_note_beats_column:
      return "Beats";
    case pitched_note_velocity_ratio_column:
      return "Velocity ratio";
    case pitched_note_words_column:
      return "Words";
    default:
      Q_ASSERT(false);
      return "";
    };
  }

  [[nodiscard]] static auto get_cells_mime() {
    return "application/prs.pitched_notes_cells+xml";
  }

  [[nodiscard]] static auto is_column_editable(int /*column_number*/) -> bool {
    return true;
  }

  [[nodiscard]] static auto get_description() { return ", pitched note "; }

  [[nodiscard]] static auto is_pitched() { return true; }

  [[nodiscard]] auto get_closest_midi(
      QWidget &parent, Player &player, const int channel_number,
      const int chord_number,
      const int note_number) const -> std::optional<short> override {
    const auto &play_state = player.play_state;
    auto &event = player.event;
    const auto frequency =
        play_state.current_key * interval_to_double(interval);
    static const auto minimum_frequency =
        midi_number_to_frequency(0 - QUARTER_STEP);
    if (frequency < minimum_frequency) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Frequency ") << frequency;
      add_note_location<PitchedNote>(stream, chord_number, note_number);
      stream << QObject::tr(" less than minimum frequency ")
             << minimum_frequency;
      QMessageBox::warning(&parent, QObject::tr("Frequency error"), message);
      return {};
    }

    if (frequency >= MAX_FREQUENCY) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Frequency ") << frequency;
      add_note_location<PitchedNote>(stream, chord_number, note_number);
      stream << QObject::tr(" greater than or equal to maximum frequency ")
             << MAX_FREQUENCY;
      QMessageBox::warning(&parent, QObject::tr("Frequency error"), message);
      return {};
    }

    const auto midi_float = frequency_to_midi_number(frequency);
    const auto closest_midi = static_cast<short>(round(midi_float));
    fluid_event_pitch_bend(
        event.internal_pointer, channel_number,
        to_int((midi_float - closest_midi + ZERO_BEND_HALFSTEPS) *
               BEND_PER_HALFSTEP));
    send_event_at(player.sequencer, event, play_state.current_time);
    return closest_midi;
  }

  [[nodiscard]] auto
  get_program_pointer(QWidget &parent, const PlayState &play_state,
                      const int chord_number,
                      const int note_number) const -> const Program * override {
    if (instrument_pointer == nullptr) {
      const auto *current_instrument_pointer =
          play_state.current_instrument_pointer;
      if (current_instrument_pointer == nullptr) {
        QString message;
        QTextStream stream(&message);
        stream << QObject::tr("No instrument");
        add_note_location<PitchedNote>(stream, chord_number, note_number);
        QMessageBox::warning(&parent, QObject::tr("Instrument error"), message);
      }
      return current_instrument_pointer;
    }
    return instrument_pointer;
  }

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override {
    switch (column_number) {
    case pitched_note_instrument_column:
      return QVariant::fromValue(instrument_pointer);
    case pitched_note_interval_column:
      return QVariant::fromValue(interval);
    case pitched_note_beats_column:
      return QVariant::fromValue(beats);
    case pitched_note_velocity_ratio_column:
      return QVariant::fromValue(velocity_ratio);
    case pitched_note_words_column:
      return words;
    default:
      Q_ASSERT(false);
      return {};
    }
  }

  void set_data(const int column_number, const QVariant &new_value) override {
    switch (column_number) {
    case pitched_note_instrument_column:
      instrument_pointer = variant_to<const Program *>(new_value);
      break;
    case pitched_note_interval_column:
      interval = variant_to<Interval>(new_value);
      break;
    case pitched_note_beats_column:
      beats = variant_to<Rational>(new_value);
      break;
    case pitched_note_velocity_ratio_column:
      velocity_ratio = variant_to<Rational>(new_value);
      break;
    case pitched_note_words_column:
      words = variant_to<QString>(new_value);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void copy_column_from(const PitchedNote &template_row,
                        const int column_number) {
    switch (column_number) {
    case pitched_note_instrument_column:
      instrument_pointer = template_row.instrument_pointer;
      break;
    case pitched_note_interval_column:
      interval = template_row.interval;
      break;
    case pitched_note_beats_column:
      beats = template_row.beats;
      break;
    case pitched_note_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    case pitched_note_words_column:
      words = template_row.words;
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void column_to_xml(xmlNode &node, const int column_number) const override {
    switch (column_number) {
    case pitched_note_instrument_column:
      maybe_add_program_to_xml(node, "instrument", instrument_pointer);
      break;
    case pitched_note_interval_column:
      maybe_add_interval_to_xml(node, "interval", interval);
      break;
    case pitched_note_beats_column:
      maybe_add_rational_to_xml(node, "beats", beats);
      break;
    case pitched_note_velocity_ratio_column:
      maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
      break;
    case pitched_note_words_column:
      maybe_set_xml_qstring(node, "words", words);
      break;
    default:
      Q_ASSERT(false);
      break;
    }
  }

  void to_xml(xmlNode &node) const override {
    maybe_add_program_to_xml(node, "instrument", instrument_pointer);
    maybe_add_interval_to_xml(node, "interval", interval);
    maybe_add_rational_to_xml(node, "beats", beats);
    maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
    maybe_set_xml_qstring(node, "words", words);
  }
};
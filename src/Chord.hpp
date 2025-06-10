#pragma once

#include "PitchedNote.hpp"
#include "Row.hpp"
#include "UnpitchedNote.hpp"

struct Chord : public Row {
  const Program *instrument_pointer = nullptr;
  PercussionInstrument percussion_instrument;
  Interval interval;
  Rational tempo_ratio;
  QList<PitchedNote> pitched_notes;
  QList<UnpitchedNote> unpitched_notes;

  void from_xml(xmlNode &node) override {
    auto *field_pointer = xmlFirstElementChild(&node);
    while ((field_pointer != nullptr)) {
      auto &field_node = get_reference(field_pointer);
      const auto name = get_xml_name(field_node);
      if (name == "beats") {
        maybe_xml_to_rational(beats, field_node);
      } else if (name == "velocity_ratio") {
        maybe_xml_to_rational(velocity_ratio, field_node);
      } else if (name == "tempo_ratio") {
        maybe_xml_to_rational(tempo_ratio, field_node);
      } else if (name == "words") {
        words = get_qstring_content(field_node);
      } else if (name == "percussion_instrument") {
        set_percussion_instrument_from_xml(percussion_instrument, field_node);
      } else if (name == "interval") {
        xml_to_interval(interval, field_node);
      } else if (name == "instrument") {
        instrument_pointer =
            &xml_to_program(get_pitched_instruments(), field_node);
      } else if (name == "pitched_notes") {
        xml_to_rows(pitched_notes, field_node);
      } else if (name == "unpitched_notes") {
        xml_to_rows(unpitched_notes, field_node);
      } else {
        Q_ASSERT(false);
      }
      field_pointer = xmlNextElementSibling(field_pointer);
    }
  }

  [[nodiscard]] static auto get_clipboard_schema() -> const char * {
    return "chords_clipboard.xsd";
  };

  [[nodiscard]] static auto get_xml_field_name() -> const char * {
    return "chord";
  };

  [[nodiscard]] static auto get_number_of_columns() -> int {
    return number_of_chord_columns;
  };

  [[nodiscard]] static auto get_column_name(int column_number) {
    switch (column_number) {
    case chord_instrument_column:
      return "Instrument";
    case chord_percussion_instrument_column:
      return "Percussion instrument";
    case chord_interval_column:
      return "Interval";
    case chord_beats_column:
      return "Beats";
    case chord_velocity_ratio_column:
      return "Velocity ratio";
    case chord_tempo_ratio_column:
      return "Tempo ratio";
    case chord_words_column:
      return "Words";
    case chord_pitched_notes_column:
      return "Pitched notes";
    case chord_unpitched_notes_column:
      return "Unpitched notes";
    default:
      Q_ASSERT(false);
      return "";
    };
  }

  [[nodiscard]] static auto get_cells_mime() {
    return "application/prs.chords_cells+xml";
  }

  [[nodiscard]] static auto is_column_editable(int column_number) -> bool {
    return column_number != chord_pitched_notes_column &&
           column_number != chord_unpitched_notes_column;
  }

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override {
    switch (column_number) {
    case chord_instrument_column:
      return QVariant::fromValue(instrument_pointer);
    case chord_percussion_instrument_column:
      return QVariant::fromValue(percussion_instrument);
    case chord_interval_column:
      return QVariant::fromValue(interval);
    case chord_beats_column:
      return QVariant::fromValue(beats);
    case chord_velocity_ratio_column:
      return QVariant::fromValue(velocity_ratio);
    case chord_tempo_ratio_column:
      return QVariant::fromValue(tempo_ratio);
    case chord_words_column:
      return words;
    case chord_pitched_notes_column:
      return pitched_notes.size();
    case chord_unpitched_notes_column:
      return unpitched_notes.size();
    default:
      Q_ASSERT(false);
      return {};
    }
  }

  void set_data(const int column_number, const QVariant &new_value) override {
    switch (column_number) {
    case chord_instrument_column:
      instrument_pointer = variant_to<const Program *>(new_value);
      break;
    case chord_percussion_instrument_column:
      percussion_instrument = variant_to<PercussionInstrument>(new_value);
      break;
    case chord_interval_column:
      interval = variant_to<Interval>(new_value);
      break;
    case chord_beats_column:
      beats = variant_to<Rational>(new_value);
      break;
    case chord_velocity_ratio_column:
      velocity_ratio = variant_to<Rational>(new_value);
      break;
    case chord_tempo_ratio_column:
      tempo_ratio = variant_to<Rational>(new_value);
      break;
    case chord_words_column:
      words = variant_to<QString>(new_value);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void copy_column_from(const Chord &template_row, const int column_number) {
    switch (column_number) {
    case chord_instrument_column:
      instrument_pointer = template_row.instrument_pointer;
      break;
    case chord_percussion_instrument_column:
      percussion_instrument = template_row.percussion_instrument;
      break;
    case chord_interval_column:
      interval = template_row.interval;
      break;
    case chord_beats_column:
      beats = template_row.beats;
      break;
    case chord_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    case chord_tempo_ratio_column:
      tempo_ratio = template_row.tempo_ratio;
      break;
    case chord_words_column:
      words = template_row.words;
      break;
    case chord_pitched_notes_column:
      pitched_notes = template_row.pitched_notes;
      break;
    case chord_unpitched_notes_column:
      unpitched_notes = template_row.unpitched_notes;
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void column_to_xml(xmlNode &chord_node,
                     const int column_number) const override {
    switch (column_number) {
    case chord_pitched_notes_column:
      maybe_set_xml_rows(chord_node, "pitched_notes", pitched_notes);
      break;
    case chord_unpitched_notes_column:
      maybe_set_xml_rows(chord_node, "unpitched_notes", unpitched_notes);
      break;
    case chord_instrument_column:
      maybe_set_xml_program(chord_node, "instrument", instrument_pointer);
      break;
    case chord_percussion_instrument_column:
      maybe_set_xml_percussion_instrument(chord_node, "percussion_instrument",
                                          percussion_instrument);
      break;
    case chord_interval_column:
      maybe_set_xml_interval(chord_node, "interval", interval);
      break;
    case chord_beats_column:
      maybe_set_xml_rational(chord_node, "beats", beats);
      break;
    case chord_velocity_ratio_column:
      maybe_set_xml_rational(chord_node, "velocity_ratio", velocity_ratio);
      break;
    case chord_tempo_ratio_column:
      maybe_set_xml_rational(chord_node, "tempo_ratio", tempo_ratio);
      break;
    case chord_words_column:
      maybe_set_xml_qstring(chord_node, "words", words);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void to_xml(xmlNode &chord_node) const override {
    maybe_set_xml_rows(chord_node, "pitched_notes", pitched_notes);
    maybe_set_xml_rows(chord_node, "unpitched_notes", unpitched_notes);
    maybe_set_xml_program(chord_node, "instrument", instrument_pointer);
    maybe_set_xml_percussion_instrument(chord_node, "percussion_instrument",
                                        percussion_instrument);
    maybe_set_xml_interval(chord_node, "interval", interval);
    maybe_set_xml_rational(chord_node, "beats", beats);
    maybe_set_xml_rational(chord_node, "velocity_ratio", velocity_ratio);
    maybe_set_xml_rational(chord_node, "tempo_ratio", tempo_ratio);
    maybe_set_xml_qstring(chord_node, "words", words);
  }
};

static inline void modulate(PlayState &play_state, const Chord &chord) {
  play_state.current_key =
      play_state.current_key * interval_to_double(chord.interval);
  play_state.current_velocity =
      play_state.current_velocity * rational_to_double(chord.velocity_ratio);
  play_state.current_tempo =
      play_state.current_tempo * rational_to_double(chord.tempo_ratio);
  const auto *chord_instrument_pointer = chord.instrument_pointer;
  if (chord_instrument_pointer != nullptr) {
    play_state.current_instrument_pointer = chord_instrument_pointer;
  }

  const auto &chord_percussion_instrument = chord.percussion_instrument;
  if (chord_percussion_instrument.percussion_set_pointer != nullptr) {
    play_state.current_percussion_instrument = chord_percussion_instrument;
  }
}

static inline void move_time(PlayState &play_state, const Chord &chord) {
  play_state.current_time =
      play_state.current_time +
      get_milliseconds(play_state.current_tempo, chord.beats);
}

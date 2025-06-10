#ifdef TEST_HOOKS

#include <QtWidgets/QPushButton>

#include "IntervalRow.hpp" 
#include "PitchedNotesTable.hpp" // IWYU pragma: keep
#include "SongWidget.hpp"
#include "UnpitchedNotesTable.hpp" // IWYU pragma: keep
#include "justly/justly.hpp"

class QAbstractItemView;

auto get_gain(const SongWidget &song_widget) -> double {
  return get_gain_internal(song_widget);
}

void export_to_file(SongWidget &song_widget,
                                   const QString &output_file) {
  export_to_file_internal(song_widget, output_file);

}

void save_as_file(SongWidget &song_widget,
                                 const QString &filename) {
  save_as_file_internal(song_widget, filename);
}

void open_file(SongWidget &song_widget,
                              const QString &filename) {
  open_file_internal(song_widget, filename);

}

void import_musicxml(SongWidget &song_widget,
                                    const QString &filename) {
  import_musicxml_internal(song_widget, filename);
}

auto get_chords_table(const SongWidget &song_widget) -> QAbstractItemView & {
  return song_widget.switch_column.chords_table;
}

auto get_pitched_notes_table(const SongWidget &song_widget)
    -> QAbstractItemView & {
  return song_widget.switch_column.pitched_notes_table;
}

auto get_unpitched_notes_table(const SongWidget &song_widget)
    -> QAbstractItemView & {
  return song_widget.switch_column.unpitched_notes_table;
}

auto get_starting_key(const SongWidget &song_widget) -> double {
  return song_widget.song.starting_key;
}

auto get_starting_velocity(const SongWidget &song_widget) -> double {
  return song_widget.song.starting_velocity;
}

auto get_starting_tempo(const SongWidget &song_widget) -> double {
  return song_widget.song.starting_tempo;
}

auto get_current_file(const SongWidget &song_widget) -> QString {
  return song_widget.current_file;
}

auto get_current_chord_number(const SongWidget &song_widget) -> int {
  return get_parent_chord_number(song_widget.switch_column);
}

void set_gain(const SongWidget &song_widget, double new_value) {
  song_widget.controls_column.spin_boxes.gain_editor.setValue(new_value);
}

void set_starting_key(const SongWidget &song_widget, double new_value) {
  song_widget.controls_column.spin_boxes.starting_key_editor.setValue(
      new_value);
}

void set_starting_velocity(const SongWidget &song_widget, double new_value) {
  song_widget.controls_column.spin_boxes.starting_velocity_editor.setValue(
      new_value);
}

void set_starting_tempo(const SongWidget &song_widget, double new_value) {
  song_widget.controls_column.spin_boxes.starting_tempo_editor.setValue(
      new_value);
}

void undo(SongWidget &song_widget) { song_widget.undo_stack.undo(); }

void trigger_third_down(SongWidget &song_widget) {
  song_widget.controls_column.third_row.minus_button.click();
};

void trigger_third_up(SongWidget &song_widget) {
  song_widget.controls_column.third_row.plus_button.click();
};

void trigger_fifth_down(SongWidget &song_widget) {
  song_widget.controls_column.fifth_row.minus_button.click();
};

void trigger_fifth_up(SongWidget &song_widget) {
  song_widget.controls_column.fifth_row.plus_button.click();
};

void trigger_seventh_down(SongWidget &song_widget) {
  song_widget.controls_column.seventh_row.minus_button.click();
};

void trigger_seventh_up(SongWidget &song_widget) {
  song_widget.controls_column.seventh_row.plus_button.click();
};

void trigger_octave_down(SongWidget &song_widget) {
  song_widget.controls_column.octave_row.minus_button.click();
};

void trigger_octave_up(SongWidget &song_widget) {
  song_widget.controls_column.octave_row.plus_button.click();
};

#endif
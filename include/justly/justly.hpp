#pragma once

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QtGlobal>
#include <QtWidgets/QMainWindow>

class QAbstractItemModel;
class QModelIndex;
class QAbstractItemView;
class QWidget;
struct SongWidget;
struct SongMenuBar;

#ifdef JUSTLY_LIBRARY
#define JUSTLY_EXPORT Q_DECL_EXPORT
#else
#define JUSTLY_EXPORT Q_DECL_IMPORT
#endif

#ifdef TEST_HOOKS
#define TEST_EXPORT JUSTLY_EXPORT
#else
#define TEST_EXPORT
#endif

enum TEST_EXPORT ChordColumn {
  chord_pitched_notes_column,
  chord_unpitched_notes_column,
  chord_instrument_column,
  chord_percussion_instrument_column,
  chord_interval_column,
  chord_beats_column,
  chord_velocity_ratio_column,
  chord_tempo_ratio_column,
  chord_words_column,
  number_of_chord_columns
};

enum TEST_EXPORT PitchedNoteColumn {
  pitched_note_instrument_column,
  pitched_note_interval_column,
  pitched_note_beats_column,
  pitched_note_velocity_ratio_column,
  pitched_note_words_column,
  number_of_pitched_note_columns
};

enum TEST_EXPORT UnpitchedNoteColumn {
  unpitched_note_percussion_instrument_column,
  unpitched_note_beats_column,
  unpitched_note_velocity_ratio_column,
  unpitched_note_words_column,
  number_of_unpitched_note_columns
};

struct JUSTLY_EXPORT SongEditor : public QMainWindow {
public:
  SongWidget &song_widget;
  SongMenuBar &song_menu_bar;

  explicit SongEditor();
  void closeEvent(QCloseEvent *close_event_pointer) override;
};

void JUSTLY_EXPORT set_up();

#ifdef TEST_HOOKS

[[nodiscard]] auto JUSTLY_EXPORT get_chords_table(const SongWidget &song_widget)
    -> QAbstractItemView &;
[[nodiscard]] auto JUSTLY_EXPORT
get_pitched_notes_table(const SongWidget &song_widget) -> QAbstractItemView &;
[[nodiscard]] auto JUSTLY_EXPORT
get_unpitched_notes_table(const SongWidget &song_widget) -> QAbstractItemView &;

[[nodiscard]] auto JUSTLY_EXPORT get_starting_key(const SongWidget &song_widget)
    -> double;
[[nodiscard]] auto JUSTLY_EXPORT
get_starting_velocity(const SongWidget &song_widget) -> double;
[[nodiscard]] auto JUSTLY_EXPORT
get_starting_tempo(const SongWidget &song_widget) -> double;

[[nodiscard]] auto JUSTLY_EXPORT get_current_file(const SongWidget &song_widget)
    -> QString;
[[nodiscard]] auto JUSTLY_EXPORT
get_current_chord_number(const SongWidget &song_widget) -> int;

void JUSTLY_EXPORT set_gain(const SongWidget &song_widget, double new_value);
void JUSTLY_EXPORT set_starting_key(const SongWidget &song_widget,
                                    double new_value);
void JUSTLY_EXPORT set_starting_velocity(const SongWidget &song_widget,
                                         double new_value);
void JUSTLY_EXPORT set_starting_tempo(const SongWidget &song_widget,
                                      double new_value);

void JUSTLY_EXPORT undo(SongWidget &song_widget);

void JUSTLY_EXPORT trigger_third_down(SongWidget &song_widget);
void JUSTLY_EXPORT trigger_third_up(SongWidget &song_widget);
void JUSTLY_EXPORT trigger_fifth_down(SongWidget &song_widget);
void JUSTLY_EXPORT trigger_fifth_up(SongWidget &song_widget);
void JUSTLY_EXPORT trigger_seventh_down(SongWidget &song_widget);
void JUSTLY_EXPORT trigger_seventh_up(SongWidget &song_widget);
void JUSTLY_EXPORT trigger_octave_down(SongWidget &song_widget);
void JUSTLY_EXPORT trigger_octave_up(SongWidget &song_widget);

void JUSTLY_EXPORT trigger_back_to_chords(SongMenuBar &song_menu_bar);
void JUSTLY_EXPORT trigger_previous_chord(SongMenuBar &song_menu_bar);
void JUSTLY_EXPORT trigger_next_chord(SongMenuBar &song_menu_bar);

void JUSTLY_EXPORT trigger_insert_after(SongMenuBar &song_menu_bar);
void JUSTLY_EXPORT trigger_insert_into(SongMenuBar &song_menu_bar);
void JUSTLY_EXPORT trigger_delete_cells(SongMenuBar &song_menu_ba);
void JUSTLY_EXPORT trigger_remove_rows(SongMenuBar &song_menu_bar);

void JUSTLY_EXPORT trigger_cut(SongMenuBar &song_menu_bar);
void JUSTLY_EXPORT trigger_copy(SongMenuBar &song_menu_bar);
void JUSTLY_EXPORT trigger_paste_over(SongMenuBar &song_menu_bar);
void JUSTLY_EXPORT trigger_paste_after(SongMenuBar &song_menu_bar);
void JUSTLY_EXPORT trigger_paste_into(SongMenuBar &song_menu_bar);

void JUSTLY_EXPORT trigger_save(SongMenuBar &song_menu_bar);

void JUSTLY_EXPORT trigger_play(SongMenuBar &song_menu_bar);
void JUSTLY_EXPORT trigger_stop_playing(SongMenuBar &song_menu_bar);

[[nodiscard]] auto JUSTLY_EXPORT get_gain(const SongWidget &song_widget)
    -> double;
void JUSTLY_EXPORT open_file(SongWidget &song_widget, const QString &filename);
void JUSTLY_EXPORT import_musicxml(SongWidget &song_widget,
                                   const QString &filename);
void JUSTLY_EXPORT save_as_file(SongWidget &song_widget,
                                const QString &filename);
void JUSTLY_EXPORT export_to_file(SongWidget &song_widget,
                                  const QString &output_file);

#endif
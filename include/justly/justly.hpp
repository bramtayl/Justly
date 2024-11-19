#pragma once

#include <QString>
#include <QVariant>
#include <QtCore/QtGlobal>

struct SongEditor;
class QAbstractItemModel;
class QModelIndex;
class QAbstractItemView;
class QWidget;


#if defined(JUSTLY_LIBRARY)
#define JUSTLY_EXPORT Q_DECL_EXPORT
#else
#define JUSTLY_EXPORT Q_DECL_IMPORT
#endif

enum JUSTLY_EXPORT ChordColumn {
  chord_instrument_column,
  chord_percussion_set_column,
  chord_percussion_instrument_column,
  chord_interval_column,
  chord_beats_column,
  chord_velocity_ratio_column,
  chord_tempo_ratio_column,
  chord_words_column,
  chord_pitched_notes_column,
  chord_unpitched_notes_column,
  number_of_chord_columns
};

enum JUSTLY_EXPORT PitchedNoteColumn {
  pitched_note_instrument_column,
  pitched_note_interval_column,
  pitched_note_beats_column,
  pitched_note_velocity_ratio_column,
  pitched_note_words_column,
  number_of_pitched_note_columns
};

enum JUSTLY_EXPORT UnpitchedNoteColumn {
  unpitched_note_percussion_set_column,
  unpitched_note_percussion_instrument_column,
  unpitched_note_beats_column,
  unpitched_note_velocity_ratio_column,
  unpitched_note_words_column,
  number_of_unpitched_note_columns
};

void JUSTLY_EXPORT set_up();
[[nodiscard]] auto JUSTLY_EXPORT make_song_editor() -> SongEditor *;
void JUSTLY_EXPORT show_song_editor(SongEditor *song_editor_pointer);
void JUSTLY_EXPORT delete_song_editor(SongEditor *song_editor_pointer);

[[nodiscard]] auto JUSTLY_EXPORT get_table_view(
    const SongEditor *song_editor_pointer) -> QAbstractItemView &;

[[nodiscard]] auto JUSTLY_EXPORT get_chords_model(
    SongEditor *song_editor_pointer) -> QAbstractItemModel &;
[[nodiscard]] auto JUSTLY_EXPORT get_pitched_notes_model(
    SongEditor *song_editor_pointer) -> QAbstractItemModel &;
[[nodiscard]] auto JUSTLY_EXPORT get_unpitched_notes_model(
    SongEditor *song_editor_pointer) -> QAbstractItemModel &;

void JUSTLY_EXPORT trigger_edit_pitched_notes(SongEditor *song_editor_pointer,
                                              int chord_number);
void JUSTLY_EXPORT trigger_edit_unpitched_notes(SongEditor *song_editor_pointer,
                                                int chord_number);
void JUSTLY_EXPORT
trigger_back_to_chords(SongEditor *song_editor_pointer);

[[nodiscard]] auto JUSTLY_EXPORT get_gain(const SongEditor *song_editor_pointer)
    -> double;
[[nodiscard]] auto JUSTLY_EXPORT
get_starting_key(const SongEditor *song_editor_pointer) -> double;
[[nodiscard]] auto JUSTLY_EXPORT
get_starting_velocity(const SongEditor *song_editor_pointer) -> double;
[[nodiscard]] auto JUSTLY_EXPORT
get_starting_tempo(const SongEditor *song_editor_pointer) -> double;

[[nodiscard]] auto JUSTLY_EXPORT
get_current_file(const SongEditor *song_editor_pointer) -> QString;

void JUSTLY_EXPORT set_gain(const SongEditor *song_editor_pointer,
                            double new_value);
void JUSTLY_EXPORT set_starting_key(const SongEditor *song_editor_pointer,
                                    double new_value);
void JUSTLY_EXPORT set_starting_velocity(const SongEditor *song_editor_pointer,
                                         double new_value);
void JUSTLY_EXPORT set_starting_tempo(const SongEditor *song_editor_pointer,
                                      double new_value);

[[nodiscard]] auto JUSTLY_EXPORT
create_editor(const QAbstractItemView& table_view,
              QModelIndex index) -> QWidget &;
void JUSTLY_EXPORT set_editor(const QAbstractItemView& table_view,
                              QWidget& cell_editor_pointer, QModelIndex index,
                              const QVariant &new_value);

void JUSTLY_EXPORT undo(const SongEditor *song_editor_pointer);

void JUSTLY_EXPORT trigger_insert_after(SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_insert_into(SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_delete_cells(SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_remove_rows(SongEditor *song_editor_pointer);

void JUSTLY_EXPORT trigger_cut(SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_copy(SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_paste_over(SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_paste_after(SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_paste_into(SongEditor *song_editor_pointer);

void JUSTLY_EXPORT trigger_save(SongEditor *song_editor_pointer);

void JUSTLY_EXPORT trigger_play(SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_stop_playing(SongEditor *song_editor_pointer);

void JUSTLY_EXPORT open_file(SongEditor *song_editor_pointer,
                             const QString &filename);
void JUSTLY_EXPORT save_as_file(SongEditor *song_editor_pointer,
                                const QString &filename);
void JUSTLY_EXPORT export_to_file(SongEditor *song_editor_pointer,
                                  const QString &output_file);
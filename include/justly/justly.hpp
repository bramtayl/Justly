#pragma once

#include "justly/JUSTLY_EXPORT.hpp"
#include <QString>
#include <QVariant>

struct SongEditor;
class QAbstractItemModel;
class QModelIndex;
class QAbstractItemView;
class QWidget;

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
void JUSTLY_EXPORT trigger_delete(SongEditor *song_editor_pointer);
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
#pragma once

#include "justly/JUSTLY_EXPORT.hpp"
#include <QString>
#include <QVariant>

struct SongEditor;
class QAbstractItemModel;
class QModelIndex;
class QAbstractItemView;
class QWidget;

void JUSTLY_EXPORT register_converters();
[[nodiscard]] auto JUSTLY_EXPORT make_song_editor() -> SongEditor *;
void JUSTLY_EXPORT show_song_editor(SongEditor *song_editor_pointer);
void JUSTLY_EXPORT delete_song_editor(SongEditor *song_editor_pointer);

[[nodiscard]] auto JUSTLY_EXPORT
get_table_view_pointer(const SongEditor *song_editor_pointer) -> QAbstractItemView *;

[[nodiscard]] auto JUSTLY_EXPORT
get_chords_model_pointer(const SongEditor *song_editor_pointer) -> QAbstractItemModel *;
[[nodiscard]] auto JUSTLY_EXPORT
get_notes_model_pointer(const SongEditor *song_editor_pointer) -> QAbstractItemModel *;
[[nodiscard]] auto JUSTLY_EXPORT
get_percussions_model_pointer(const SongEditor *song_editor_pointer) -> QAbstractItemModel *;

void JUSTLY_EXPORT trigger_edit_notes(SongEditor *song_editor_pointer, int chord_number);
void JUSTLY_EXPORT trigger_edit_percussions(SongEditor *song_editor_pointer, int chord_number);
void JUSTLY_EXPORT trigger_back_to_chords(const SongEditor *song_editor_pointer);

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

[[nodiscard]] auto JUSTLY_EXPORT create_editor(
    const QAbstractItemView *table_view_pointer,QModelIndex index) -> QWidget *;
void JUSTLY_EXPORT set_editor(const QAbstractItemView *table_view_pointer,
                              QWidget *cell_editor_pointer, QModelIndex index,
                              const QVariant &new_value);

void JUSTLY_EXPORT undo(const SongEditor *song_editor_pointer);

void JUSTLY_EXPORT trigger_insert_after(const SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_insert_into(const SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_delete(const SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_remove_rows(const SongEditor *song_editor_pointer);

void JUSTLY_EXPORT trigger_cut(const SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_copy(const SongEditor *song_editor_pointer);
void JUSTLY_EXPORT
trigger_paste_over(const SongEditor *song_editor_pointer);
void JUSTLY_EXPORT
trigger_paste_after(const SongEditor *song_editor_pointer);
void JUSTLY_EXPORT
trigger_paste_into(const SongEditor *song_editor_pointer);

void JUSTLY_EXPORT trigger_save(const SongEditor *song_editor_pointer);

void JUSTLY_EXPORT trigger_play(const SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_stop_playing(const SongEditor *song_editor_pointer);

void JUSTLY_EXPORT open_file(SongEditor *song_editor_pointer,
                             const QString &filename);
void JUSTLY_EXPORT save_as_file(SongEditor *song_editor_pointer,
                                const QString &filename);
void JUSTLY_EXPORT export_to_file(SongEditor *song_editor_pointer,
                                  const QString &output_file);
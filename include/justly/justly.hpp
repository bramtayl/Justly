#pragma once

#include <cstddef>

#include <QString>
#include <QVariant>

#include "justly/JUSTLY_EXPORT.hpp"
#include "justly/NoteChordColumn.hpp"

class SongEditor;
class QModelIndex;
class QTreeView;
class QWidget;

void JUSTLY_EXPORT register_converters();
[[nodiscard]] auto JUSTLY_EXPORT make_song_editor() -> SongEditor*;
void JUSTLY_EXPORT show_song_editor(SongEditor *song_editor_pointer);
void JUSTLY_EXPORT delete_song_editor(SongEditor *song_editor_pointer);

[[nodiscard]] auto
JUSTLY_EXPORT get_chord_index(const SongEditor *song_editor_pointer, size_t chord_number,
                NoteChordColumn note_chord_column = type_column) -> QModelIndex;
[[nodiscard]] auto
JUSTLY_EXPORT get_note_index(const SongEditor *song_editor_pointer, size_t chord_number,
               size_t note_number,
               NoteChordColumn note_chord_column = type_column) -> QModelIndex;

[[nodiscard]] auto
JUSTLY_EXPORT get_chords_view_pointer(const SongEditor *song_editor_pointer) -> QTreeView *;

[[nodiscard]] auto JUSTLY_EXPORT get_gain(const SongEditor *song_editor_pointer) -> double;
[[nodiscard]] auto
JUSTLY_EXPORT get_starting_key(const SongEditor *song_editor_pointer) -> double;
[[nodiscard]] auto
JUSTLY_EXPORT get_starting_velocity(const SongEditor *song_editor_pointer) -> double;
[[nodiscard]] auto
JUSTLY_EXPORT get_starting_tempo(const SongEditor *song_editor_pointer) -> double;

[[nodiscard]] auto
JUSTLY_EXPORT get_current_file(const SongEditor *song_editor_pointer) -> QString;

void JUSTLY_EXPORT set_gain(const SongEditor *song_editor_pointer, double new_value);
void JUSTLY_EXPORT set_starting_key(const SongEditor *song_editor_pointer, double new_value);
void JUSTLY_EXPORT set_starting_velocity(const SongEditor *song_editor_pointer,
                           double new_value);
void JUSTLY_EXPORT set_starting_tempo(const SongEditor *song_editor_pointer,
                        double new_value);

[[nodiscard]] auto JUSTLY_EXPORT create_editor(const SongEditor *song_editor_pointer,
                                 QModelIndex index) -> QWidget *;
void JUSTLY_EXPORT set_editor(const SongEditor *song_editor_pointer,
                QWidget *cell_editor_pointer, QModelIndex index,
                const QVariant &new_value);

void JUSTLY_EXPORT undo(const SongEditor *song_editor_pointer);
void JUSTLY_EXPORT redo(const SongEditor *song_editor_pointer);

void JUSTLY_EXPORT trigger_insert_after(const SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_insert_into(const SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_delete(const SongEditor *song_editor_pointer);

void JUSTLY_EXPORT trigger_cut(const SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_copy(const SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_paste_cells_or_after(const SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_paste_into(const SongEditor *song_editor_pointer);

void JUSTLY_EXPORT trigger_save(const SongEditor *song_editor_pointer);

void JUSTLY_EXPORT trigger_play(const SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_stop_playing(const SongEditor *song_editor_pointer);

void JUSTLY_EXPORT trigger_expand(const SongEditor *song_editor_pointer);
void JUSTLY_EXPORT trigger_collapse(const SongEditor *song_editor_pointer);

void JUSTLY_EXPORT open_file(SongEditor *song_editor_pointer, const QString &filename);
void JUSTLY_EXPORT save_as_file(SongEditor *song_editor_pointer, const QString &filename);
void JUSTLY_EXPORT export_to_file(SongEditor *song_editor_pointer,
                    const QString &output_file);
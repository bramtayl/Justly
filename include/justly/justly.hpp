#pragma once

#include <cstddef>

// TODO: remove QFileDialog
#include <QFileDialog>
#include <QString>
#include <QVariant>
#include <string>

#include "justly/NoteChordColumn.hpp"

class SongEditor;
class QModelIndex;
class QTreeView;
class QWidget;

void register_converters();
[[nodiscard]] auto make_song_editor() -> SongEditor*;
void show_song_editor(SongEditor *song_editor_pointer);
void delete_song_editor(SongEditor *song_editor_pointer);

[[nodiscard]] auto
get_chord_index(const SongEditor *song_editor_pointer, size_t chord_number,
                NoteChordColumn note_chord_column = type_column) -> QModelIndex;
[[nodiscard]] auto
get_note_index(const SongEditor *song_editor_pointer, size_t chord_number,
               size_t note_number,
               NoteChordColumn note_chord_column = type_column) -> QModelIndex;

[[nodiscard]] auto
get_chords_view_pointer(const SongEditor *song_editor_pointer) -> QTreeView *;

[[nodiscard]] auto get_gain(const SongEditor *song_editor_pointer) -> double;
[[nodiscard]] auto get_starting_instrument_name(
    const SongEditor *song_editor_pointer) -> std::string;
[[nodiscard]] auto
get_starting_key(const SongEditor *song_editor_pointer) -> double;
[[nodiscard]] auto
get_starting_velocity(const SongEditor *song_editor_pointer) -> double;
[[nodiscard]] auto
get_starting_tempo(const SongEditor *song_editor_pointer) -> double;

[[nodiscard]] auto
get_current_file(const SongEditor *song_editor_pointer) -> QString;

void set_gain(const SongEditor *song_editor_pointer, double new_value);
void set_starting_instrument_name(const SongEditor *song_editor_pointer,
                                  const std::string &new_name);
void set_starting_key(const SongEditor *song_editor_pointer, double new_value);
void set_starting_velocity(const SongEditor *song_editor_pointer,
                           double new_value);
void set_starting_tempo(const SongEditor *song_editor_pointer,
                        double new_value);

[[nodiscard]] auto create_editor(const SongEditor *song_editor_pointer,
                                 QModelIndex index) -> QWidget *;
void set_editor(const SongEditor *song_editor_pointer,
                QWidget *cell_editor_pointer, QModelIndex index,
                const QVariant &new_value);

void undo(const SongEditor *song_editor_pointer);
void redo(const SongEditor *song_editor_pointer);

void trigger_insert_after(const SongEditor *song_editor_pointer);
void trigger_insert_into(const SongEditor *song_editor_pointer);
void trigger_delete(const SongEditor *song_editor_pointer);

void trigger_cut(const SongEditor *song_editor_pointer);
void trigger_copy(const SongEditor *song_editor_pointer);
void trigger_paste_cells_or_after(const SongEditor *song_editor_pointer);
void trigger_paste_into(const SongEditor *song_editor_pointer);

void trigger_save(const SongEditor *song_editor_pointer);

void trigger_play(const SongEditor *song_editor_pointer);
void trigger_stop_playing(const SongEditor *song_editor_pointer);

void trigger_expand(const SongEditor *song_editor_pointer);
void trigger_collapse(const SongEditor *song_editor_pointer);

[[nodiscard]] auto
make_file_dialog(SongEditor *song_editor_pointer, const QString &caption,
                 const QString &filter, QFileDialog::AcceptMode accept_mode,
                 const QString &suffix,
                 QFileDialog::FileMode file_mode) -> QFileDialog *;

void open_file(SongEditor *song_editor_pointer, const QString &filename);
void save_as_file(SongEditor *song_editor_pointer, const QString &filename);
void export_to_file(SongEditor *song_editor_pointer,
                    const QString &output_file);
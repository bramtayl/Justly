#pragma once

#include "models/UndoRowsModel.hpp"
#include "other/Song.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/UnpitchedNote.hpp"

template <NoteInterface SubNote>
struct NotesModel : public UndoRowsModel<SubNote> {
  explicit NotesModel(QUndoStack& undo_stack, Song& song)
      : UndoRowsModel<SubNote>(undo_stack, song) {}

  [[nodiscard]] auto make_empty_row() const -> SubNote override {
    const auto& song = this->song;
    SubNote sub_note;
    if constexpr (std::same_as<SubNote, PitchedNote>) {
      sub_note.voice_name = song.pitched_voices.at(0).name;
    } else {
      sub_note.voice_name = song.unpitched_voices.at(0).name;
    }
    return sub_note;
  }

  void add_to_status(QTextStream& stream, const int /*row_number*/,
                     const SubNote& sub_note) const override {
    const auto& song = this->song;
    auto play_state = get_play_state_at_chord(song, this->parent_chord_number);
    if constexpr (std::same_as<SubNote, PitchedNote>) {
      add_frequency_to_stream(
          stream,
          play_state.current_key * interval_to_double(sub_note.interval));
    }
    add_timing_to_stream(
        stream, play_state,
        sub_note.get_velocity(play_state.current_velocity,
                              song.pitched_voices, song.unpitched_voices),
        rational_to_double(sub_note.beats));
  }
};

using PitchedNotesModel = NotesModel<PitchedNote>;
using UnpitchedNotesModel = NotesModel<UnpitchedNote>;

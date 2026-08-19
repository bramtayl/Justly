#include "menus/PlayMenu.hpp"

void modulate_before_chord(const Song &song, PlayState &play_state,
                           const int next_chord_number) {
  const auto &chords = song.chords;
  if (next_chord_number > 0) {
    std::ranges::for_each(
        chords | std::views::take(next_chord_number),
        [&play_state](const auto &chord) -> void { modulate(play_state, chord); });
  }
}

auto get_play_selection(const SongWidget &song_widget) -> PlaySelection {
  const auto &switch_table = song_widget.switch_column.switch_table;
  const auto &range = get_only_range(switch_table);
  return {.row_type = switch_table.delegate.current_row_type,
          .chord_number = get_parent_chord_number(switch_table),
          .first_row_number = range.top(),
          .number_of_rows = get_number_of_rows(range)};
}

PlayMenu::PlayMenu(SongWidget &song_widget) : QMenu(PlayMenu::tr("&Play")) {
  add_menu_action(*this, play_action, QKeySequence::UnknownKey, false);
  play_action.setShortcut(Qt::Key_Space);
  add_menu_action(*this, play_to_end_action, QKeySequence::UnknownKey, false);
  play_to_end_action.setShortcut(Qt::ShiftModifier | Qt::Key_Space);
  add_menu_action(*this, stop_playing_action, QKeySequence::Cancel);

  const auto &player = song_widget.player;
  QObject::connect(&play_action, &QAction::triggered, this, [&song_widget]() -> auto {
    const auto &song = song_widget.song;
    const auto &pitched_voices = song.pitched_voices;
    const auto &unpitched_voices = song.unpitched_voices;
    auto &player = song_widget.player;
    auto &play_state = player.play_state;

    const auto selection = get_play_selection(song_widget);
    const auto current_row_type = selection.row_type;
    const auto first_row_number = selection.first_row_number;
    const auto number_of_rows = selection.number_of_rows;

    stop_playing(player.sequencer, player.event);
    initialize_play(song_widget);

    switch (current_row_type) {
    case RowType::chord_type:
      modulate_before_chord(song, play_state, first_row_number);
      play_chords(song_widget, first_row_number, number_of_rows);
      break;
    case RowType::pitched_note_type:
    case RowType::unpitched_note_type: {
      const auto chord_number = selection.chord_number;
      modulate_before_chord(song, play_state, chord_number);
      const auto &chord = song.chords.at(chord_number);
      modulate(play_state, chord);
      if (current_row_type == RowType::pitched_note_type) {
        const auto pitched_result =
            play_notes(player, pitched_voices, unpitched_voices, chord_number,
                       chord.pitched_notes, first_row_number, number_of_rows);
        if (!pitched_result) {
          return;
        }
      } else {
        const auto unpitched_result = play_notes(
            player, pitched_voices, unpitched_voices, chord_number,
            chord.unpitched_notes, first_row_number, number_of_rows);
        if (!unpitched_result) {
          return;
        }
      }
      break;
    }
    case RowType::pitched_voice_type:
      if (!play_voices(player, pitched_voices, first_row_number,
                       number_of_rows)) {
        return;
      }
      break;
    case RowType::unpitched_voice_type:
      if (!play_voices(player, unpitched_voices, first_row_number,
                       number_of_rows)) {
        return;
      }
      break;
    }
  });

  QObject::connect(&play_to_end_action, &QAction::triggered, this, [&song_widget]() -> auto {
    const auto &song = song_widget.song;
    const auto &pitched_voices = song.pitched_voices;
    const auto &unpitched_voices = song.unpitched_voices;
    auto &player = song_widget.player;
    auto &play_state = player.play_state;
    const auto number_of_chords = static_cast<int>(song.chords.size());

    const auto selection = get_play_selection(song_widget);
    const auto current_row_type = selection.row_type;
    const auto first_row_number = selection.first_row_number;

    stop_playing(player.sequencer, player.event);
    initialize_play(song_widget);

    switch (current_row_type) {
    case RowType::chord_type:
      modulate_before_chord(song, play_state, first_row_number);
      play_chords(song_widget, first_row_number,
                 number_of_chords - first_row_number);
      break;
    case RowType::pitched_note_type:
    case RowType::unpitched_note_type: {
      const auto chord_number = selection.chord_number;
      modulate_before_chord(song, play_state, chord_number);
      const auto &chord = song.chords.at(chord_number);
      modulate(play_state, chord);
      if (current_row_type == RowType::pitched_note_type) {
        const auto pitched_result = play_notes(
            player, pitched_voices, unpitched_voices, chord_number,
            chord.pitched_notes, first_row_number,
            static_cast<int>(chord.pitched_notes.size()) - first_row_number);
        if (!pitched_result) {
          return;
        }
      } else {
        const auto unpitched_result = play_notes(
            player, pitched_voices, unpitched_voices, chord_number,
            chord.unpitched_notes, first_row_number,
            static_cast<int>(chord.unpitched_notes.size()) - first_row_number);
        if (!unpitched_result) {
          return;
        }
      }
      move_time(play_state, chord);
      update_final_time(player, play_state.current_time);
      play_chords(song_widget, chord_number + 1,
                 number_of_chords - chord_number - 1);
      break;
    }
    case RowType::pitched_voice_type:
    case RowType::unpitched_voice_type:
      // play_to_end_action is disabled for voice rows; see
      // ReplaceTable.hpp's update_actions/get_is_voice
      Q_UNREACHABLE();
    }
  });

  QObject::connect(
      &stop_playing_action, &QAction::triggered, this,
      [&player]() -> auto { stop_playing(player.sequencer, player.event); });
}

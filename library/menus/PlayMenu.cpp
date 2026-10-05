#include "menus/PlayMenu.hpp"

#include "menus/MenuAction.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"
#include "widgets/WindowBody.hpp"

namespace {

void modulate_before_chord(const Song& song, PlayState& play_state,
                           const int next_chord_number) {
  const auto& chords = song.chords;
  if (next_chord_number > 0) {
    std::ranges::for_each(chords | std::views::take(next_chord_number),
                          [&play_state](const auto& chord) -> void {
                            modulate(play_state, chord);
                          });
  }
}

template <NoteInterface SubNote>
[[nodiscard]] auto play_selected_notes(Player& player, const Song& song,
                                       const TableSelection& selection,
                                       const QList<SubNote>& notes,
                                       const bool to_end) -> bool {
  const auto first_row_number = selection.first_row_number;
  return play_notes(player, song.pitched_voices, song.unpitched_voices,
                    selection.chord_number, notes, first_row_number,
                    to_end ? static_cast<int>(notes.size()) - first_row_number
                           : selection.number_of_rows);
}

// plays the selected rows, or with to_end, everything from the start of the
// selection through the end of the song
void play_selection(WindowBody& window_body, const bool to_end) {
  const auto& song = window_body.song;
  auto& player = window_body.player;
  auto& play_state = player.play_state;
  const auto number_of_chords = static_cast<int>(song.chords.size());

  const auto selection = get_play_selection(window_body);
  const auto row_type = selection.row_type;
  const auto first_row_number = selection.first_row_number;
  const auto number_of_rows = selection.number_of_rows;

  stop_playing(player.sequencer, player.event);
  initialize_play(window_body);

  switch (row_type) {
    case RowType::chord_type:
      modulate_before_chord(song, play_state, first_row_number);
      play_chords(
          window_body, first_row_number,
          to_end ? number_of_chords - first_row_number : number_of_rows);
      break;
    case RowType::pitched_note_type:
    case RowType::unpitched_note_type: {
      const auto chord_number = selection.chord_number;
      modulate_before_chord(song, play_state, chord_number);
      const auto& chord = song.chords.at(chord_number);
      modulate(play_state, chord);
      const auto played =
          row_type == RowType::pitched_note_type
              ? play_selected_notes(player, song, selection,
                                    chord.pitched_notes, to_end)
              : play_selected_notes(player, song, selection,
                                    chord.unpitched_notes, to_end);
      if (played && to_end) {
        move_time(play_state, chord);
        update_final_time(player, play_state.current_time);
        play_chords(window_body, chord_number + 1,
                    number_of_chords - chord_number - 1);
      }
      break;
    }
    case RowType::pitched_voice_type:
    case RowType::unpitched_voice_type:
      // play_to_end_action is disabled for voice rows; see
      // ReplaceTable.cpp's update_actions
      Q_ASSERT(!to_end);
      // play_voices has already warned about anything it couldn't play
      static_cast<void>(row_type == RowType::pitched_voice_type
                            ? play_voices(player, song.pitched_voices,
                                          first_row_number, number_of_rows)
                            : play_voices(player, song.unpitched_voices,
                                          first_row_number, number_of_rows));
      break;
  }
}

}  // namespace

auto get_play_selection(const WindowBody& window_body) -> TableSelection {
  const auto& switch_table = window_body.switch_column.switch_table;
  const auto& range = get_only_range(switch_table);
  return {.row_type = switch_table.delegate.current_row_type,
          .chord_number = get_parent_chord_number(switch_table),
          .first_row_number = range.top(),
          .number_of_rows = get_number_of_rows(range)};
}

PlayMenu::PlayMenu(WindowBody& window_body)
    : QMenu(PlayMenu::tr("&Play")),
      play_action(PlayMenu::tr("&Play selection")),
      play_to_end_action(PlayMenu::tr("Play to &end")),
      stop_playing_action(PlayMenu::tr("&Stop playing")) {
  add_menu_action(*this, play_action, QKeySequence::UnknownKey, false);
  play_action.setShortcut(Qt::Key_Space);
  add_menu_action(*this, play_to_end_action, QKeySequence::UnknownKey, false);
  play_to_end_action.setShortcut(Qt::ShiftModifier | Qt::Key_Space);
  add_menu_action(*this, stop_playing_action, QKeySequence::Cancel);

  const auto& player = window_body.player;
  QObject::connect(
      &play_action, &QAction::triggered, this,
      [&window_body]() -> auto { play_selection(window_body, false); });

  QObject::connect(
      &play_to_end_action, &QAction::triggered, this,
      [&window_body]() -> auto { play_selection(window_body, true); });

  QObject::connect(
      &stop_playing_action, &QAction::triggered, this,
      [&player]() -> auto { stop_playing(player.sequencer, player.event); });
}

#pragma once

#include <QtWidgets/QMenu>

#include "rows/RowType.hpp"
#include "widgets/SongWidget.hpp"

static void modulate_before_chord(const Song &song, PlayState &play_state,
                                  const int next_chord_number) {
  const auto &chords = song.chords;
  if (next_chord_number > 0) {
    for (auto chord_number = 0; chord_number < next_chord_number;
         chord_number = chord_number + 1) {
      modulate(play_state, chords.at(chord_number));
    }
  }
}

struct PlayMenu : public QMenu {
  QAction play_action = QAction(PlayMenu::tr("&Play selection"));
  QAction stop_playing_action = QAction(PlayMenu::tr("&Stop playing"));

  explicit PlayMenu(SongWidget &song_widget) : QMenu(PlayMenu::tr("&Play")) {
    add_menu_action(*this, play_action, QKeySequence::UnknownKey, false);
    play_action.setShortcut(Qt::Key_Space);
    add_menu_action(*this, stop_playing_action, QKeySequence::Cancel);

    const auto &player = song_widget.player;
    QObject::connect(&play_action, &QAction::triggered, this, [&song_widget]() {
      const auto &switch_table = song_widget.switch_column.switch_table;
      const auto &song = song_widget.song;
      const auto &pitched_voices = song.pitched_voices;
      const auto &unpitched_voices = song.unpitched_voices;
      auto &player = song_widget.player;
      auto &play_state = player.play_state;

      const auto current_row_type = switch_table.delegate.current_row_type;

      const auto &range = get_only_range(switch_table);
      const auto first_row_number = range.top();
      const auto number_of_rows = get_number_of_rows(range);

      stop_playing(player.sequencer, player.event);
      initialize_play(song_widget);

      if (current_row_type == chord_type) {
        modulate_before_chord(song, play_state, first_row_number);
        play_chords(song_widget, first_row_number, number_of_rows);
      } else if (current_row_type == pitched_note_type || current_row_type == unpitched_note_type) {
        const auto chord_number = get_parent_chord_number(switch_table);
        modulate_before_chord(song, play_state, chord_number);
        const auto &chord = song.chords.at(chord_number);
        modulate(play_state, chord);
        if (current_row_type == pitched_note_type) {
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
      } else if (current_row_type == pitched_voice_type) {
        if (!play_voices(player, pitched_voices, first_row_number,
                         number_of_rows)) {
          return;
        }
      } else if (current_row_type == unpitched_voice_type) {
        if (!play_voices(player, unpitched_voices, first_row_number,
                         number_of_rows)) {
          return;
        }
      } else {
        Q_ASSERT(false);
      }
    });

    QObject::connect(
        &stop_playing_action, &QAction::triggered, this,
        [&player]() { stop_playing(player.sequencer, player.event); });
  }
};

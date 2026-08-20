#pragma once

#include <QtWidgets/QMenu>

enum class RowType : std::uint8_t;
struct SongWidget;

struct PlaySelection {
  RowType row_type;
  int chord_number;  // -1 unless row_type is a note type
  int first_row_number;
  int number_of_rows;
};

[[nodiscard]] auto get_play_selection(const SongWidget& song_widget)
    -> PlaySelection;

struct PlayMenu : public QMenu {
  QAction play_action;
  QAction play_to_end_action;
  QAction stop_playing_action;

  explicit PlayMenu(SongWidget& song_widget);
};

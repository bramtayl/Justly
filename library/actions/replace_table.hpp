#pragma once

#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"
#include "widgets/WindowBody.hpp"

struct PianoRollWidget;
struct SongMenuBar;

void replace_table(SongMenuBar& song_menu_bar, WindowBody& window_body,
                   RowType new_row_type, int new_chord_number,
                   PianoRollWidget& piano_roll_widget,
                   int new_note_number = -1);

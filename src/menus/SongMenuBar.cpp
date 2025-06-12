#ifdef TEST_HOOKS

#include "justly/justly.hpp"
#include "menus/SongMenuBar.hpp"

void trigger_back_to_chords(SongMenuBar &song_menu_bar) {
  song_menu_bar.view_menu.back_to_chords_action.trigger();
}

void trigger_previous_chord(SongMenuBar &song_menu_bar) {
  song_menu_bar.view_menu.previous_chord_action.trigger();
}

void trigger_next_chord(SongMenuBar &song_menu_bar) {
  song_menu_bar.view_menu.next_chord_action.trigger();
}

void trigger_insert_after(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.insert_menu.insert_after_action.trigger();
}

void trigger_insert_into(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.insert_menu.insert_into_action.trigger();
}

void trigger_delete_cells(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.delete_cells_action.trigger();
}

void trigger_remove_rows(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.remove_rows_action.trigger();
}

void trigger_cut(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.cut_action.trigger();
}

void trigger_copy(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.copy_action.trigger();
}

void trigger_paste_over(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.paste_menu.paste_over_action.trigger();
}

void trigger_paste_after(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.paste_menu.paste_after_action.trigger();
}

void trigger_paste_into(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.paste_menu.paste_into_action.trigger();
}

void trigger_save(SongMenuBar &song_menu_bar) {
  song_menu_bar.file_menu.save_action.trigger();
}

void trigger_play(SongMenuBar &song_menu_bar) {
  song_menu_bar.play_menu.play_action.trigger();
}

void trigger_stop_playing(SongMenuBar &song_menu_bar) {
  song_menu_bar.play_menu.stop_playing_action.trigger();
}

#endif
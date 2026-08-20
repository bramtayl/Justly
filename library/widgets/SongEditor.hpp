#pragma once

#include <QtWidgets/QMainWindow>

struct PianoRollWidget;
struct SongMenuBar;
struct SongWidget;

// open_file/import_musicxml replace the song wholesale, bypassing the undo
// stack, so the usual indexChanged-driven refresh never fires for them --
// call this afterward. They also always land back on the chords view (see
// reset_switch_table_to_chords), so reuse replace_table to redo the same
// label/view-menu/selection reset it applies when switching there manually;
// unlike ordinary navigation it doesn't know the song's contents changed
// under it, so the piano roll scene still needs an explicit rebuild on top
void song_reloaded(SongMenuBar& song_menu_bar, SongWidget& song_widget,
                   PianoRollWidget& piano_roll_widget);

void open_file_and_reload(SongMenuBar& song_menu_bar, SongWidget& song_widget,
                          PianoRollWidget& piano_roll_widget,
                          const QString& filename);

void import_musicxml_and_reload(SongMenuBar& song_menu_bar,
                                SongWidget& song_widget,
                                PianoRollWidget& piano_roll_widget,
                                const QString& filename);

void zoom_in_piano_roll(PianoRollWidget& widget);

void zoom_out_piano_roll(PianoRollWidget& widget);

void stop_piano_roll_playhead(PianoRollWidget& widget);

void start_piano_roll_playhead(PianoRollWidget& widget, double baseline_ms,
                               double end_ms);

struct SongEditor : public QMainWindow {
 public:
  SongWidget& song_widget;
  SongMenuBar& song_menu_bar;
  PianoRollWidget& piano_roll_widget;
  QDockWidget& piano_roll_dock;

  explicit SongEditor();

  void closeEvent(QCloseEvent* close_event_pointer) override;
};

void set_up();

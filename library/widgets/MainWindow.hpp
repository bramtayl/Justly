#pragma once

#include <QtWidgets/QMainWindow>

struct PianoRollWidget;
struct SongMenuBar;
struct WindowBody;

// open_file/import_musicxml replace the song wholesale, bypassing the undo
// stack, so the usual indexChanged-driven refresh never fires for them --
// call this afterward. They also always land back on the chords view (see
// reset_switch_table_to_chords), so reuse replace_table to redo the same
// label/view-menu/selection reset it applies when switching there manually;
// unlike ordinary navigation it doesn't know the song's contents changed
// under it, so the piano roll scene still needs an explicit rebuild on top
void song_reloaded(SongMenuBar& song_menu_bar, WindowBody& window_body,
                   PianoRollWidget& piano_roll_widget);

void open_file_and_reload(SongMenuBar& song_menu_bar, WindowBody& window_body,
                          PianoRollWidget& piano_roll_widget,
                          const QString& filename);

void import_musicxml_and_reload(SongMenuBar& song_menu_bar,
                                WindowBody& window_body,
                                PianoRollWidget& piano_roll_widget,
                                const QString& filename);

struct MainWindow : public QMainWindow {
 public:
  WindowBody& window_body;
  SongMenuBar& song_menu_bar;
  PianoRollWidget& piano_roll_widget;
  QDockWidget& piano_roll_dock;

  explicit MainWindow();

  void closeEvent(QCloseEvent* close_event_pointer) override;
};

void set_up();

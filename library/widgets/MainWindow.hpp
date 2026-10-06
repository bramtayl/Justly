#pragma once

#include <QtWidgets/QMainWindow>

struct PianoRollWidget;
struct SongMenuBar;
struct WindowBody;

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

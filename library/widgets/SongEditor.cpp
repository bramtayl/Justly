#include "widgets/SongEditor.hpp"

static void add_replace_table(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                       const RowType new_row_type,
                       const int new_chord_number,
                       PianoRollWidget &piano_roll_widget,
                       const int new_note_number = -1) {
  song_widget.undo_stack.push(
      new ReplaceTable( // NOLINT(cppcoreguidelines-owning-memory)
          song_menu_bar, song_widget, piano_roll_widget, new_row_type,
          new_chord_number, new_note_number));
}

static void connect_switch_to_table(QAction &action, QObject &context,
                             SongMenuBar &song_menu_bar,
                             SongWidget &song_widget,
                             PianoRollWidget &piano_roll_widget,
                             const RowType row_type) {
  QObject::connect(
      &action, &QAction::triggered, &context,
      [&song_menu_bar, &song_widget, &piano_roll_widget, row_type]() -> auto {
        add_replace_table(song_menu_bar, song_widget, row_type, -1,
                          piano_roll_widget);
      });
}

static void connect_navigate_chord_action(QAction &action, QObject &context,
                                   SongMenuBar &song_menu_bar,
                                   SongWidget &song_widget,
                                   PianoRollWidget &piano_roll_widget,
                                   const int delta) {
  QObject::connect(
      &action, &QAction::triggered, &context,
      [&song_menu_bar, &song_widget, &piano_roll_widget, delta]() -> auto {
        const auto &switch_table = song_widget.switch_column.switch_table;
        add_replace_table(song_menu_bar, song_widget,
                          switch_table.delegate.current_row_type,
                          get_parent_chord_number(switch_table) + delta,
                          piano_roll_widget);
      });
}

static void rebuild_piano_roll_scene(PianoRollWidget &widget) {
  rebuild_scene(widget, widget.song_widget, widget.piano_roll_scene,
               widget.axis_scene, widget.legend_scene, widget.row_layout,
               widget.selection_row_type, widget.selection_chord_number,
               widget.selection_first_row_number,
               widget.selection_number_of_rows,
               widget.selecting_chord_from_playhead);
}

void song_reloaded(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                   PianoRollWidget &piano_roll_widget) {
  replace_table(song_menu_bar, song_widget, RowType::chord_type, -1,
               piano_roll_widget);
  rebuild_piano_roll_scene(piano_roll_widget);
}

void open_file_and_reload(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                          PianoRollWidget &piano_roll_widget,
                          const QString &filename) {
  if (open_file(song_widget, filename)) {
    song_reloaded(song_menu_bar, song_widget, piano_roll_widget);
  }
}

void import_musicxml_and_reload(SongMenuBar &song_menu_bar,
                                SongWidget &song_widget,
                                PianoRollWidget &piano_roll_widget,
                                const QString &filename) {
  if (import_musicxml(song_widget, filename)) {
    song_reloaded(song_menu_bar, song_widget, piano_roll_widget);
  }
}

void zoom_in_piano_roll(PianoRollWidget &widget) {
  zoom_in(widget.piano_roll_scene);
}

void zoom_out_piano_roll(PianoRollWidget &widget) {
  zoom_out(widget.piano_roll_scene);
}

void stop_piano_roll_playhead(PianoRollWidget &widget) {
  stop_playhead(widget.piano_roll_scene, widget.axis_scene,
               widget.song_widget.song, widget.selection_row_type,
               widget.selection_chord_number, widget.selection_first_row_number,
               widget.selection_number_of_rows,
               widget.selecting_chord_from_playhead);
}

void start_piano_roll_playhead(PianoRollWidget &widget,
                               const double baseline_ms,
                               const double end_ms) {
  set_manual_scrolling_enabled(widget.piano_roll_scene, widget.axis_scene,
                               false);

  auto &piano_roll_scene = widget.piano_roll_scene;
  piano_roll_scene.playhead_baseline_ms = baseline_ms;
  piano_roll_scene.playhead_end_ms = end_ms;
  piano_roll_scene.playhead_elapsed_timer.restart();
  piano_roll_scene.playhead_active = true;
  piano_roll_scene.playhead_item.show();

  // decides which transition position_playhead() should run, based on
  // where the playhead is starting relative to the view's current
  // (not-yet-moved) center -- see PlayheadTransition
  auto &view = piano_roll_scene.view;
  const auto initial_center_x =
      view.mapToScene(get_reference(view.viewport()).rect()).boundingRect().center().x();
  const auto playhead_x = to_scene_x(piano_roll_scene, baseline_ms);
  if (playhead_x <= initial_center_x) {
    piano_roll_scene.playhead_transition = PlayheadTransition::waiting_to_reach_center;
  } else {
    piano_roll_scene.playhead_transition = PlayheadTransition::catching_up;
    piano_roll_scene.playhead_catchup_start_center_x = initial_center_x;
  }

  position_playhead(piano_roll_scene, baseline_ms);
  piano_roll_scene.playhead_timer.start(PIANO_ROLL_TIMER_INTERVAL_MS);
}

SongEditor::SongEditor()
    : song_widget(*(new SongWidget)),
      song_menu_bar(*(new SongMenuBar(song_widget))) {
  setWindowIcon(QIcon(QString::fromStdString(get_share_file("Justly.svg"))));

  auto &song_menu_bar_ref = this->song_menu_bar;
  auto &song_widget_ref = this->song_widget;
  auto &piano_roll_widget_ref = this->piano_roll_widget;

  auto &switch_table = song_widget.switch_column.switch_table;
  auto &undo_stack = song_widget.undo_stack;

  get_reference(statusBar()).showMessage("");

  setWindowTitle("Justly");
  setCentralWidget(&song_widget);
  setMenuBar(&song_menu_bar);
  resize(QSize(get_reference(QGuiApplication::primaryScreen())
                   .availableGeometry()
                   .size()
                   .width(),
               minimumSizeHint().height()));

  connect_switch_to_table(song_menu_bar.view_menu.back_to_chords_action,
                          *this, song_menu_bar, song_widget,
                          piano_roll_widget, RowType::chord_type);
  connect_switch_to_table(song_menu_bar.view_menu.edit_pitched_voices_action,
                          *this, song_menu_bar, song_widget,
                          piano_roll_widget, RowType::pitched_voice_type);
  connect_switch_to_table(
      song_menu_bar.view_menu.edit_unpitched_voices_action, *this,
      song_menu_bar, song_widget, piano_roll_widget, RowType::unpitched_voice_type);

  QObject::connect(
      &switch_table, &QAbstractItemView::doubleClicked, this,
      [&song_menu_bar_ref, &song_widget_ref,
      &piano_roll_widget_ref](const QModelIndex &index) -> auto {
        if (song_widget_ref.switch_column.switch_table.delegate
                .current_row_type == RowType::chord_type) {
          const auto column = index.column();
          const auto is_pitched = column == static_cast<int>(ChordColumn::chord_pitched_notes_column);
          if (is_pitched || (column == static_cast<int>(ChordColumn::chord_unpitched_notes_column))) {
            add_replace_table(
                song_menu_bar_ref, song_widget_ref,
                (is_pitched ? RowType::pitched_note_type : RowType::unpitched_note_type),
                index.row(), piano_roll_widget_ref);
          }
        }
      });

  connect_navigate_chord_action(song_menu_bar.view_menu.previous_chord_action,
                                *this, song_menu_bar, song_widget,
                                piano_roll_widget, -1);
  connect_navigate_chord_action(song_menu_bar.view_menu.next_chord_action,
                                *this, song_menu_bar, song_widget,
                                piano_roll_widget, 1);

  piano_roll_dock.setWidget(&piano_roll_widget);
  addDockWidget(Qt::BottomDockWidgetArea, &piano_roll_dock);
  piano_roll_dock.setVisible(false);

  auto &show_piano_roll_action = song_menu_bar.view_menu.show_piano_roll_action;
  QObject::connect(&show_piano_roll_action, &QAction::toggled,
                   &piano_roll_dock, &QDockWidget::setVisible);
  QObject::connect(&piano_roll_dock, &QDockWidget::visibilityChanged,
                   &show_piano_roll_action, &QAction::setChecked);

  QObject::connect(&song_menu_bar.view_menu.zoom_in_action,
                   &QAction::triggered, &piano_roll_widget_ref,
                   [&piano_roll_widget_ref]() -> auto {
                     zoom_in_piano_roll(piano_roll_widget_ref);
                   });
  QObject::connect(&song_menu_bar.view_menu.zoom_out_action,
                   &QAction::triggered, &piano_roll_widget_ref,
                   [&piano_roll_widget_ref]() -> auto {
                     zoom_out_piano_roll(piano_roll_widget_ref);
                   });

  QObject::connect(&undo_stack, &QUndoStack::indexChanged, this,
                   [&piano_roll_widget_ref]() -> auto {
                     rebuild_piano_roll_scene(piano_roll_widget_ref);
                   });

  connect_recovery_timer(song_widget);

  QObject::connect(
      &song_menu_bar.file_menu.open_action, &QAction::triggered, this,
      [&song_menu_bar_ref, &song_widget_ref, &piano_roll_widget_ref]() -> auto {
        if (can_discard_changes(song_widget_ref)) {
          auto &dialog = make_file_dialog(
              song_widget_ref, "Open — Justly", "XML file (*.xml)",
              QFileDialog::AcceptOpen, ".xml", QFileDialog::ExistingFile);
          if (dialog.exec() != 0) {
            open_file_and_reload(song_menu_bar_ref, song_widget_ref,
                                 piano_roll_widget_ref,
                                 get_selected_file(song_widget_ref, dialog));
          }
          dialog.deleteLater();
        }
      });
  QObject::connect(
      &song_menu_bar.file_menu.import_action, &QAction::triggered, this,
      [&song_menu_bar_ref, &song_widget_ref, &piano_roll_widget_ref]() -> auto {
        if (can_discard_changes(song_widget_ref)) {
          auto &dialog = make_file_dialog(
              song_widget_ref, "Import MusicXML — Justly",
              "MusicXML file (*.musicxml *.mxl)", QFileDialog::AcceptOpen,
              ".musicxml", QFileDialog::ExistingFile);
          if (dialog.exec() != 0) {
            import_musicxml_and_reload(
                song_menu_bar_ref, song_widget_ref, piano_roll_widget_ref,
                get_selected_file(song_widget_ref, dialog));
          }
          dialog.deleteLater();
        }
      });

  // double-clicking a note in the piano roll opens the pitched/unpitched
  // notes table for its chord, scrolled to and highlighting that note --
  // mirroring the chords table's own double-click-into-notes behavior
  piano_roll_widget.note_double_clicked =
      [&song_menu_bar_ref, &song_widget_ref,
      &piano_roll_widget_ref](const int chord_number, const int note_number,
                              const bool is_pitched) -> void {
        add_replace_table(song_menu_bar_ref, song_widget_ref,
                          is_pitched
                              ? RowType::pitched_note_type
                              : RowType::unpitched_note_type,
                          chord_number, piano_roll_widget_ref, note_number);
      };

  // wires the piano roll's playhead animation to the existing Play/Stop
  // actions, since playback itself remains fire-and-forget (no
  // pause/resume, no "now playing" callback from FluidSynth) — the
  // playhead is driven purely by wall-clock elapsed time against the same
  // precomputed schedule bounds.
  auto &play_menu = song_menu_bar.play_menu;
  QObject::connect(
      &play_menu.play_action, &QAction::triggered, this,
      [&piano_roll_widget_ref, &song_widget_ref]() -> auto {
        const auto selection = get_play_selection(song_widget_ref);
        if (selection.row_type == RowType::pitched_voice_type ||
            selection.row_type == RowType::unpitched_voice_type) {
          // voice audition/preview has no timeline position
          stop_piano_roll_playhead(piano_roll_widget_ref);
          return;
        }
        const auto is_chord_selection = selection.row_type == RowType::chord_type;
        const auto [baseline_ms, end_ms] = get_piano_roll_time_bounds(
            song_widget_ref.song,
            is_chord_selection ? selection.first_row_number
                               : selection.chord_number,
            is_chord_selection ? selection.number_of_rows : 1,
            is_chord_selection ? 0 : selection.first_row_number,
            is_chord_selection ? -1 : selection.number_of_rows,
            is_chord_selection
                ? std::nullopt
                : std::make_optional(
                      selection.row_type == RowType::pitched_note_type));
        start_piano_roll_playhead(piano_roll_widget_ref, baseline_ms, end_ms);
      });
  QObject::connect(
      &play_menu.play_to_end_action, &QAction::triggered, this,
      [&piano_roll_widget_ref, &song_widget_ref]() -> auto {
        const auto &song = song_widget_ref.song;
        const auto selection = get_play_selection(song_widget_ref);
        if (selection.row_type == RowType::pitched_voice_type ||
            selection.row_type == RowType::unpitched_voice_type) {
          // voice audition/preview has no timeline position
          stop_piano_roll_playhead(piano_roll_widget_ref);
          return;
        }
        const auto is_chord_selection = selection.row_type == RowType::chord_type;
        const auto first_chord_number =
            is_chord_selection ? selection.first_row_number : selection.chord_number;
        const auto baseline_ms = get_piano_roll_time_bounds(
            song, first_chord_number,
            is_chord_selection ? selection.number_of_rows : 1,
            is_chord_selection ? 0 : selection.first_row_number,
            is_chord_selection ? -1 : selection.number_of_rows,
            is_chord_selection
                ? std::nullopt
                : std::make_optional(
                      selection.row_type == RowType::pitched_note_type)).first;
        // "play to end" always continues through every remaining chord in
        // full, regardless of note-row selection, so the end bound must
        // span the whole remaining song rather than just the selected notes
        const auto end_ms = get_piano_roll_time_bounds(
            song, first_chord_number,
            static_cast<int>(song.chords.size()) - first_chord_number)
                                .second;
        start_piano_roll_playhead(piano_roll_widget_ref, baseline_ms, end_ms);
      });
  QObject::connect(
      &play_menu.stop_playing_action, &QAction::triggered, this,
      [&piano_roll_widget_ref]() -> auto { stop_piano_roll_playhead(piano_roll_widget_ref); });

  add_replace_table(song_menu_bar, song_widget, RowType::pitched_voice_type, -1,
                    piano_roll_widget);
  add_insert_row(song_widget, 0, RowType::pitched_voice_type);
  add_replace_table(song_menu_bar, song_widget, RowType::unpitched_voice_type, -1,
                    piano_roll_widget);
  add_insert_row(song_widget, 0, RowType::unpitched_voice_type);
  add_replace_table(song_menu_bar, song_widget, RowType::chord_type, -1,
                    piano_roll_widget);
  clear_and_clean(undo_stack);
}

void SongEditor::closeEvent(QCloseEvent *close_event_pointer) {
  if (!can_discard_changes(song_widget)) {
    get_reference(close_event_pointer).ignore();
    return;
  }
  remove_recovery_file();
  QMainWindow::closeEvent(close_event_pointer);
}

static void write_rational(QTextStream &stream, const Rational &rational) {
  const auto numerator = rational.numerator;
  const auto denominator = rational.denominator;
  if (numerator != 1) {
    stream << numerator;
  }
  if (denominator != 1) {
    stream << "/" << denominator;
  }
}

void set_up() {
  LIBXML_TEST_VERSION

  // needed for QStandardPaths::AppDataLocation and QSettings (used by the
  // crash-recovery feature) to resolve to a stable, Justly-specific location
  QApplication::setOrganizationName("Justly");
  QApplication::setApplicationName("Justly");
  QApplication::setApplicationDisplayName("Justly");

  const QPixmap pixmap(get_share_file("Justly.svg").c_str());
  if (!pixmap.isNull()) {
    QApplication::setWindowIcon(QIcon(pixmap));
  }

  QMetaType::registerConverter<Rational, QString>([](const Rational &rational) -> auto {
    QString result;
    QTextStream stream(&result);
    write_rational(stream, rational);
    return result;
  });
  QMetaType::registerConverter<Interval, QString>([](const Interval &interval) -> auto {
    const auto octave = interval.octave;

    QString result;
    QTextStream stream(&result);
    write_rational(stream, interval.ratio);
    if (octave != 0) {
      stream << "o" << octave;
    }
    return result;
  });
}

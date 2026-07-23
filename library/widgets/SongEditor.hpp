#pragma once

#include <QtGui/QCloseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QItemEditorFactory>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStandardItemEditorCreator>
#include <QtWidgets/QStatusBar>

#include "actions/ReplaceTable.hpp"
#include "menus/SongMenuBar.hpp"
#include "rows/RowType.hpp"
#include "widgets/PianoRollWidget.hpp"

static void add_replace_table(SongMenuBar &song_menu_bar,
                              SongWidget &song_widget,
                              const RowType new_row_type,
                              const int new_chord_number,
                              PianoRollWidget &piano_roll_widget,
                              const int new_note_number = -1) {
  song_widget.undo_stack.push(
      new ReplaceTable( // NOLINT(cppcoreguidelines-owning-memory)
          song_menu_bar, song_widget, piano_roll_widget, new_row_type,
          new_chord_number, new_note_number));
}

// wires an action that just switches to a fixed table (voices/chords)
static void connect_switch_to_table(QAction &action, QObject &context,
                                    SongMenuBar &song_menu_bar,
                                    SongWidget &song_widget,
                                    PianoRollWidget &piano_roll_widget,
                                    const RowType row_type) {
  QObject::connect(
      &action, &QAction::triggered, &context,
      [&song_menu_bar, &song_widget, &piano_roll_widget, row_type]() {
        add_replace_table(song_menu_bar, song_widget, row_type, -1,
                          piano_roll_widget);
      });
}

// wires the previous/next chord actions, which stay on the current table
// type but step the chord number by delta
static void connect_navigate_chord_action(QAction &action, QObject &context,
                                          SongMenuBar &song_menu_bar,
                                          SongWidget &song_widget,
                                          PianoRollWidget &piano_roll_widget,
                                          const int delta) {
  QObject::connect(
      &action, &QAction::triggered, &context,
      [&song_menu_bar, &song_widget, &piano_roll_widget, delta]() {
        const auto &switch_table = song_widget.switch_column.switch_table;
        add_replace_table(song_menu_bar, song_widget,
                          switch_table.delegate.current_row_type,
                          get_parent_chord_number(switch_table) + delta,
                          piano_roll_widget);
      });
}

// wires the piano roll's playhead animation to the existing Play/Stop
// actions, since playback itself remains fire-and-forget (no pause/resume,
// no "now playing" callback from FluidSynth) — the playhead is driven purely
// by wall-clock elapsed time against the same precomputed schedule bounds.
static void connect_piano_roll_playhead(PianoRollWidget &piano_roll_widget,
                                        QObject &context,
                                        SongWidget &song_widget,
                                        PlayMenu &play_menu) {
  QObject::connect(
      &play_menu.play_action, &QAction::triggered, &context,
      [&piano_roll_widget, &song_widget]() {
        const auto selection = get_play_selection(song_widget);
        if (selection.row_type == pitched_voice_type ||
            selection.row_type == unpitched_voice_type) {
          // voice audition/preview has no timeline position
          piano_roll_widget.stop_playhead();
          return;
        }
        const auto is_chord_selection = selection.row_type == chord_type;
        const auto [baseline_ms, end_ms] = get_piano_roll_time_bounds(
            song_widget.song,
            is_chord_selection ? selection.first_row_number
                               : selection.chord_number,
            is_chord_selection ? selection.number_of_rows : 1,
            is_chord_selection ? 0 : selection.first_row_number,
            is_chord_selection ? -1 : selection.number_of_rows,
            is_chord_selection
                ? std::nullopt
                : std::make_optional(selection.row_type == pitched_note_type
                                         ? PianoRollNoteKind::pitched_kind
                                         : PianoRollNoteKind::unpitched_kind));
        piano_roll_widget.start_playhead(baseline_ms, end_ms);
      });
  QObject::connect(
      &play_menu.stop_playing_action, &QAction::triggered, &context,
      [&piano_roll_widget]() { piano_roll_widget.stop_playhead(); });
}

struct SongEditor : public QMainWindow {
public:
  SongWidget &song_widget;
  SongMenuBar &song_menu_bar;
  PianoRollWidget &piano_roll_widget = *(new PianoRollWidget(song_widget));
  QDockWidget &piano_roll_dock =
      *(new QDockWidget(SongEditor::tr("Piano Roll"), this));

  explicit SongEditor()
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
                            piano_roll_widget, chord_type);
    connect_switch_to_table(song_menu_bar.view_menu.edit_pitched_voices_action,
                            *this, song_menu_bar, song_widget,
                            piano_roll_widget, pitched_voice_type);
    connect_switch_to_table(
        song_menu_bar.view_menu.edit_unpitched_voices_action, *this,
        song_menu_bar, song_widget, piano_roll_widget, unpitched_voice_type);

    QObject::connect(
        &switch_table, &QAbstractItemView::doubleClicked, this,
        [&song_menu_bar_ref, &song_widget_ref,
        &piano_roll_widget_ref](const QModelIndex &index) {
          if (song_widget_ref.switch_column.switch_table.delegate
                  .current_row_type == chord_type) {
            const auto column = index.column();
            const auto is_pitched = column == chord_pitched_notes_column;
            if (is_pitched || (column == chord_unpitched_notes_column)) {
              add_replace_table(
                  song_menu_bar_ref, song_widget_ref,
                  (is_pitched ? pitched_note_type : unpitched_note_type),
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

    QObject::connect(&undo_stack, &QUndoStack::indexChanged, this,
                     [&piano_roll_widget_ref]() {
                       piano_roll_widget_ref.rebuild_scene();
                     });

    // double-clicking a note in the piano roll opens the pitched/unpitched
    // notes table for its chord, scrolled to and highlighting that note --
    // mirroring the chords table's own double-click-into-notes behavior
    piano_roll_widget.note_double_clicked =
        [&song_menu_bar_ref, &song_widget_ref,
        &piano_roll_widget_ref](const int chord_number, const int note_number,
                                const PianoRollNoteKind kind) -> void {
          add_replace_table(song_menu_bar_ref, song_widget_ref,
                            kind == PianoRollNoteKind::pitched_kind
                                ? pitched_note_type
                                : unpitched_note_type,
                            chord_number, piano_roll_widget_ref, note_number);
        };

    connect_piano_roll_playhead(piano_roll_widget, *this, song_widget,
                                song_menu_bar.play_menu);

    add_replace_table(song_menu_bar, song_widget, pitched_voice_type, -1,
                      piano_roll_widget);
    add_insert_row(song_widget, 0, pitched_voice_type);
    add_replace_table(song_menu_bar, song_widget, unpitched_voice_type, -1,
                      piano_roll_widget);
    add_insert_row(song_widget, 0, unpitched_voice_type);
    add_replace_table(song_menu_bar, song_widget, chord_type, -1,
                      piano_roll_widget);
    clear_and_clean(undo_stack);
  };
  void closeEvent(QCloseEvent *close_event_pointer) override {
    if (!can_discard_changes(song_widget)) {
      get_reference(close_event_pointer).ignore();
      return;
    }
    QMainWindow::closeEvent(close_event_pointer);
  };
};

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

inline void set_up() {
  LIBXML_TEST_VERSION

  QApplication::setApplicationDisplayName("Justly");

  const QPixmap pixmap(get_share_file("Justly.svg").c_str());
  if (!pixmap.isNull()) {
    QApplication::setWindowIcon(QIcon(pixmap));
  }

  QMetaType::registerConverter<Rational, QString>([](const Rational &rational) {
    QString result;
    QTextStream stream(&result);
    write_rational(stream, rational);
    return result;
  });
  QMetaType::registerConverter<Interval, QString>([](const Interval &interval) {
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


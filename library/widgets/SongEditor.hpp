#pragma once

#include <QtGui/QCloseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QItemEditorFactory>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStandardItemEditorCreator>
#include <QtWidgets/QStatusBar>

#include "actions/ReplaceTable.hpp"
#include "menus/SongMenuBar.hpp"
#include "rows/RowType.hpp"

static void add_replace_table(SongMenuBar &song_menu_bar,
                              SongWidget &song_widget,
                              const RowType new_row_type,
                              const int new_chord_number) {
  song_widget.undo_stack.push(
      new ReplaceTable( // NOLINT(cppcoreguidelines-owning-memory)
          song_menu_bar, song_widget, new_row_type, new_chord_number));
}

struct SongEditor : public QMainWindow {
public:
  SongWidget &song_widget;
  SongMenuBar &song_menu_bar;

  explicit SongEditor()
      : song_widget(*(new SongWidget)),
        song_menu_bar(*(new SongMenuBar(song_widget))) {
    setWindowIcon(QIcon(QString::fromStdString(get_share_file("Justly.svg"))));

    auto &song_menu_bar_ref = this->song_menu_bar;
    auto &song_widget_ref = this->song_widget;

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

    replace_table(song_menu_bar, song_widget, chord_type, -1);

    QObject::connect(
        &song_menu_bar.view_menu.back_to_chords_action, &QAction::triggered,
        this, [&song_menu_bar_ref, &song_widget_ref]() {
          add_replace_table(song_menu_bar_ref, song_widget_ref, chord_type, -1);
        });

    QObject::connect(&song_menu_bar.view_menu.edit_pitched_voices_action,
                     &QAction::triggered, this,
                     [&song_menu_bar_ref, &song_widget_ref]() {
                       add_replace_table(song_menu_bar_ref, song_widget_ref,
                                         pitched_voice_type, -1);
                     });

    QObject::connect(&song_menu_bar.view_menu.edit_unpitched_voices_action,
                     &QAction::triggered, this,
                     [&song_menu_bar_ref, &song_widget_ref]() {
                       add_replace_table(song_menu_bar_ref, song_widget_ref,
                                         unpitched_voice_type, -1);
                     });

    QObject::connect(
        &switch_table, &QAbstractItemView::doubleClicked, this,
        [&song_menu_bar_ref, &song_widget_ref](const QModelIndex &index) {
          if (song_widget_ref.switch_column.switch_table.delegate
                  .current_row_type == chord_type) {
            const auto column = index.column();
            const auto is_pitched = column == chord_pitched_notes_column;
            if (is_pitched || (column == chord_unpitched_notes_column)) {
              add_replace_table(
                  song_menu_bar_ref, song_widget_ref,
                  (is_pitched ? pitched_note_type : unpitched_note_type),
                  index.row());
            }
          }
        });

    QObject::connect(
        &song_menu_bar.view_menu.previous_chord_action, &QAction::triggered,
        this, [&song_menu_bar_ref, &song_widget_ref]() {
          const auto &switch_table = song_widget_ref.switch_column.switch_table;
          add_replace_table(song_menu_bar_ref, song_widget_ref,
                            switch_table.delegate.current_row_type,
                            get_parent_chord_number(switch_table) - 1);
        });
    QObject::connect(
        &song_menu_bar.view_menu.next_chord_action, &QAction::triggered, this,
        [&song_menu_bar_ref, &song_widget_ref]() {
          const auto &switch_table = song_widget_ref.switch_column.switch_table;
          add_replace_table(song_menu_bar_ref, song_widget_ref,
                            switch_table.delegate.current_row_type,
                            get_parent_chord_number(switch_table) + 1);
        });

    add_replace_table(song_menu_bar, song_widget, chord_type, -1);
    add_replace_table(song_menu_bar, song_widget, pitched_voice_type, -1);
    add_insert_row(song_widget, 0, pitched_voice_type);
    add_replace_table(song_menu_bar, song_widget, unpitched_voice_type, -1);
    add_insert_row(song_widget, 0, unpitched_voice_type);
    add_replace_table(song_menu_bar, song_widget, chord_type, -1);
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

inline void set_up() {
  LIBXML_TEST_VERSION

  QApplication::setApplicationDisplayName("Justly");

  const QPixmap pixmap(get_share_file("Justly.svg").c_str());
  if (!pixmap.isNull()) {
    QApplication::setWindowIcon(QIcon(pixmap));
  }

  QMetaType::registerConverter<Rational, QString>([](const Rational &rational) {
    const auto numerator = rational.numerator;
    const auto denominator = rational.denominator;

    QString result;
    QTextStream stream(&result);
    if (numerator != 1) {
      stream << numerator;
    }
    if (denominator != 1) {
      stream << "/" << denominator;
    }
    return result;
  });
  QMetaType::registerConverter<Interval, QString>([](const Interval &interval) {
    const auto numerator = interval.ratio.numerator;
    const auto denominator = interval.ratio.denominator;
    const auto octave = interval.octave;

    QString result;
    QTextStream stream(&result);
    if (numerator != 1) {
      stream << numerator;
    }
    if (denominator != 1) {
      stream << "/" << denominator;
    }
    if (octave != 0) {
      stream << "o" << octave;
    }
    return result;
  });
}

// TODO(brandon): transposing instruments
// TODO(brandon): instrument mapping for musicxml
// TODO(brandon): musicxml repeats
// TODO(brandon): audit string encoding issues
// TODO(brandon): check pitched/unpitched voice consistency in xml on import (no duplicated or empty names, correspondence between voices and notes)
// TODO(brandon): upon voice rename, update note voices
// TODO(brandon): disable copy/paste of pitched/unpitched voice names
// TODO(brandon): upon deletion of voice, warn for corresponding notes and
// update notes accordingly
// TODO(brandon): add voice tests

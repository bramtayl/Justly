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

struct SongEditor : public QMainWindow {
public:
  SongWidget &song_widget;
  SongMenuBar &song_menu_bar;

  explicit SongEditor();
  void closeEvent(QCloseEvent *close_event_pointer) override;
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
  QMetaType::registerConverter<const Program *, QString>(
      [](const Program *program_pointer) {
        if (program_pointer == nullptr) {
          return QString("");
        }
        return get_reference(program_pointer).translated_name;
      });
  QMetaType::registerConverter<PercussionInstrument, QString>(
      [](const PercussionInstrument &percussion_instrument) {
        if (percussion_instrument_is_default(percussion_instrument)) {
          return QString("");
        }
        QString result;
        QTextStream stream(&result);
        stream << get_reference(percussion_instrument.percussion_set_pointer)
                      .translated_name
               << " #" << percussion_instrument.midi_number;
        return result;
      });

  auto &factory = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QItemEditorFactory);
  factory.registerEditor(
      qMetaTypeId<Rational>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          RationalEditor>);
  factory.registerEditor(
      qMetaTypeId<const Program *>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          ProgramEditor>);
  factory.registerEditor(
      qMetaTypeId<PercussionInstrument>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          PercussionInstrumentEditor>);
  factory.registerEditor(
      qMetaTypeId<Interval>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          IntervalEditor>);
  factory.registerEditor(
      qMetaTypeId<QString>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          QLineEdit>);
  QItemEditorFactory::setDefaultFactory(&factory);
}

static void add_replace_table(SongMenuBar &song_menu_bar,
                              SongWidget &song_widget,
                              const RowType new_row_type,
                              const int new_chord_number) {
  song_widget.undo_stack.push(
      new ReplaceTable( // NOLINT(cppcoreguidelines-owning-memory)
          song_menu_bar, song_widget, new_row_type, new_chord_number));
}

static void connect_selection_model(SongMenuBar &song_menu_bar,
                                    SongWidget &song_widget,
                                    const QAbstractItemView &table) {
  const auto &selection_model = get_selection_model(table);
  update_actions(song_menu_bar, song_widget, selection_model);
  QObject::connect(
      &selection_model, &QItemSelectionModel::selectionChanged,
      &selection_model, [&song_menu_bar, &song_widget, &selection_model]() {
        update_actions(song_menu_bar, song_widget, selection_model);
      });
}

inline SongEditor::SongEditor()
    : song_widget(*(new SongWidget)),
      song_menu_bar(*(new SongMenuBar(song_widget))) {
  setWindowIcon(QIcon(QString::fromStdString(get_share_file("Justly.svg"))));

  auto &song_menu_bar_ref = this->song_menu_bar;
  auto &song_widget_ref = this->song_widget;

  auto &switch_column = song_widget.switch_column;
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

  connect_selection_model(song_menu_bar, song_widget,
                          switch_column.chords_table);
  connect_selection_model(song_menu_bar, song_widget,
                          switch_column.pitched_notes_table);
  connect_selection_model(song_menu_bar, song_widget,
                          switch_column.unpitched_notes_table);

  QObject::connect(
      &song_menu_bar.view_menu.back_to_chords_action, &QAction::triggered, this,
      [&song_menu_bar_ref, &song_widget_ref]() {
        add_replace_table(song_menu_bar_ref, song_widget_ref, chord_type, -1);
      });

  QObject::connect(
      &switch_column.chords_table, &QAbstractItemView::doubleClicked, this,
      [&song_menu_bar_ref, &song_widget_ref](const QModelIndex &index) {
        const auto column = index.column();
        const auto is_pitched = column == chord_pitched_notes_column;
        if (is_pitched || (column == chord_unpitched_notes_column)) {
          add_replace_table(
              song_menu_bar_ref, song_widget_ref,
              (is_pitched ? pitched_note_type : unpitched_note_type),
              index.row());
        }
      });

  QObject::connect(
      &song_menu_bar.view_menu.previous_chord_action, &QAction::triggered, this,
      [&song_menu_bar_ref, &song_widget_ref]() {
        const auto &switch_column = song_widget_ref.switch_column;
        add_replace_table(song_menu_bar_ref, song_widget_ref,
                          switch_column.current_row_type,
                          get_parent_chord_number(switch_column) - 1);
      });
  QObject::connect(
      &song_menu_bar.view_menu.next_chord_action, &QAction::triggered, this,
      [&song_menu_bar_ref, &song_widget_ref]() {
        const auto &switch_column = song_widget_ref.switch_column;
        add_replace_table(song_menu_bar_ref, song_widget_ref,
                          switch_column.current_row_type,
                          get_parent_chord_number(switch_column) + 1);
      });

  add_replace_table(song_menu_bar, song_widget, chord_type, -1);
  clear_and_clean(undo_stack);
}

inline void SongEditor::closeEvent(QCloseEvent *const close_event_pointer) {
  if (!can_discard_changes(song_widget)) {
    get_reference(close_event_pointer).ignore();
    return;
  }
  QMainWindow::closeEvent(close_event_pointer);
};

// TODO(brandon): instrument mapping for musicxml
// TODO(brandon): musicxml repeats
// TODO(brandon): audit string encoding issues

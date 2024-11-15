#include "justly/justly.hpp"

#include <QAbstractItemDelegate>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QIcon>
#include <QItemEditorFactory>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMetaObject>
#include <QMetaType>
#include <QObject>
#include <QSpinBox>
#include <QString>
#include <QStringListModel>
#include <QStyleOption>
#include <QTableView>
#include <QTextStream>
#include <QUndoStack>
#include <QWidget>
#include <QtGlobal>
#include <algorithm>
#include <concepts>
#include <iterator>

#include "abstract_rational/AbstractRational.hpp"
#include "abstract_rational/interval/Interval.hpp"
#include "abstract_rational/rational/Rational.hpp"
#include "justly/ChordColumn.hpp"
#include "named/Named.hpp"
#include "named/percussion_instrument/PercussionInstrument.hpp"
#include "named/program/instrument/Instrument.hpp"
#include "named/program/percussion_set/PercussionSet.hpp"
#include "other/other.hpp"
#include "row/RowsModel.hpp"
#include "row/chord/ChordsModel.hpp"
#include "row/note/pitched_note/PitchedNotesModel.hpp"
#include "song/Player.hpp"
#include "song/Song.hpp"
#include "song/SongEditor.hpp"

static void prevent_compression(QWidget &widget) {
  widget.setMinimumSize(widget.minimumSizeHint());
}

template <std::derived_from<Named> SubNamed>
struct NamedEditor : public QComboBox {
public:
  explicit NamedEditor(QWidget *parent_pointer) : QComboBox(parent_pointer) {

    static auto names_model = []() {
      const auto &all_nameds = SubNamed::get_all_nameds();
      QList<QString> names({""});
      std::transform(all_nameds.cbegin(), all_nameds.cend(),
                     std::back_inserter(names), [](const SubNamed &item) {
                       return QObject::tr(item.name.toStdString().c_str());
                     });
      return QStringListModel(names);
    }();
    setModel(&names_model);
    prevent_compression(*this);
    // force scrollbar for combo box
    setStyleSheet("combobox-popup: 0;");
  };

  [[nodiscard]] auto value() const -> const SubNamed * {
    auto row = currentIndex();
    if (row == 0) {
      return nullptr;
    }
    return &SubNamed::get_all_nameds().at(row - 1);
  };

  void setValue(const SubNamed *new_value) {
    setCurrentIndex(new_value == nullptr
                        ? 0
                        : static_cast<int>(std::distance(
                              SubNamed::get_all_nameds().data(), new_value)) +
                              1);
  };
};

struct InstrumentEditor : public NamedEditor<Instrument> {
  Q_OBJECT
  Q_PROPERTY(const Instrument *value READ value WRITE setValue USER true)
public:
  explicit InstrumentEditor(QWidget *parent_pointer)
      : NamedEditor<Instrument>(parent_pointer){};
};

struct PercussionSetEditor : public NamedEditor<PercussionSet> {
  Q_OBJECT
  Q_PROPERTY(const PercussionSet *value READ value WRITE setValue USER true)
public:
  explicit PercussionSetEditor(QWidget *parent_pointer_input)
      : NamedEditor<PercussionSet>(parent_pointer_input){};
};

struct PercussionInstrumentEditor : public NamedEditor<PercussionInstrument> {
  Q_OBJECT
  Q_PROPERTY(
      const PercussionInstrument *value READ value WRITE setValue USER true)
public:
  explicit PercussionInstrumentEditor(QWidget *parent_pointer)
      : NamedEditor<PercussionInstrument>(parent_pointer){};
};

struct AbstractRationalEditor : public QFrame {
public:
  QSpinBox &numerator_box;
  QSpinBox &denominator_box;
  QHBoxLayout &row_layout;

  explicit AbstractRationalEditor(QWidget *parent_pointer)
      : QFrame(parent_pointer), numerator_box(*(new QSpinBox)),
        denominator_box(*(new QSpinBox)), row_layout(*(new QHBoxLayout(this))) {
    setFrameStyle(QFrame::StyledPanel);
    setAutoFillBackground(true);

    numerator_box.setMinimum(1);
    numerator_box.setMaximum(MAX_RATIONAL_NUMERATOR);

    denominator_box.setMinimum(1);
    denominator_box.setMaximum(MAX_RATIONAL_DENOMINATOR);

    row_layout.addWidget(&numerator_box);
    row_layout.addWidget(
        new QLabel("/")); // NOLINT(cppcoreguidelines-owning-memory)
    row_layout.addWidget(&denominator_box);
  };

  void setValue(const AbstractRational &new_value) const {
    numerator_box.setValue(new_value.numerator);
    denominator_box.setValue(new_value.denominator);
  };
};

struct IntervalEditor : public AbstractRationalEditor {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ value WRITE setValue USER true)

public:
  QSpinBox &octave_box;

  explicit IntervalEditor(QWidget *parent_pointer)
      : AbstractRationalEditor(parent_pointer), octave_box(*(new QSpinBox)) {
    octave_box.setMinimum(-MAX_OCTAVE);
    octave_box.setMaximum(MAX_OCTAVE);

    row_layout.addWidget(
        new QLabel("o")); // NOLINT(cppcoreguidelines-owning-memory)
    row_layout.addWidget(&octave_box);

    prevent_compression(*this);
  };

  [[nodiscard]] auto value() const -> Interval {
    return {numerator_box.value(), denominator_box.value(), octave_box.value()};
  };
  void setValue(const Interval &new_value) const {
    AbstractRationalEditor::setValue(new_value);
    octave_box.setValue(new_value.octave);
  };
};

struct RationalEditor : public AbstractRationalEditor {
  Q_OBJECT
  Q_PROPERTY(Rational rational READ value WRITE setValue USER true)

public:
  explicit RationalEditor(QWidget *parent_pointer)
      : AbstractRationalEditor(parent_pointer) {
    prevent_compression(*this);
  };

  [[nodiscard]] auto value() const -> Rational {
    return {numerator_box.value(), denominator_box.value()};
  };
};

static auto get_name_or_empty(const Named *named_pointer) -> QString {
  if (named_pointer == nullptr) {
    return "";
  }
  return named_pointer->name;
}

void set_up() {
  QApplication::setApplicationDisplayName("Justly");

  auto icon_file = QDir(QCoreApplication::applicationDirPath())
                       .filePath("../share/Justly.svg");
  Q_ASSERT(QFile::exists(icon_file));
  QIcon icon(icon_file);
  Q_ASSERT(!icon.isNull());
  QApplication::setWindowIcon(icon);

  QMetaType::registerConverter<Rational, QString>([](const Rational &rational) {
    auto numerator = rational.numerator;
    auto denominator = rational.denominator;

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
    auto numerator = interval.numerator;
    auto denominator = interval.denominator;
    auto octave = interval.octave;

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
  QMetaType::registerConverter<const Instrument *, QString>(&get_name_or_empty);
  QMetaType::registerConverter<const PercussionInstrument *, QString>(
      &get_name_or_empty);
  QMetaType::registerConverter<const PercussionSet *, QString>(
      &get_name_or_empty);

  auto &factory = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QItemEditorFactory);
  factory.registerEditor(
      qMetaTypeId<Rational>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          RationalEditor>);
  factory.registerEditor(
      qMetaTypeId<const PercussionInstrument *>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          PercussionInstrumentEditor>);
  factory.registerEditor(
      qMetaTypeId<const PercussionSet *>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          PercussionSetEditor>);
  factory.registerEditor(
      qMetaTypeId<const Instrument *>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          InstrumentEditor>);
  factory.registerEditor(
      qMetaTypeId<Interval>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          IntervalEditor>);
  factory.registerEditor(
      qMetaTypeId<QString>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          QLineEdit>);
  factory.registerEditor(
      qMetaTypeId<int>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          QSpinBox>);
  QItemEditorFactory::setDefaultFactory(&factory);
}

auto make_song_editor() -> SongEditor * {
  return new SongEditor; // NOLINT(cppcoreguidelines-owning-memory)
}

void show_song_editor(SongEditor *song_editor_pointer) {
  return get_reference(song_editor_pointer).show();
}

void delete_song_editor(SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  delete song_editor_pointer; // NOLINT(cppcoreguidelines-owning-memory)
}

auto get_table_view(const SongEditor *song_editor_pointer)
    -> QAbstractItemView & {
  return get_reference(song_editor_pointer).table_view;
}

auto get_chords_model(SongEditor *song_editor_pointer) -> QAbstractItemModel & {
  return get_reference(song_editor_pointer).chords_model;
};

auto get_pitched_notes_model(SongEditor *song_editor_pointer)
    -> QAbstractItemModel & {
  return get_reference(song_editor_pointer).pitched_notes_model;
};

auto get_unpitched_notes_model(SongEditor *song_editor_pointer)
    -> QAbstractItemModel & {
  return get_reference(song_editor_pointer).unpitched_notes_model;
};

void trigger_edit_pitched_notes(SongEditor *song_editor_pointer,
                                int chord_number) {
  auto &song_editor = get_reference(song_editor_pointer);
  song_editor.table_view.doubleClicked(
      song_editor.chords_model.index(chord_number, chord_pitched_notes_column));
};

void trigger_edit_unpitched_notes(SongEditor *song_editor_pointer,
                                  int chord_number) {
  auto &song_editor = get_reference(song_editor_pointer);
  song_editor.table_view.doubleClicked(song_editor.chords_model.index(
      chord_number, chord_unpitched_notes_column));
};

void trigger_back_to_chords(SongEditor *song_editor_pointer) {
  get_reference(song_editor_pointer).back_to_chords_action.trigger();
};

auto get_gain(const SongEditor *song_editor_pointer) -> double {
  return reference_get_gain(get_reference(song_editor_pointer));
};

auto get_starting_key(const SongEditor *song_editor_pointer) -> double {
  return get_reference(song_editor_pointer).song.starting_key;
};

auto get_starting_velocity(const SongEditor *song_editor_pointer) -> double {
  return get_reference(song_editor_pointer).song.starting_velocity;
};

auto get_starting_tempo(const SongEditor *song_editor_pointer) -> double {
  return get_reference(song_editor_pointer).song.starting_tempo;
};

auto get_current_file(const SongEditor *song_editor_pointer) -> QString {
  return get_reference(song_editor_pointer).current_file;
};

void set_gain(const SongEditor *song_editor_pointer, double new_value) {
  get_reference(song_editor_pointer).gain_editor.setValue(new_value);
};

void set_starting_key(const SongEditor *song_editor_pointer, double new_value) {
  get_reference(song_editor_pointer).starting_key_editor.setValue(new_value);
}

void set_starting_velocity(const SongEditor *song_editor_pointer,
                           double new_value) {
  get_reference(song_editor_pointer)
      .starting_velocity_editor.setValue(new_value);
}

void set_starting_tempo(const SongEditor *song_editor_pointer,
                        double new_value) {
  get_reference(song_editor_pointer).starting_tempo_editor.setValue(new_value);
}

auto create_editor(const QAbstractItemView &table_view,
                   QModelIndex index) -> QWidget & {
  auto &delegate = get_reference(table_view.itemDelegate());
  auto &viewport = get_reference(table_view.viewport());
  auto &cell_editor = get_reference(
      delegate.createEditor(&viewport, QStyleOptionViewItem(), index));
  delegate.setEditorData(&cell_editor, index);
  return cell_editor;
}

void set_editor(const QAbstractItemView &table_view, QWidget &cell_editor,
                QModelIndex index, const QVariant &new_value) {
  cell_editor.setProperty(
      get_const_reference(cell_editor.metaObject()).userProperty().name(),
      new_value);
  get_reference(table_view.itemDelegate())
      .setModelData(&cell_editor, table_view.model(), index);
}

void undo(const SongEditor *song_editor_pointer) {
  get_reference(song_editor_pointer).undo_stack.undo();
};

void trigger_insert_after(SongEditor *song_editor_pointer) {
  get_reference(song_editor_pointer).insert_after_action.trigger();
};
void trigger_insert_into(SongEditor *song_editor_pointer) {
  get_reference(song_editor_pointer).insert_into_action.trigger();
};
void trigger_delete(SongEditor *song_editor_pointer) {
  get_reference(song_editor_pointer).delete_action.trigger();
};
void trigger_remove_rows(SongEditor *song_editor_pointer) {
  get_reference(song_editor_pointer).remove_rows_action.trigger();
};
void trigger_cut(SongEditor *song_editor_pointer) {
  get_reference(song_editor_pointer).cut_action.trigger();
};
void trigger_copy(SongEditor *song_editor_pointer) {
  get_reference(song_editor_pointer).copy_action.trigger();
};
void trigger_paste_over(SongEditor *song_editor_pointer) {
  get_reference(song_editor_pointer).paste_over_action.trigger();
};
void trigger_paste_into(SongEditor *song_editor_pointer) {
  get_reference(song_editor_pointer).paste_into_action.trigger();
};
void trigger_paste_after(SongEditor *song_editor_pointer) {
  get_reference(song_editor_pointer).paste_after_action.trigger();
};
void trigger_save(SongEditor *song_editor_pointer) {
  get_reference(song_editor_pointer).save_action.trigger();
};

void trigger_play(SongEditor *song_editor_pointer) {
  get_reference(song_editor_pointer).play_action.trigger();
};

void trigger_stop_playing(SongEditor *song_editor_pointer) {
  get_reference(song_editor_pointer).stop_playing_action.trigger();
};

void open_file(SongEditor *song_editor_pointer, const QString &filename) {
  reference_open_file(get_reference(song_editor_pointer), filename);
};
void save_as_file(SongEditor *song_editor_pointer, const QString &filename) {
  reference_safe_as_file(get_reference(song_editor_pointer), filename);
};
void export_to_file(SongEditor *song_editor_pointer,
                    const QString &output_file) {
  auto &song_editor = get_reference(song_editor_pointer);
  export_song_to_file(song_editor.player, song_editor.song, output_file);
};

#include "justly.moc"
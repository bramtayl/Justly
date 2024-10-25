// TODO: add percussion tests

#include "song/SongEditor.hpp"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QAction>
#include <QBoxLayout>
#include <QByteArray>
#include <QClipboard>
#include <QCloseEvent>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGuiApplication>
#include <QHeaderView>
#include <QItemEditorFactory>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaType>
#include <QMimeData>
#include <QRect>
#include <QScreen>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QString>
#include <QTableView>
#include <QTextStream>
#include <QUndoStack>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

#include "chord/Chord.hpp"
#include "chord/ChordsModel.hpp"
#include "instrument/Instrument.hpp"
#include "instrument/InstrumentEditor.hpp"
#include "interval/Interval.hpp"
#include "interval/IntervalEditor.hpp"
#include "justly/ChordColumn.hpp"
#include "named/Named.hpp"
#include "note/Note.hpp"
#include "note/NotesModel.hpp"
#include "other/other.hpp"
#include "percussion/Percussion.hpp"
#include "percussion/PercussionsModel.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_instrument/PercussionInstrumentEditor.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "percussion_set/PercussionSetEditor.hpp"
#include "rational/Rational.hpp"
#include "rational/RationalEditor.hpp"
#include "rows/InsertRemoveRows.hpp"
#include "rows/InsertRow.hpp"
#include "rows/Row.hpp"
#include "rows/SetCells.hpp"
#include "song/EditChildrenOrBack.hpp"
#include "song/SetStartingDouble.hpp"

// starting control bounds
static const auto MAX_GAIN = 10;
static const auto MAX_VELOCITY = 127;
static const auto MIN_STARTING_KEY = 60;
static const auto MAX_STARTING_KEY = 440;
static const auto MIN_STARTING_TEMPO = 25;
static const auto MAX_STARTING_TEMPO = 200;

// mime types
static const auto CHORDS_CELLS_MIME = "application/prs.chords_cells+json";
static const auto NOTES_CELLS_MIME = "application/prs.notes_cells+json";
static const auto PERCUSSIONS_CELLS_MIME =
    "application/prs.percussions_cells+json";

// play settings
static const auto SECONDS_PER_MINUTE = 60;
static const unsigned int MILLISECONDS_PER_SECOND = 1000;
static const auto VERBOSE_FLUIDSYNTH = false;
static const auto NUMBER_OF_MIDI_CHANNELS = 16;
static const auto GAIN_STEP = 0.1;
static const auto BEND_PER_HALFSTEP = 4096;
static const auto ZERO_BEND_HALFSTEPS = 2;
static const unsigned int START_END_MILLISECONDS = 500;

// get json functions
[[nodiscard]] static auto get_json_value(const nlohmann::json &json_data,
                                         const char *field) -> nlohmann::json {
  Q_ASSERT(json_data.contains(field));
  return json_data[field];
}

[[nodiscard]] static auto get_json_double(const nlohmann::json &json_data,
                                          const char *field) -> double {
  const auto &json_value = get_json_value(json_data, field);
  Q_ASSERT(json_value.is_number());
  return json_value.get<double>();
}

[[nodiscard]] static auto get_json_int(const nlohmann::json &json_data,
                                       const char *field) -> int {
  Q_ASSERT(json_data.contains(field));
  const auto &json_value = get_json_value(json_data, field);
  Q_ASSERT(json_value.is_number());
  return json_value.get<int>();
}

[[nodiscard]] static auto
get_only_range(const QTableView &table_view) -> QItemSelectionRange {
  const auto *selection_model_pointer = table_view.selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);
  const auto &selection = selection_model_pointer->selection();
  Q_ASSERT(selection.size() == 1);
  return selection.at(0);
}

[[nodiscard]] static auto
get_number_of_rows(const QItemSelectionRange &range) -> int {
  Q_ASSERT(range.isValid());
  return range.bottom() - range.top() + 1;
}

[[nodiscard]] static auto get_settings_pointer() -> fluid_settings_t * {
  fluid_settings_t *settings_pointer = new_fluid_settings();
  Q_ASSERT(settings_pointer != nullptr);
  auto cores = std::thread::hardware_concurrency();
  if (cores > 0) {
    auto cores_result = fluid_settings_setint(
        settings_pointer, "synth.cpu-cores", static_cast<int>(cores));
    Q_ASSERT(cores_result == FLUID_OK);
  }
  if (VERBOSE_FLUIDSYNTH) {
    auto verbose_result =
        fluid_settings_setint(settings_pointer, "synth.verbose", 1);
    Q_ASSERT(verbose_result == FLUID_OK);
  }
  return settings_pointer;
}

[[nodiscard]] static auto get_beat_time(double tempo) -> double {
  return SECONDS_PER_MINUTE / tempo;
}

static void set_value_no_signals(QDoubleSpinBox &spin_box, double new_value) {
  spin_box.blockSignals(true);
  spin_box.setValue(new_value);
  spin_box.blockSignals(false);
}

template <typename Item>
[[nodiscard]] static auto copy_items(const QList<Item> &items,
                                     int first_row_number,
                                     int number_of_rows) -> QList<Item> {
  QList<Item> copied;
  std::copy(items.cbegin() + first_row_number,
            items.cbegin() + first_row_number + number_of_rows,
            std::back_inserter(copied));
  return copied;
}

static void copy_json(const nlohmann::json &copied, const QString &mime_type) {
  std::stringstream json_text;
  json_text << std::setw(4) << copied;

  auto *new_data_pointer = // NOLINT(cppcoreguidelines-owning-memory)
      new QMimeData;

  new_data_pointer->setData(mime_type, json_text.str().c_str());

  auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  clipboard_pointer->setMimeData(new_data_pointer);
}

[[nodiscard]] static auto
get_mime_description(const QString &mime_type) -> QString {
  if (mime_type == CHORDS_CELLS_MIME) {
    return SongEditor::tr("chords cells");
  }
  if (mime_type == NOTES_CELLS_MIME) {
    return SongEditor::tr("notes cells");
  }
  if (mime_type == PERCUSSIONS_CELLS_MIME) {
    return SongEditor::tr("percussions cells");
  }
  return mime_type;
}

auto SongEditor::parse_clipboard(
    const QString &mime_type,
    const nlohmann::json_schema::json_validator &validator) -> nlohmann::json {
  const auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  const auto *mime_data_pointer = clipboard_pointer->mimeData();
  Q_ASSERT(mime_data_pointer != nullptr);

  if (!mime_data_pointer->hasFormat(mime_type)) {
    auto formats = mime_data_pointer->formats();
    Q_ASSERT(!(formats.empty()));
    QString message;
    QTextStream stream(&message);
    stream << SongEditor::tr("Cannot paste ")
           << get_mime_description(formats[0])
           << SongEditor::tr(" into destination needing ")
           << get_mime_description(mime_type);
    QMessageBox::warning(this, SongEditor::tr("MIME type error"), message);
    return {};
  }
  const auto &copied_text = mime_data_pointer->data(mime_type).toStdString();
  nlohmann::json copied;
  try {
    copied = nlohmann::json::parse(copied_text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    QMessageBox::warning(this, SongEditor::tr("Parsing error"),
                         parse_error.what());
    return {};
  }
  if (copied.empty()) {
    QMessageBox::warning(this, SongEditor::tr("Empty paste"),
                         SongEditor::tr("Nothing to paste!"));
    return {};
  }
  try {
    validator.validate(copied);
  } catch (const std::exception &error) {
    QMessageBox::warning(this, SongEditor::tr("Schema error"), error.what());
    return {};
  }
  return copied;
}

SongEditor::SongEditor(QWidget *parent_pointer, Qt::WindowFlags flags)
    : QMainWindow(parent_pointer, flags),
      current_folder(
          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)),
      channel_schedules(QList<double>(NUMBER_OF_MIDI_CHANNELS, 0)),
      current_instrument_pointer(nullptr),
      current_percussion_set_pointer(nullptr),
      current_percussion_instrument_pointer(nullptr),
      undo_stack_pointer(new QUndoStack(this)),
      gain_editor_pointer(new QDoubleSpinBox(this)),
      starting_key_editor_pointer(new QDoubleSpinBox(this)),
      starting_velocity_editor_pointer(new QDoubleSpinBox(this)),
      starting_tempo_editor_pointer(new QDoubleSpinBox(this)),
      editing_chord_text_pointer(new QLabel("")),
      table_view_pointer(new QTableView(this)),
      chords_model_pointer(
          new ChordsModel(undo_stack_pointer, &chords, table_view_pointer)),
      notes_model_pointer(
          new NotesModel(chords_model_pointer, table_view_pointer)),
      percussions_model_pointer(
          new PercussionsModel(undo_stack_pointer, table_view_pointer)),
      back_to_chords_action_pointer(new QAction(tr("&Back to chords"), this)),
      insert_after_action_pointer(new QAction(tr("&After"), this)),
      insert_into_action_pointer(new QAction(tr("&Into start"), this)),
      delete_action_pointer(new QAction(tr("&Delete"), this)),
      remove_rows_action_pointer(new QAction(tr("&Remove rows"), this)),
      cut_action_pointer(new QAction(tr("&Cut"), this)),
      copy_action_pointer(new QAction(tr("&Copy"), this)),
      paste_over_action_pointer(new QAction(tr("&Over"), this)),
      paste_into_action_pointer(new QAction(tr("&Into start"), this)),
      paste_after_action_pointer(new QAction(tr("&After"), this)),
      play_action_pointer(new QAction(tr("&Play selection"), this)),
      stop_playing_action_pointer(new QAction(tr("&Stop playing"), this)),
      save_action_pointer(new QAction(tr("&Save"), this)),
      open_action_pointer(new QAction(tr("&Open"), this)),
      settings_pointer(get_settings_pointer()),
      event_pointer(new_fluid_event()),
      sequencer_pointer(new_fluid_sequencer2(0)),
      synth_pointer(new_fluid_synth(settings_pointer)),
      soundfont_id(get_soundfont_id(synth_pointer)),
      sequencer_id(fluid_sequencer_register_fluidsynth(sequencer_pointer,
                                                       synth_pointer)) {
  statusBar()->showMessage(tr(""));

  auto *factory_pointer = // NOLINT(cppcoreguidelines-owning-memory)
      new QItemEditorFactory;
  factory_pointer->registerEditor(
      qMetaTypeId<Rational>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          RationalEditor>);
  factory_pointer->registerEditor(
      qMetaTypeId<const PercussionInstrument *>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          PercussionInstrumentEditor>);
  factory_pointer->registerEditor(
      qMetaTypeId<const PercussionSet *>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          PercussionSetEditor>);
  factory_pointer->registerEditor(
      qMetaTypeId<const Instrument *>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          InstrumentEditor>);
  factory_pointer->registerEditor(
      qMetaTypeId<Interval>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          IntervalEditor>);
  factory_pointer->registerEditor(
      qMetaTypeId<QString>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          QLineEdit>);
  factory_pointer->registerEditor(
      qMetaTypeId<int>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          QSpinBox>);
  QItemEditorFactory::setDefaultFactory(factory_pointer);

  auto *controls_pointer = // NOLINT(cppcoreguidelines-owning-memory)
      new QFrame(this);
  controls_pointer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  auto *dock_widget_pointer = new QDockWidget(
      "Controls", this); // NOLINT(cppcoreguidelines-owning-memory)

  auto *menu_bar_pointer = menuBar();
  Q_ASSERT(menu_bar_pointer != nullptr);

  auto *file_menu_pointer = new QMenu(tr("&File"), this);

  file_menu_pointer->addAction(open_action_pointer);
  connect(open_action_pointer, &QAction::triggered, this, [this]() {
    if (undo_stack_pointer->isClean() || verify_discard_changes()) {
      auto *dialog_pointer = make_file_dialog(
          tr("Open — Justly"), "JSON file (*.json)", QFileDialog::AcceptOpen,
          ".json", QFileDialog::ExistingFile);
      if (dialog_pointer->exec() != 0) {
        open_file(get_selected_file(dialog_pointer));
      }
    }
  });
  open_action_pointer->setShortcuts(QKeySequence::Open);

  file_menu_pointer->addSeparator();

  save_action_pointer->setShortcuts(QKeySequence::Save);
  connect(save_action_pointer, &QAction::triggered, this,
          [this]() { save_as_file(current_file); });
  file_menu_pointer->addAction(save_action_pointer);
  save_action_pointer->setEnabled(false);

  auto *save_as_action_pointer =
      new QAction(tr("&Save As..."), file_menu_pointer);
  save_as_action_pointer->setShortcuts(QKeySequence::SaveAs);
  connect(save_as_action_pointer, &QAction::triggered, this, [this]() {
    auto *dialog_pointer = make_file_dialog(
        tr("Save As — Justly"), "JSON file (*.json)", QFileDialog::AcceptSave,
        ".json", QFileDialog::AnyFile);

    if (dialog_pointer->exec() != 0) {
      save_as_file(get_selected_file(dialog_pointer));
    }
  });
  file_menu_pointer->addAction(save_as_action_pointer);
  save_as_action_pointer->setEnabled(true);

  auto *export_action_pointer =
      new QAction(tr("&Export recording"), file_menu_pointer);
  connect(export_action_pointer, &QAction::triggered, this, [this]() {
    auto *dialog_pointer =
        make_file_dialog(tr("Export — Justly"), "WAV file (*.wav)",
                         QFileDialog::AcceptSave, ".wav", QFileDialog::AnyFile);
    dialog_pointer->setLabelText(QFileDialog::Accept, "Export");
    if (dialog_pointer->exec() != 0) {
      export_to_file(get_selected_file(dialog_pointer));
    }
  });
  file_menu_pointer->addAction(export_action_pointer);

  menu_bar_pointer->addMenu(file_menu_pointer);

  auto *edit_menu_pointer = new QMenu(tr("&Edit"), this);

  auto *undo_action_pointer =
      undo_stack_pointer->createUndoAction(edit_menu_pointer);
  undo_action_pointer->setShortcuts(QKeySequence::Undo);
  edit_menu_pointer->addAction(undo_action_pointer);

  auto *redo_action_pointer =
      undo_stack_pointer->createRedoAction(edit_menu_pointer);
  redo_action_pointer->setShortcuts(QKeySequence::Redo);
  edit_menu_pointer->addAction(redo_action_pointer);

  edit_menu_pointer->addSeparator();

  cut_action_pointer->setEnabled(false);
  cut_action_pointer->setShortcuts(QKeySequence::Cut);
  connect(cut_action_pointer, &QAction::triggered, this, [this]() {
    copy();
    delete_cells();
  });
  edit_menu_pointer->addAction(cut_action_pointer);

  copy_action_pointer->setEnabled(false);
  copy_action_pointer->setShortcuts(QKeySequence::Copy);
  connect(copy_action_pointer, &QAction::triggered, this, &SongEditor::copy);
  edit_menu_pointer->addAction(copy_action_pointer);

  auto *paste_menu_pointer = new QMenu(tr("&Paste"), edit_menu_pointer);
  edit_menu_pointer->addMenu(paste_menu_pointer);

  paste_over_action_pointer->setEnabled(false);
  connect(paste_over_action_pointer, &QAction::triggered, this, [this]() {
    auto first_row_number = get_only_range(*table_view_pointer).top();

    if (current_model_type == chords_type) {
      const auto json_chords_cells =
          parse_clipboard(CHORDS_CELLS_MIME, get_chords_cells_validator());
      if (json_chords_cells.empty()) {
        return;
      }

      Q_ASSERT(json_chords_cells.contains("chords"));
      const auto &json_chords = json_chords_cells["chords"];

      auto number_of_chords =
          std::min({static_cast<int>(json_chords.size()),
                    static_cast<int>(chords.size()) - first_row_number});

      QList<Chord> new_chords;
      partial_json_to_rows(new_chords, json_chords, number_of_chords);
      undo_stack_pointer->push(
          new SetCells<Chord>( // NOLINT(cppcoreguidelines-owning-memory)
              chords_model_pointer, first_row_number,
              get_json_int(json_chords_cells, "left_column"),
              get_json_int(json_chords_cells, "right_column"),
              copy_items(chords, first_row_number, number_of_chords),
              new_chords));
    } else if (current_model_type == notes_type) {
      auto *rows_pointer = notes_model_pointer->rows_pointer;
      Q_ASSERT(rows_pointer != nullptr);
      const auto json_notes_cells =
          parse_clipboard(NOTES_CELLS_MIME, get_notes_cells_validator());
      if (json_notes_cells.empty()) {
        return;
      }

      Q_ASSERT(json_notes_cells.contains("notes"));
      const auto &json_notes = json_notes_cells["notes"];

      auto number_of_notes =
          std::min({static_cast<int>(json_notes.size()),
                    static_cast<int>(rows_pointer->size()) - first_row_number});
      QList<Note> new_notes;
      partial_json_to_rows(new_notes, json_notes, number_of_notes);
      undo_stack_pointer->push(
          new SetCells<Note>( // NOLINT(cppcoreguidelines-owning-memory)
              notes_model_pointer, first_row_number,
              get_json_int(json_notes_cells, "left_column"),
              get_json_int(json_notes_cells, "right_column"),
              copy_items(*rows_pointer, first_row_number, number_of_notes),
              new_notes));
    } else {
      auto *rows_pointer = percussions_model_pointer->rows_pointer;
      Q_ASSERT(rows_pointer != nullptr);
      const auto json_percussions_cells = parse_clipboard(
          PERCUSSIONS_CELLS_MIME, get_percussions_cells_validator());
      if (json_percussions_cells.empty()) {
        return;
      }

      Q_ASSERT(json_percussions_cells.contains("percussions"));
      const auto &json_percussions = json_percussions_cells["percussions"];

      auto number_of_percussions =
          std::min({static_cast<int>(json_percussions.size()),
                    static_cast<int>(rows_pointer->size()) - first_row_number});

      QList<Percussion> new_percussions;
      partial_json_to_rows(new_percussions, json_percussions,
                           static_cast<int>(number_of_percussions));
      undo_stack_pointer->push(
          new SetCells<Percussion>( // NOLINT(cppcoreguidelines-owning-memory)
              percussions_model_pointer, first_row_number,
              get_json_int(json_percussions_cells, "left_column"),
              get_json_int(json_percussions_cells, "right_column"),
              copy_items((*rows_pointer), first_row_number,
                         static_cast<int>(number_of_percussions)),
              new_percussions));
    }
  });
  paste_over_action_pointer->setShortcuts(QKeySequence::Paste);
  paste_menu_pointer->addAction(paste_over_action_pointer);

  paste_into_action_pointer->setEnabled(true);
  connect(paste_into_action_pointer, &QAction::triggered, this,
          [this]() { paste_insert(0); });
  paste_menu_pointer->addAction(paste_into_action_pointer);

  paste_after_action_pointer->setEnabled(false);
  connect(paste_after_action_pointer, &QAction::triggered, this, [this]() {
    paste_insert(get_only_range(*table_view_pointer).bottom() + 1);
  });
  paste_menu_pointer->addAction(paste_after_action_pointer);

  edit_menu_pointer->addSeparator();

  auto *insert_menu_pointer = new QMenu(tr("&Insert"), edit_menu_pointer);

  edit_menu_pointer->addMenu(insert_menu_pointer);

  insert_after_action_pointer->setEnabled(false);
  insert_after_action_pointer->setShortcuts(QKeySequence::InsertLineSeparator);
  connect(insert_after_action_pointer, &QAction::triggered, this, [this]() {
    insert_row(get_only_range(*table_view_pointer).bottom() + 1);
  });
  insert_menu_pointer->addAction(insert_after_action_pointer);

  insert_into_action_pointer->setEnabled(true);
  insert_into_action_pointer->setShortcuts(QKeySequence::AddTab);
  connect(insert_into_action_pointer, &QAction::triggered, this,
          [this]() { insert_row(0); });
  insert_menu_pointer->addAction(insert_into_action_pointer);

  delete_action_pointer->setEnabled(false);
  delete_action_pointer->setShortcuts(QKeySequence::Delete);
  connect(delete_action_pointer, &QAction::triggered, this,
          &SongEditor::delete_cells);
  edit_menu_pointer->addAction(delete_action_pointer);

  remove_rows_action_pointer->setEnabled(false);
  remove_rows_action_pointer->setShortcuts(QKeySequence::DeleteStartOfWord);
  connect(remove_rows_action_pointer, &QAction::triggered, this, [this]() {
    const auto &range = get_only_range(*table_view_pointer);
    auto first_row_number = range.top();
    auto number_of_rows = get_number_of_rows(range);

    if (current_model_type == chords_type) {
      undo_stack_pointer->push(
          new InsertRemoveRows<Chord>( // NOLINT(cppcoreguidelines-owning-memory)
              chords_model_pointer, first_row_number,
              copy_items(chords, first_row_number, number_of_rows), true));
    } else if (current_model_type == notes_type) {
      auto *rows_pointer = notes_model_pointer->rows_pointer;
      Q_ASSERT(rows_pointer != nullptr);
      undo_stack_pointer->push(
          new InsertRemoveRows<Note>( // NOLINT(cppcoreguidelines-owning-memory)
              notes_model_pointer, first_row_number,
              copy_items(*rows_pointer, first_row_number, number_of_rows), true));
    } else {
      auto *rows_pointer = percussions_model_pointer->rows_pointer;
      Q_ASSERT(rows_pointer != nullptr);
      undo_stack_pointer->push(
          new InsertRemoveRows< // NOLINT(cppcoreguidelines-owning-memory)
              Percussion>(
              percussions_model_pointer, first_row_number,
              copy_items((*rows_pointer), first_row_number, number_of_rows), true));
    }
  });
  edit_menu_pointer->addAction(remove_rows_action_pointer);

  edit_menu_pointer->addSeparator();

  back_to_chords_action_pointer->setEnabled(false);
  back_to_chords_action_pointer->setShortcuts(QKeySequence::Back);
  connect(back_to_chords_action_pointer, &QAction::triggered, this,
          &SongEditor::back_to_chords);
  edit_menu_pointer->addAction(back_to_chords_action_pointer);

  menu_bar_pointer->addMenu(edit_menu_pointer);

  auto *view_menu_pointer = new QMenu(tr("&View"), this);

  auto *view_controls_checkbox_pointer =
      new QAction(tr("&Controls"), view_menu_pointer);

  view_controls_checkbox_pointer->setCheckable(true);
  view_controls_checkbox_pointer->setChecked(true);
  connect(view_controls_checkbox_pointer, &QAction::toggled,
          dock_widget_pointer, &QWidget::setVisible);
  view_menu_pointer->addAction(view_controls_checkbox_pointer);

  menu_bar_pointer->addMenu(view_menu_pointer);

  auto *play_menu_pointer = new QMenu(tr("&Play"), this);

  play_action_pointer->setEnabled(false);
  play_action_pointer->setShortcuts(QKeySequence::Print);
  connect(play_action_pointer, &QAction::triggered, this, [this]() {
    const auto &range = get_only_range(*table_view_pointer);
    auto first_row_number = range.top();
    auto number_of_rows = get_number_of_rows(range);

    stop_playing();
    initialize_play();
    if (current_model_type == chords_type) {
      if (first_row_number > 0) {
        for (auto chord_number = 0; chord_number < first_row_number;
             chord_number = chord_number + 1) {
          modulate(chords.at(chord_number));
        }
      }
      play_chords(first_row_number, number_of_rows);
    } else if (current_model_type == notes_type) {
      if (current_chord_number > 0) {
        for (auto chord_number = 0; chord_number < current_chord_number;
             chord_number = chord_number + 1) {
          modulate(chords.at(chord_number));
        }
      }
      const auto &chord = chords.at(current_chord_number);
      modulate(chord);
      play_notes(current_chord_number, chord, first_row_number, number_of_rows);
    } else {
      if (current_chord_number > 0) {
        for (auto chord_number = 0; chord_number < current_chord_number;
             chord_number = chord_number + 1) {
          modulate(chords.at(chord_number));
        }
      }
      const auto &chord = chords.at(current_chord_number);
      modulate(chord);
      play_percussions(current_chord_number, chord, first_row_number,
                       number_of_rows);
    }
  });
  play_menu_pointer->addAction(play_action_pointer);

  stop_playing_action_pointer->setEnabled(true);
  play_menu_pointer->addAction(stop_playing_action_pointer);
  connect(stop_playing_action_pointer, &QAction::triggered, this,
          &SongEditor::stop_playing);
  stop_playing_action_pointer->setShortcuts(QKeySequence::Cancel);

  menu_bar_pointer->addMenu(play_menu_pointer);

  auto *controls_form_pointer = // NOLINT(cppcoreguidelines-owning-memory)
      new QFormLayout(controls_pointer);

  gain_editor_pointer->setMinimum(0);
  gain_editor_pointer->setMaximum(MAX_GAIN);
  gain_editor_pointer->setSingleStep(GAIN_STEP);
  connect(gain_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &SongEditor::set_gain);
  set_starting_double_directly(gain_id, chords_model_pointer->gain);
  controls_form_pointer->addRow(tr("&Gain:"), gain_editor_pointer);

  starting_key_editor_pointer->setMinimum(MIN_STARTING_KEY);
  starting_key_editor_pointer->setMaximum(MAX_STARTING_KEY);
  starting_key_editor_pointer->setDecimals(1);
  starting_key_editor_pointer->setSuffix(" hz");

  starting_key_editor_pointer->setValue(
      this->chords_model_pointer->starting_key);

  connect(starting_key_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &SongEditor::set_starting_key);
  controls_form_pointer->addRow(tr("Starting &key:"),
                                starting_key_editor_pointer);

  starting_velocity_editor_pointer->setMinimum(0);
  starting_velocity_editor_pointer->setMaximum(MAX_VELOCITY);
  starting_velocity_editor_pointer->setDecimals(1);
  starting_velocity_editor_pointer->setValue(
      this->chords_model_pointer->starting_velocity);
  connect(starting_velocity_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &SongEditor::set_starting_velocity);
  controls_form_pointer->addRow(tr("Starting &velocity:"),
                                starting_velocity_editor_pointer);

  starting_tempo_editor_pointer->setMinimum(MIN_STARTING_TEMPO);
  starting_tempo_editor_pointer->setValue(
      this->chords_model_pointer->starting_tempo);
  starting_tempo_editor_pointer->setDecimals(1);
  starting_tempo_editor_pointer->setSuffix(" bpm");
  starting_tempo_editor_pointer->setMaximum(MAX_STARTING_TEMPO);

  connect_model(chords_model_pointer);
  connect_model(notes_model_pointer);
  connect_model(percussions_model_pointer);

  set_model(chords_model_pointer);
  connect(starting_tempo_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &SongEditor::set_starting_tempo);
  controls_form_pointer->addRow(tr("Starting &tempo:"),
                                starting_tempo_editor_pointer);

  dock_widget_pointer->setWidget(controls_pointer);
  dock_widget_pointer->setFeatures(QDockWidget::NoDockWidgetFeatures);
  addDockWidget(Qt::LeftDockWidgetArea, dock_widget_pointer);

  table_view_pointer->setSelectionMode(QAbstractItemView::ContiguousSelection);
  table_view_pointer->setSelectionBehavior(QAbstractItemView::SelectItems);
  table_view_pointer->setSizeAdjustPolicy(
      QAbstractScrollArea::AdjustToContentsOnFirstShow);

  auto *header_pointer = table_view_pointer->horizontalHeader();
  Q_ASSERT(header_pointer != nullptr);
  header_pointer->setSectionResizeMode(QHeaderView::ResizeToContents);

  table_view_pointer->setMouseTracking(true);

  connect(
      table_view_pointer, &QAbstractItemView::doubleClicked, this,
      [this](const QModelIndex &index) {
        if (current_model_type == chords_type) {
          auto row = index.row();
          auto column = index.column();
          if (column == chord_notes_column) {
            undo_stack_pointer->push(
                new EditChildrenOrBack( // NOLINT(cppcoreguidelines-owning-memory)
                    this, row, true, false));
          } else if (column == chord_percussions_column) {
            undo_stack_pointer->push(
                new EditChildrenOrBack( // NOLINT(cppcoreguidelines-owning-memory)
                    this, row, false, false));
          }
        }
      });

  setWindowTitle("Justly");

  editing_chord_text_pointer->setVisible(false);
  auto *table_column_pointer = // NOLINT(cppcoreguidelines-owning-memory)
      new QWidget(this);
  auto *table_column_layout_pointer = // NOLINT(cppcoreguidelines-owning-memory)
      new QVBoxLayout(table_column_pointer);
  table_column_layout_pointer->addWidget(editing_chord_text_pointer);
  table_column_layout_pointer->addWidget(table_view_pointer);
  setCentralWidget(table_column_pointer);

  connect(undo_stack_pointer, &QUndoStack::cleanChanged, this, [this]() {
    save_action_pointer->setEnabled(!undo_stack_pointer->isClean() &&
                                    !current_file.isEmpty());
  });

  const auto *primary_screen_pointer = QGuiApplication::primaryScreen();
  Q_ASSERT(primary_screen_pointer != nullptr);
  const auto full_size = primary_screen_pointer->availableGeometry();
  resize(full_size.width(), full_size.height());

  undo_stack_pointer->clear();
  undo_stack_pointer->setClean();

  fluid_event_set_dest(event_pointer, sequencer_id);

  start_real_time();
}

SongEditor::~SongEditor() {
  undo_stack_pointer->disconnect();

  delete_audio_driver();
  delete_fluid_event(event_pointer);
  delete_fluid_sequencer(sequencer_pointer);
  delete_fluid_synth(synth_pointer);
  delete_fluid_settings(settings_pointer);
}

void SongEditor::closeEvent(QCloseEvent *close_event_pointer) {
  if (!undo_stack_pointer->isClean() && !verify_discard_changes()) {
    close_event_pointer->ignore();
    return;
  }
  QMainWindow::closeEvent(close_event_pointer);
}

void SongEditor::update_actions() const {
  auto *selection_model_pointer = table_view_pointer->selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);
  auto anything_selected = !selection_model_pointer->selection().empty();

  cut_action_pointer->setEnabled(anything_selected);
  copy_action_pointer->setEnabled(anything_selected);
  paste_over_action_pointer->setEnabled(anything_selected);
  paste_after_action_pointer->setEnabled(anything_selected);
  insert_after_action_pointer->setEnabled(anything_selected);
  delete_action_pointer->setEnabled(anything_selected);
  remove_rows_action_pointer->setEnabled(anything_selected);
  play_action_pointer->setEnabled(anything_selected);
}

void SongEditor::connect_model(const QAbstractItemModel *model_pointer) const {
  SongEditor::connect(model_pointer, &QAbstractItemModel::rowsInserted, this,
                      &SongEditor::update_actions);
  SongEditor::connect(model_pointer, &QAbstractItemModel::rowsRemoved, this,
                      &SongEditor::update_actions);
}

void SongEditor::set_model(QAbstractItemModel *model_pointer) const {
  table_view_pointer->setModel(model_pointer);
  const auto *selection_model_pointer = table_view_pointer->selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);
  SongEditor::connect(selection_model_pointer,
                      &QItemSelectionModel::selectionChanged, this,
                      &SongEditor::update_actions);
  update_actions();
}

void SongEditor::is_chords_now(bool is_chords) const {
  back_to_chords_action_pointer->setEnabled(!is_chords);
  open_action_pointer->setEnabled(is_chords);
}

void SongEditor::edit_notes(int chord_number) {
  Q_ASSERT(current_model_type == chords_type);
  Q_ASSERT(notes_model_pointer->rows_pointer == nullptr);
  current_chord_number = chord_number;
  notes_model_pointer->begin_reset_model();
  notes_model_pointer->rows_pointer = &chords[chord_number].notes;
  notes_model_pointer->parent_chord_number = chord_number;
  notes_model_pointer->end_reset_model();
  current_model_type = notes_type;
  is_chords_now(false);

  QString label_text;
  QTextStream stream(&label_text);
  stream << SongEditor::tr("Editing notes for chord ") << chord_number + 1;
  editing_chord_text_pointer->setText(label_text);
  editing_chord_text_pointer->setVisible(true);

  set_model(notes_model_pointer);
}

void SongEditor::edit_percussions(int chord_number) {
  Q_ASSERT(current_model_type == chords_type);
  Q_ASSERT(percussions_model_pointer->rows_pointer == nullptr);
  current_chord_number = chord_number;
  percussions_model_pointer->begin_reset_model();
  percussions_model_pointer->rows_pointer = &chords[chord_number].percussions;
  percussions_model_pointer->end_reset_model();
  current_model_type = percussion_type;
  is_chords_now(false);
  Q_ASSERT(percussions_model_pointer->rows_pointer != nullptr);

  QString label_text;
  QTextStream stream(&label_text);
  stream << SongEditor::tr("Editing percussions for chord ")
         << chord_number + 1;
  editing_chord_text_pointer->setText(label_text);
  editing_chord_text_pointer->setVisible(true);

  set_model(percussions_model_pointer);
}

void SongEditor::back_to_chords_directly() {
  editing_chord_text_pointer->setVisible(false);
  set_model(chords_model_pointer);
  current_model_type = chords_type;
  current_chord_number = -1;
  is_chords_now(true);
}

void SongEditor::notes_to_chords() {
  back_to_chords_directly();
  notes_model_pointer->begin_reset_model();
  notes_model_pointer->rows_pointer = nullptr;
  notes_model_pointer->parent_chord_number = -1;
  notes_model_pointer->end_reset_model();
}

void SongEditor::percussions_to_chords() {
  back_to_chords_directly();
  percussions_model_pointer->begin_reset_model();
  percussions_model_pointer->rows_pointer = nullptr;
  percussions_model_pointer->end_reset_model();
}

void SongEditor::back_to_chords() {
  if (current_model_type == notes_type) {
    undo_stack_pointer->push(
        new EditChildrenOrBack( // NOLINT(cppcoreguidelines-owning-memory)
            this, current_chord_number, true, true));
  } else if (current_model_type == percussion_type) {
    undo_stack_pointer->push(
        new EditChildrenOrBack( // NOLINT(cppcoreguidelines-owning-memory)
            this, current_chord_number, false, true));
  } else {
    Q_ASSERT(false);
  }
}

void SongEditor::set_starting_double_directly(ControlId command_id,
                                              double new_value) const {
  switch (command_id) {
  case gain_id:
    set_value_no_signals(*gain_editor_pointer, new_value);
    chords_model_pointer->gain = new_value;
    fluid_synth_set_gain(synth_pointer, static_cast<float>(new_value));
    break;
  case starting_key_id:
    set_value_no_signals(*starting_key_editor_pointer, new_value);
    chords_model_pointer->starting_key = new_value;
    break;
  case starting_velocity_id:
    set_value_no_signals(*starting_velocity_editor_pointer, new_value);
    chords_model_pointer->starting_velocity = new_value;
    break;
  case starting_tempo_id:
    set_value_no_signals(*starting_tempo_editor_pointer, new_value);
    chords_model_pointer->starting_tempo = new_value;
  }
};

void SongEditor::set_gain(double new_value) {
  undo_stack_pointer->push(
      new SetStartingDouble( // NOLINT(cppcoreguidelines-owning-memory)
          this, gain_id, chords_model_pointer->gain, new_value));
}

void SongEditor::set_starting_key(double new_value) {
  undo_stack_pointer->push(
      new SetStartingDouble( // NOLINT(cppcoreguidelines-owning-memory)
          this, starting_key_id, this->chords_model_pointer->starting_key,
          new_value));
}

void SongEditor::set_starting_velocity(double new_value) {
  undo_stack_pointer->push(
      new SetStartingDouble( // NOLINT(cppcoreguidelines-owning-memory)
          this, starting_velocity_id,
          this->chords_model_pointer->starting_velocity, new_value));
}

void SongEditor::set_starting_tempo(double new_value) {
  undo_stack_pointer->push(
      new SetStartingDouble( // NOLINT(cppcoreguidelines-owning-memory)
          this, starting_tempo_id, this->chords_model_pointer->starting_tempo,
          new_value));
}

void SongEditor::insert_row(int row_number) const {
  if (current_model_type == chords_type) {
    undo_stack_pointer->push(
        new InsertRow<Chord>( // NOLINT(cppcoreguidelines-owning-memory)
            chords_model_pointer, row_number, Chord()));
  } else if (current_model_type == notes_type) {
    undo_stack_pointer->push(
        new InsertRow<Note>( // NOLINT(cppcoreguidelines-owning-memory)
            notes_model_pointer, row_number, Note()));
  } else {
    undo_stack_pointer->push(
        new InsertRow<Percussion>( // NOLINT(cppcoreguidelines-owning-memory)
            percussions_model_pointer, row_number, Percussion()));
  }
}

void SongEditor::paste_insert(int row_number) {
  if (current_model_type == chords_type) {
    const auto json_chords_cells =
        parse_clipboard(CHORDS_CELLS_MIME, get_chords_cells_validator());
    if (json_chords_cells.empty()) {
      return;
    }

    Q_ASSERT(json_chords_cells.contains("chords"));
    const auto &json_chords = json_chords_cells["chords"];

    QList<Chord> new_chords;
    json_to_rows(new_chords, json_chords);
    undo_stack_pointer->push(
        new InsertRemoveRows<Chord>( // NOLINT(cppcoreguidelines-owning-memory)
            chords_model_pointer, row_number, new_chords, false));
  } else if (current_model_type == notes_type) {
    auto *rows_pointer = notes_model_pointer->rows_pointer;
    Q_ASSERT(rows_pointer != nullptr);
    const auto json_notes_cells =
        parse_clipboard(NOTES_CELLS_MIME, get_notes_cells_validator());
    if (json_notes_cells.empty()) {
      return;
    }

    Q_ASSERT(json_notes_cells.contains("notes"));
    const auto &json_notes = json_notes_cells["notes"];

    QList<Note> new_notes;
    json_to_rows(new_notes, json_notes);
    undo_stack_pointer->push(
        new InsertRemoveRows<Note>( // NOLINT(cppcoreguidelines-owning-memory)
            notes_model_pointer, row_number, new_notes, false));
  } else {
    auto *rows_pointer = percussions_model_pointer->rows_pointer;
    Q_ASSERT(rows_pointer != nullptr);
    const auto json_percussions_cells = parse_clipboard(
        PERCUSSIONS_CELLS_MIME, get_percussions_cells_validator());
    if (json_percussions_cells.empty()) {
      return;
    }

    Q_ASSERT(json_percussions_cells.contains("percussions"));
    const auto &json_percussions = json_percussions_cells["percussions"];

    QList<Percussion> new_percussions;
    json_to_rows(new_percussions, json_percussions);
    undo_stack_pointer->push(
        new InsertRemoveRows<Percussion>( // NOLINT(cppcoreguidelines-owning-memory)
            percussions_model_pointer, row_number, new_percussions, false));
  }
}

void SongEditor::delete_cells() const {
  const auto &range = get_only_range(*table_view_pointer);
  auto first_row_number = range.top();
  auto number_of_rows = get_number_of_rows(range);
  auto left_column = range.left();
  auto right_column = range.right();

  if (current_model_type == chords_type) {
    undo_stack_pointer->push(
        new SetCells<Chord>( // NOLINT(cppcoreguidelines-owning-memory)
            chords_model_pointer, first_row_number, left_column, right_column,
            copy_items(chords, first_row_number, number_of_rows),
            QList<Chord>(number_of_rows)));
  } else if (current_model_type == notes_type) {
    auto *rows_pointer = notes_model_pointer->rows_pointer;
    Q_ASSERT(rows_pointer != nullptr);
    undo_stack_pointer->push(
        new SetCells<Note>( // NOLINT(cppcoreguidelines-owning-memory)
            notes_model_pointer, first_row_number, left_column, right_column,
            copy_items(*rows_pointer, first_row_number, number_of_rows),
            QList<Note>(number_of_rows)));
  } else {
    auto *rows_pointer = percussions_model_pointer->rows_pointer;
    Q_ASSERT(rows_pointer != nullptr);
    undo_stack_pointer->push(
        new SetCells<Percussion>( // NOLINT(cppcoreguidelines-owning-memory)
            percussions_model_pointer, first_row_number, left_column,
            right_column,
            copy_items((*rows_pointer), first_row_number, number_of_rows),
            QList<Percussion>(number_of_rows)));
  }
}

void SongEditor::copy() const {
  const auto &range = get_only_range(*table_view_pointer);
  auto first_row_number = range.top();
  auto number_of_rows = get_number_of_rows(range);
  auto left_column = range.left();
  auto right_column = range.right();

  if (current_model_type == chords_type) {
    auto left_chord_column = left_column;
    auto right_chord_column = right_column;
    copy_json(
        nlohmann::json(
            {{"left_column", left_chord_column},
             {"right_column", right_chord_column},
             {"chords", rows_to_json(chords, first_row_number, number_of_rows,
                                     left_chord_column, right_chord_column)}}),
        CHORDS_CELLS_MIME);
  } else if (current_model_type == notes_type) {
    auto *rows_pointer = notes_model_pointer->rows_pointer;
    Q_ASSERT(rows_pointer != nullptr);
    auto left_note_column = left_column;
    auto right_note_column = right_column;
    copy_json(
        nlohmann::json({{"left_column", left_note_column},
                        {"right_column", right_note_column},
                        {"notes", rows_to_json(*rows_pointer, first_row_number,
                                               number_of_rows, left_note_column,
                                               right_note_column)}}),
        NOTES_CELLS_MIME);
  } else {
    auto *rows_pointer = percussions_model_pointer->rows_pointer;
    Q_ASSERT(rows_pointer != nullptr);
    auto left_percussion_column = left_column;
    auto right_percussion_column = right_column;
    copy_json(
        nlohmann::json(
            {{"left_column", left_percussion_column},
             {"right_column", right_percussion_column},
             {"percussions",
              rows_to_json((*rows_pointer), first_row_number, number_of_rows,
                           left_percussion_column, right_percussion_column)}}),
        PERCUSSIONS_CELLS_MIME);
  }
}

void SongEditor::send_event_at(double time) const {
  Q_ASSERT(time >= 0);
  auto result =
      fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                              static_cast<unsigned int>(std::round(time)), 1);
  Q_ASSERT(result == FLUID_OK);
};

void SongEditor::start_real_time() {
  auto default_driver_pointer = std::make_unique<char *>();
  fluid_settings_dupstr(settings_pointer, "audio.driver",
                        default_driver_pointer.get());
  Q_ASSERT(default_driver_pointer != nullptr);

  delete_audio_driver();

  QString default_driver(*default_driver_pointer);
#ifdef __linux__
  default_driver = "pulseaudio";
#endif
  fluid_settings_setstr(settings_pointer, "audio.driver",
                        default_driver.toStdString().c_str());

#ifndef NO_REALTIME_AUDIO
  audio_driver_pointer =
      new_fluid_audio_driver(settings_pointer, synth_pointer);
  if (audio_driver_pointer == nullptr) {
    QString message;
    QTextStream stream(&message);
    stream << SongEditor::tr("Cannot start audio driver \"") << default_driver
           << SongEditor::tr("\"");
    QMessageBox::warning(this, SongEditor::tr("Audio driver error"), message);
  }
#endif
}

void SongEditor::initialize_play() {
  current_instrument_pointer = nullptr;
  current_percussion_set_pointer = nullptr;
  current_percussion_instrument_pointer = nullptr;
  current_key = chords_model_pointer->starting_key;
  current_velocity = chords_model_pointer->starting_velocity;
  current_tempo = chords_model_pointer->starting_tempo;
  current_time = fluid_sequencer_get_tick(sequencer_pointer);
  for (auto index = 0; index < NUMBER_OF_MIDI_CHANNELS; index = index + 1) {
    Q_ASSERT(index < channel_schedules.size());
    channel_schedules[index] = 0;
  }
}

auto SongEditor::get_open_channel_number(
    int chord_number, int item_number, const QString &item_description) -> int {
  for (auto channel_index = 0; channel_index < NUMBER_OF_MIDI_CHANNELS;
       channel_index = channel_index + 1) {
    if (current_time >= channel_schedules.at(channel_index)) {
      return channel_index;
    }
  }
  QString message;
  QTextStream stream(&message);
  stream << SongEditor::tr("Out of MIDI channels for chord ")
         << chord_number + 1 << SongEditor::tr(", ") << item_description << " "
         << item_number + 1 << SongEditor::tr(". Not playing ")
         << item_description << ".";
  QMessageBox::warning(this, SongEditor::tr("MIDI channel error"), message);
  return -1;
}

void SongEditor::change_instrument(int channel_number, short bank_number,
                                   short preset_number) const {
  fluid_event_program_select(event_pointer, channel_number, soundfont_id,
                             bank_number, preset_number);
  send_event_at(current_time);
}

void SongEditor::update_final_time(double new_final_time) {
  if (new_final_time > final_time) {
    final_time = new_final_time;
  }
};

void SongEditor::modulate(const Chord &chord) {
  current_key = current_key * interval_to_double(chord.interval);
  current_velocity =
      current_velocity * rational_to_double(chord.velocity_ratio);
  current_tempo = current_tempo * rational_to_double(chord.tempo_ratio);
  const auto *chord_instrument_pointer = chord.instrument_pointer;
  if (chord_instrument_pointer != nullptr) {
    current_instrument_pointer = chord_instrument_pointer;
  }

  const auto *chord_percussion_set_pointer = chord.percussion_set_pointer;
  if (chord_percussion_set_pointer == nullptr) {
    current_percussion_set_pointer = chord_percussion_set_pointer;
  }

  const auto *chord_percussion_instrument_pointer =
      chord.percussion_instrument_pointer;
  if (chord_percussion_instrument_pointer != nullptr) {
    current_percussion_instrument_pointer = chord_percussion_instrument_pointer;
  }
}

void SongEditor::play_note_or_percussion(int channel_number, short midi_number,
                                         const Rational &beats,
                                         const Rational &velocity_ratio,
                                         int time_offset, int chord_number,
                                         int item_number,
                                         const QString &item_description) {
  auto velocity = current_velocity * rational_to_double(velocity_ratio);
  short new_velocity = 1;
  if (velocity > MAX_VELOCITY) {
    QString message;
    QTextStream stream(&message);
    stream << SongEditor::tr("Velocity ") << velocity
           << SongEditor::tr(" exceeds ") << MAX_VELOCITY
           << SongEditor::tr(" for chord ") << chord_number + 1
           << SongEditor::tr(", ") << item_description << " " << item_number + 1
           << SongEditor::tr(". Playing with velocity ") << MAX_VELOCITY
           << SongEditor::tr(".");
    QMessageBox::warning(this, SongEditor::tr("Velocity error"), message);
  } else {
    new_velocity = static_cast<short>(std::round(velocity));
  }
  fluid_event_noteon(event_pointer, channel_number, midi_number, new_velocity);
  send_event_at(current_time + time_offset);

  auto end_time = current_time + get_beat_time(current_tempo) *
                                     rational_to_double(beats) *
                                     MILLISECONDS_PER_SECOND;

  fluid_event_noteoff(event_pointer, channel_number, midi_number);
  send_event_at(end_time);
  Q_ASSERT(channel_number < channel_schedules.size());
  channel_schedules[channel_number] = end_time;
  update_final_time(end_time);
}

void SongEditor::play_notes(int chord_number, const Chord &chord,
                            int first_note_index, int number_of_notes) {
  for (auto note_index = first_note_index;
       note_index < first_note_index + number_of_notes;
       note_index = note_index + 1) {
    auto channel_number = get_open_channel_number(chord_number, note_index,
                                                  SongEditor::tr("note"));
    if (channel_number != -1) {
      const auto &note = chord.notes.at(note_index);

      const auto *instrument_pointer = note.instrument_pointer;
      if (instrument_pointer == nullptr) {
        instrument_pointer = current_instrument_pointer;
      };
      if (instrument_pointer == nullptr) {
        QString message;
        QTextStream stream(&message);
        stream << SongEditor::tr("No instrument for chord ") << chord_number + 1
               << SongEditor::tr(", note ") << note_index + 1
               << SongEditor::tr(". Using Marimba.");
        QMessageBox::warning(this, SongEditor::tr("Instrument error"), message);
        instrument_pointer = &get_by_name(get_all_instruments(), "Marimba");
      }
      qInfo() << instrument_pointer->name;
      change_instrument(channel_number, instrument_pointer->bank_number,
                        instrument_pointer->preset_number);

      auto midi_float =
          get_midi(current_key * interval_to_double(note.interval));
      auto closest_midi = static_cast<short>(round(midi_float));

      fluid_event_pitch_bend(event_pointer, channel_number,
                             static_cast<int>(round((midi_float - closest_midi +
                                                     ZERO_BEND_HALFSTEPS) *
                                                    BEND_PER_HALFSTEP)));
      send_event_at(current_time + 1);

      play_note_or_percussion(channel_number, closest_midi, note.beats,
                              note.velocity_ratio, 2, chord_number, note_index,
                              SongEditor::tr("note"));
    }
  }
}

void SongEditor::play_percussions(int chord_number, const Chord &chord,
                                  int first_percussion_number,
                                  int number_of_percussions) {
  for (auto percussion_number = first_percussion_number;
       percussion_number < first_percussion_number + number_of_percussions;
       percussion_number = percussion_number + 1) {
    auto channel_number = get_open_channel_number(
        chord_number, percussion_number, SongEditor::tr("percussion"));
    if (channel_number != -1) {
      const auto &percussion = chord.percussions.at(percussion_number);

      const auto *percussion_set_pointer = percussion.percussion_set_pointer;
      if (percussion_set_pointer == nullptr) {
        percussion_set_pointer = chord.percussion_set_pointer;
      };
      if (percussion_set_pointer == nullptr) {
        QString message;
        QTextStream stream(&message);
        stream << SongEditor::tr("No percussion set for chord ")
               << chord_number + 1 << SongEditor::tr(", percussion ")
               << percussion_number + 1 << SongEditor::tr(". Using Standard.");
        QMessageBox::warning(this, SongEditor::tr("Percussion set error"),
                             message);
        percussion_set_pointer =
            &get_by_name(get_all_percussion_sets(), "Standard");
      }
      const auto &percussion_set = *percussion_set_pointer;
      change_instrument(channel_number, percussion_set.bank_number,
                        percussion_set.preset_number);

      const auto *percussion_instrument_pointer =
          percussion.percussion_instrument_pointer;
      if (percussion_instrument_pointer == nullptr) {
        percussion_instrument_pointer = current_percussion_instrument_pointer;
      };
      if (percussion_instrument_pointer == nullptr) {
        QString message;
        QTextStream stream(&message);
        stream << SongEditor::tr("No percussion instrument for chord ")
               << chord_number + 1 << SongEditor::tr(", percussion ")
               << percussion_number + 1
               << SongEditor::tr(". Using Tambourine.");
        QMessageBox::warning(
            this, SongEditor::tr("Percussion instrument error"), message);
        percussion_instrument_pointer =
            &get_by_name(get_all_percussion_instruments(), "Tambourine");
      }

      play_note_or_percussion(
          channel_number, percussion_instrument_pointer->midi_number,
          percussion.beats, percussion.velocity_ratio, 1, chord_number,
          percussion_number, SongEditor::tr("percussion"));
    }
  }
}

void SongEditor::play_chords(int first_chord_number, int number_of_chords,
                             int wait_frames) {
  auto start_time = current_time + wait_frames;
  current_time = start_time;
  update_final_time(start_time);
  for (auto chord_number = first_chord_number;
       chord_number < first_chord_number + number_of_chords;
       chord_number = chord_number + 1) {
    const auto &chord = chords.at(chord_number);

    modulate(chord);
    play_notes(chord_number, chord, 0, static_cast<int>(chord.notes.size()));
    play_percussions(chord_number, chord, 0,
                     static_cast<int>(chord.percussions.size()));
    auto new_current_time = current_time + (get_beat_time(current_tempo) *
                                            rational_to_double(chord.beats)) *
                                               MILLISECONDS_PER_SECOND;
    current_time = new_current_time;
    update_final_time(new_current_time);
  }
}

void SongEditor::stop_playing() const {
  fluid_sequencer_remove_events(sequencer_pointer, -1, -1, -1);

  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       channel_number = channel_number + 1) {
    fluid_event_all_sounds_off(event_pointer, channel_number);
    fluid_sequencer_send_now(sequencer_pointer, event_pointer);
  }
}

void SongEditor::delete_audio_driver() {
  if (audio_driver_pointer != nullptr) {
    delete_fluid_audio_driver(audio_driver_pointer);
    audio_driver_pointer = nullptr;
  }
}

auto SongEditor::verify_discard_changes() -> bool {
  return QMessageBox::question(this, SongEditor::tr("Unsaved changes"),
                               SongEditor::tr("Discard unsaved changes?")) ==
         QMessageBox::Yes;
}

auto SongEditor::make_file_dialog(
    const QString &caption, const QString &filter,
    QFileDialog::AcceptMode accept_mode, const QString &suffix,
    QFileDialog::FileMode file_mode) -> QFileDialog * {
  auto *dialog_pointer = // NOLINT(cppcoreguidelines-owning-memory)
      new QFileDialog(this, caption, current_folder, filter);

  dialog_pointer->setAcceptMode(accept_mode);
  dialog_pointer->setDefaultSuffix(suffix);
  dialog_pointer->setFileMode(file_mode);

  return dialog_pointer;
}

auto SongEditor::get_selected_file(QFileDialog *dialog_pointer) -> QString {
  current_folder = dialog_pointer->directory().absolutePath();
  const auto &selected_files = dialog_pointer->selectedFiles();
  Q_ASSERT(!(selected_files.empty()));
  return selected_files[0];
}

void SongEditor::open_file(const QString &filename) {
  std::ifstream file_io(filename.toStdString().c_str());
  nlohmann::json json_song;
  try {
    json_song = nlohmann::json::parse(file_io);
  } catch (const nlohmann::json::parse_error &parse_error) {
    QMessageBox::warning(this, SongEditor::tr("Parsing error"),
                         parse_error.what());
    return;
  }
  file_io.close();

  static const nlohmann::json_schema::json_validator song_validator =
      make_validator(
          "Song",
          nlohmann::json(
              {{"description", "A Justly song in JSON format"},
               {"type", "object"},
               {"required", nlohmann::json({
                                "starting_key",
                                "starting_tempo",
                                "starting_velocity",
                            })},
               {"properties",
                nlohmann::json(
                    {{"gain", nlohmann::json(
                                  {{"type", "number"},
                                   {"description", "the gain (speaker volume)"},
                                   {"minimum", 0},
                                   {"maximum", MAX_GAIN}})},
                     {"starting_key",
                      nlohmann::json(
                          {{"type", "number"},
                           {"description", "the starting key, in Hz"},
                           {"minimum", MIN_STARTING_KEY},
                           {"maximum", MAX_STARTING_KEY}})},
                     {"starting_tempo",
                      nlohmann::json(
                          {{"type", "number"},
                           {"description", "the starting tempo, in bpm"},
                           {"minimum", MIN_STARTING_TEMPO},
                           {"maximum", MAX_STARTING_TEMPO}})},
                     {"starting_velocity",
                      nlohmann::json({{"type", "number"},
                                      {"description",
                                       "the starting velocity (note force)"},
                                      {"minimum", 0},
                                      {"maximum", MAX_VELOCITY}})},
                     {"chords", get_chords_schema()}})}}));
  try {
    song_validator.validate(json_song);
  } catch (const std::exception &error) {
    QMessageBox::warning(this, SongEditor::tr("Schema error"), error.what());
    return;
  }

  if (json_song.contains("gain")) {
    gain_editor_pointer->setValue(get_json_double(json_song, "gain"));
  }

  if (json_song.contains("starting_key")) {
    starting_key_editor_pointer->setValue(
        get_json_double(json_song, "starting_key"));
  }

  if (json_song.contains("starting_velocity")) {
    starting_velocity_editor_pointer->setValue(
        get_json_double(json_song, "starting_velocity"));
  }

  if (json_song.contains("starting_tempo")) {
    starting_tempo_editor_pointer->setValue(
        get_json_double(json_song, "starting_tempo"));
  }

  if (!chords.empty()) {
    Q_ASSERT(chords_model_pointer != nullptr);
    chords_model_pointer->remove_rows(0, static_cast<int>(chords.size()));
  }

  if (json_song.contains("chords")) {
    const auto &json_chords = json_song["chords"];

    Q_ASSERT(chords_model_pointer != nullptr);

    chords_model_pointer->begin_insert_rows(
        static_cast<int>(chords.size()), static_cast<int>(json_chords.size()));
    json_to_rows(chords, json_chords);
    chords_model_pointer->end_insert_rows();
  }

  current_file = filename;

  undo_stack_pointer->clear();
  undo_stack_pointer->setClean();
}

void SongEditor::save_as_file(const QString &filename) {
  std::ofstream file_io(filename.toStdString().c_str());

  nlohmann::json json_song;
  json_song["gain"] = chords_model_pointer->gain;
  json_song["starting_key"] = chords_model_pointer->starting_key;
  json_song["starting_tempo"] = chords_model_pointer->starting_tempo;
  json_song["starting_velocity"] = chords_model_pointer->starting_velocity;

  if (!chords.empty()) {
    json_song["chords"] =
        rows_to_json(chords, 0, static_cast<int>(chords.size()),
                     chord_instrument_column, chord_percussions_column);
  }

  file_io << std::setw(4) << json_song;
  file_io.close();
  current_file = filename;

  undo_stack_pointer->setClean();
}

void SongEditor::export_to_file(const QString &output_file) {
  stop_playing();

  delete_audio_driver();
  auto file_result = fluid_settings_setstr(settings_pointer, "audio.file.name",
                                           output_file.toStdString().c_str());
  Q_ASSERT(file_result == FLUID_OK);

  auto unlock_result =
      fluid_settings_setint(settings_pointer, "synth.lock-memory", 0);
  Q_ASSERT(unlock_result == FLUID_OK);

  auto finished = false;
  auto finished_timer_id = fluid_sequencer_register_client(
      sequencer_pointer, "finished timer",
      [](unsigned int /*time*/, fluid_event_t * /*event*/,
         fluid_sequencer_t * /*seq*/, void *data_pointer) -> void {
        auto *finished_pointer = static_cast<bool *>(data_pointer);
        Q_ASSERT(finished_pointer != nullptr);
        *finished_pointer = true;
      },
      &finished);
  Q_ASSERT(finished_timer_id >= 0);

  initialize_play();
  play_chords(0, static_cast<int>(chords.size()), START_END_MILLISECONDS);

  fluid_event_set_dest(event_pointer, finished_timer_id);
  fluid_event_timer(event_pointer, nullptr);
  send_event_at(final_time + START_END_MILLISECONDS);

  auto *renderer_pointer = new_fluid_file_renderer(synth_pointer);
  Q_ASSERT(renderer_pointer != nullptr);
  while (!finished) {
    auto process_result = fluid_file_renderer_process_block(renderer_pointer);
    Q_ASSERT(process_result == FLUID_OK);
  }
  delete_fluid_file_renderer(renderer_pointer);

  fluid_event_set_dest(event_pointer, sequencer_id);
  auto lock_result =
      fluid_settings_setint(settings_pointer, "synth.lock-memory", 1);
  Q_ASSERT(lock_result == FLUID_OK);
  start_real_time();
}

// TODO: add percussion tests

#include "song/SongEditor.hpp"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAction>
#include <QByteArray>
#include <QClipboard>
#include <QCloseEvent>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QRect>
#include <QScreen>
#include <QSize>
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
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include "chord/Chord.hpp"
#include "chord/ChordsModel.hpp"
#include "chord/InsertChord.hpp"
#include "chord/RemoveChords.hpp"
#include "chord/SetChordsCells.hpp"
#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "justly/ChordColumn.hpp"
#include "note/InsertNote.hpp"
#include "note/Note.hpp"
#include "note/NotesModel.hpp"
#include "note/RemoveNotes.hpp"
#include "note/SetNotesCells.hpp"
#include "other/JustlyView.hpp"
#include "other/other.hpp"
#include "percussion/InsertPercussion.hpp"
#include "percussion/Percussion.hpp"
#include "percussion/PercussionsModel.hpp"
#include "percussion/RemovePercussions.hpp"
#include "percussion/SetPercussionsCells.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "rational/Rational.hpp"
#include "song/EditNotes.hpp"
#include "song/EditPercussions.hpp"
#include "song/NotesToChords.hpp"
#include "song/PercussionsToChords.hpp"
#include "song/SetGain.hpp"
#include "song/SetStartingKey.hpp"
#include "song/SetStartingTempo.hpp"
#include "song/SetStartingVelocity.hpp"

// starting control bounds
static const auto MAX_GAIN = 10;
static const auto MAX_VELOCITY = 127;
static const auto MIN_STARTING_KEY = 60;
static const auto MAX_STARTING_KEY = 440;
static const auto MIN_STARTING_TEMPO = 25;
static const auto MAX_STARTING_TEMPO = 200;

// mime types
static const auto CHORDS_MIME = "application/prs.chords+json";
static const auto NOTES_MIME = "application/prs.notes+json";
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

[[nodiscard]] static auto get_only_range(const QTableView *table_view_pointer)
    -> const QItemSelectionRange & {
  const auto *selection_model_pointer = table_view_pointer->selectionModel();
  Q_ASSERT(selection_model_pointer != nullptr);
  const auto &selection = selection_model_pointer->selection();
  Q_ASSERT(selection.size() == 1);
  return selection.at(0);
}

[[nodiscard]] static auto
get_first_child_number(const QItemSelectionRange &range) -> qsizetype {
  return to_qsizetype(range.top());
}

[[nodiscard]] static auto
get_number_of_children(const QItemSelectionRange &range) -> qsizetype {
  return to_qsizetype(range.bottom() - range.top() + 1);
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

static void set_value_no_signals(QDoubleSpinBox *spin_box_pointer,
                                 double new_value) {
  spin_box_pointer->blockSignals(true);
  spin_box_pointer->setValue(new_value);
  spin_box_pointer->blockSignals(false);
}

template <typename Item>
[[nodiscard]] static auto
copy_items(const QList<Item> &items, qsizetype first_child_number,
           qsizetype number_of_children) -> QList<Item> {
  QList<Item> copied;
  std::copy(items.cbegin() + first_child_number,
            items.cbegin() + first_child_number + number_of_children,
            std::back_inserter(copied));
  return copied;
}

static void copy_json(const nlohmann::json &copied, const QString &mime_type) {
  std::stringstream json_text;
  json_text << std::setw(4) << copied;

  auto *new_data_pointer = std::make_unique<QMimeData>().release();

  new_data_pointer->setData(mime_type, json_text.str().c_str());

  auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  clipboard_pointer->setMimeData(new_data_pointer);
}

[[nodiscard]] static auto
get_mime_description(const QString &mime_type) -> QString {
  if (mime_type == CHORDS_MIME) {
    return ChordsModel::tr("chords");
  }
  if (mime_type == NOTES_MIME) {
    return ChordsModel::tr("notes");
  }
  if (mime_type == CHORDS_CELLS_MIME) {
    return ChordsModel::tr("chords cells");
  }
  if (mime_type == NOTES_CELLS_MIME) {
    return ChordsModel::tr("notes cells");
  }
  if (mime_type == PERCUSSIONS_CELLS_MIME) {
    return ChordsModel::tr("percussions cells");
  }
  return mime_type;
}

[[nodiscard]] static auto parse_clipboard(
    QWidget *parent_pointer, const QString &mime_type,
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
    stream << ChordsModel::tr("Cannot paste ")
           << get_mime_description(formats[0])
           << ChordsModel::tr(" into destination needing ")
           << get_mime_description(mime_type);
    QMessageBox::warning(parent_pointer, ChordsModel::tr("MIME type error"),
                         message);
    return {};
  }
  const auto &copied_text = mime_data_pointer->data(mime_type).toStdString();
  nlohmann::json copied;
  try {
    copied = nlohmann::json::parse(copied_text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    QMessageBox::warning(parent_pointer, ChordsModel::tr("Parsing error"),
                         parse_error.what());
    return {};
  }
  if (copied.empty()) {
    QMessageBox::warning(parent_pointer, ChordsModel::tr("Empty paste"),
                         "Nothing to paste!");
    return {};
  }
  try {
    validator.validate(copied);
  } catch (const std::exception &error) {
    QMessageBox::warning(parent_pointer, ChordsModel::tr("Schema error"),
                         error.what());
    return {};
  }
  return copied;
}

[[nodiscard]] static auto make_validator(const char *title, nlohmann::json json)
    -> nlohmann::json_schema::json_validator {
  json["$schema"] = "http://json-schema.org/draft-07/schema#";
  json["title"] = title;
  return {json};
}

SongEditor::SongEditor(QWidget *parent_pointer, Qt::WindowFlags flags)
    : QMainWindow(parent_pointer, flags),
      current_folder(
          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)),
      channel_schedules(QList<double>(NUMBER_OF_MIDI_CHANNELS, 0)),
      undo_stack_pointer(new QUndoStack(this)),
      gain_editor_pointer(new QDoubleSpinBox(this)),
      starting_key_editor_pointer(new QDoubleSpinBox(this)),
      starting_velocity_editor_pointer(new QDoubleSpinBox(this)),
      starting_tempo_editor_pointer(new QDoubleSpinBox(this)),
      table_view_pointer(new JustlyView(this)),
      chords_model_pointer(new ChordsModel(undo_stack_pointer, this)),
      notes_model_pointer(new NotesModel(chords_model_pointer, this)),
      percussions_model_pointer(new PercussionsModel(undo_stack_pointer, this)),
      back_to_chords_action_pointer(new QAction(tr("&Back to chords"), this)),
      insert_after_action_pointer(new QAction(tr("&After"), this)),
      insert_into_action_pointer(new QAction(tr("&Into start"), this)),
      delete_action_pointer(new QAction(tr("&Delete"), this)),
      remove_rows_action_pointer(new QAction(tr("&Remove rows"), this)),
      cut_action_pointer(new QAction(tr("&Cut"), this)),
      copy_action_pointer(new QAction(tr("&Copy"), this)),
      paste_action_pointer(new QAction(tr("&Paste"), this)),
      play_action_pointer(new QAction(tr("&Play selection"), this)),
      stop_playing_action_pointer(new QAction(tr("&Stop playing"), this)),
      save_action_pointer(new QAction(tr("&Save"), this)),
      settings_pointer(get_settings_pointer()),
      event_pointer(new_fluid_event()),
      sequencer_pointer(new_fluid_sequencer2(0)),
      synth_pointer(new_fluid_synth(settings_pointer)),
      soundfont_id(get_soundfont_id(synth_pointer)),
      sequencer_id(fluid_sequencer_register_fluidsynth(sequencer_pointer,
                                                       synth_pointer)) {
  statusBar()->showMessage(tr(""));

  auto *controls_pointer = std::make_unique<QFrame>(this).release();
  controls_pointer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QDockWidget *dock_widget_pointer =
      std::make_unique<QDockWidget>("Controls", this).release();

  auto *menu_bar_pointer = menuBar();
  Q_ASSERT(menu_bar_pointer != nullptr);

  auto *file_menu_pointer =
      std::make_unique<QMenu>(tr("&File"), this).release();

  auto *open_action_pointer =
      std::make_unique<QAction>(tr("&Open"), file_menu_pointer).release();
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
      std::make_unique<QAction>(tr("&Save As..."), file_menu_pointer).release();
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
      std::make_unique<QAction>(tr("&Export recording"), file_menu_pointer)
          .release();
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

  auto *edit_menu_pointer =
      std::make_unique<QMenu>(tr("&Edit"), this).release();

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

  paste_action_pointer->setEnabled(false);
  connect(paste_action_pointer, &QAction::triggered, this, [this]() {
    auto first_child_number =
        get_first_child_number(get_only_range(table_view_pointer));

    if (current_model_type == chords_type) {
      const auto &chords = chords_model_pointer->chords;
      static const nlohmann::json_schema::json_validator
          chords_cells_validator = make_validator(
              "Chords cells",
              nlohmann::json(
                  {{"description", "chords cells"},
                   {"type", "object"},
                   {"required", {"left_column", "right_column", "chords"}},
                   {"properties",
                    nlohmann::json({{"left_column", get_chord_column_schema(
                                                        "left ChordColumn")},
                                    {"right_column", get_chord_column_schema(
                                                         "right ChordColumn")},
                                    {"chords", get_chords_schema()}})}}));
      const auto json_chords_cells =
          parse_clipboard(chords_model_pointer->parent_pointer,
                          CHORDS_CELLS_MIME, chords_cells_validator);
      if (json_chords_cells.empty()) {
        return;
      }

      Q_ASSERT(json_chords_cells.contains("chords"));
      const auto &json_chords = json_chords_cells["chords"];

      auto number_of_chords =
          std::min({static_cast<qsizetype>(json_chords.size()),
                    chords.size() - first_child_number});

      QList<Chord> new_chords;
      json_to_chords(new_chords, json_chords, number_of_chords);
      undo_stack_pointer->push(
          std::make_unique<SetChordsCells>(
              chords_model_pointer, first_child_number,
              to_chord_column(get_json_int(json_chords_cells, "left_column")),
              to_chord_column(get_json_int(json_chords_cells, "right_column")),
              copy_items(chords, first_child_number, number_of_chords),
              std::move(new_chords))
              .release());
    } else if (current_model_type == notes_type) {
      auto *notes_pointer = notes_model_pointer->notes_pointer;
      Q_ASSERT(notes_pointer != nullptr);
      static const nlohmann::json_schema::json_validator notes_cells_validator =
          make_validator(
              "Notes cells",
              nlohmann::json(
                  {{"description", "notes cells"},
                   {"type", "object"},
                   {"required", {"left_column", "right_column", "notes"}},
                   {"properties",
                    nlohmann::json({{"left_column", get_note_column_schema(
                                                        "left note column")},
                                    {"right_column", get_note_column_schema(
                                                         "right note column")},
                                    {"notes", get_notes_schema()}})}}));
      const auto json_notes_cells =
          parse_clipboard(notes_model_pointer->parent_pointer, NOTES_CELLS_MIME,
                          notes_cells_validator);
      if (json_notes_cells.empty()) {
        return;
      }

      Q_ASSERT(json_notes_cells.contains("notes"));
      const auto &json_notes = json_notes_cells["notes"];

      auto number_of_notes =
          std::min({static_cast<qsizetype>(json_notes.size()),
                    notes_pointer->size() - first_child_number});
      QList<Note> new_notes;
      json_to_notes(new_notes, json_notes, number_of_notes);
      undo_stack_pointer->push(
          std::make_unique<SetNotesCells>(
              notes_model_pointer, first_child_number,
              to_note_column(get_json_int(json_notes_cells, "left_column")),
              to_note_column(get_json_int(json_notes_cells, "right_column")),
              copy_items(*notes_pointer, first_child_number, number_of_notes),
              std::move(new_notes))
              .release());
    } else {
      auto *percussions_pointer =
          percussions_model_pointer->percussions_pointer;
      Q_ASSERT(percussions_pointer != nullptr);
      static const nlohmann::json_schema::json_validator
          percussions_cells_validator = make_validator(
              "Percussions cells",
              nlohmann::json(
                  {{"description", "cells"},
                   {"type", "object"},
                   {"required", {"left_column", "right_column", "percussions"}},
                   {"properties",
                    nlohmann::json(
                        {{"left_column", get_percussion_column_schema(
                                             "left percussion column")},
                         {"right_column", get_percussion_column_schema(
                                              "right percussion column")},
                         {"percussions", get_percussions_schema()}})}}));
      const auto json_percussions_cells =
          parse_clipboard(percussions_model_pointer->parent_pointer,
                          PERCUSSIONS_CELLS_MIME, percussions_cells_validator);
      if (json_percussions_cells.empty()) {
        return;
      }

      Q_ASSERT(json_percussions_cells.contains("chords"));
      const auto &json_percussions = json_percussions_cells["chords"];

      auto number_of_percurussions =
          std::min({static_cast<qsizetype>(json_percussions.size()),
                    percussions_pointer->size() - first_child_number});

      QList<Percussion> new_percussions;
      json_to_percussions(new_percussions, json_percussions,
                          number_of_percurussions);
      undo_stack_pointer->push(
          std::make_unique<SetPercussionsCells>(
              percussions_model_pointer, first_child_number,
              to_percussion_column(
                  get_json_int(json_percussions_cells, "left_column")),
              to_percussion_column(
                  get_json_int(json_percussions_cells, "right_column")),
              copy_items((*percussions_pointer), first_child_number,
                         number_of_percurussions),
              std::move(new_percussions))
              .release());
    }
  });
  paste_action_pointer->setShortcuts(QKeySequence::Paste);
  edit_menu_pointer->addAction(paste_action_pointer);

  edit_menu_pointer->addSeparator();

  auto *insert_menu_pointer =
      std::make_unique<QMenu>(tr("&Insert"), edit_menu_pointer).release();

  edit_menu_pointer->addMenu(insert_menu_pointer);

  insert_after_action_pointer->setEnabled(false);
  insert_after_action_pointer->setShortcuts(QKeySequence::InsertLineSeparator);
  connect(insert_after_action_pointer, &QAction::triggered, this, [this]() {
    insert_row(get_only_range(table_view_pointer).bottom() + 1);
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
    const auto &range = get_only_range(table_view_pointer);
    auto first_child_number = get_first_child_number(range);
    auto number_of_children = get_number_of_children(range);

    if (current_model_type == chords_type) {
      undo_stack_pointer->push(
          std::make_unique<RemoveChords>(
              chords_model_pointer, first_child_number,
              copy_items(chords_model_pointer->chords, first_child_number,
                         number_of_children))
              .release());
    } else if (current_model_type == notes_type) {
      auto *notes_pointer = notes_model_pointer->notes_pointer;
      Q_ASSERT(notes_pointer != nullptr);
      undo_stack_pointer->push(
          std::make_unique<RemoveNotes>(notes_model_pointer, first_child_number,
                                        copy_items(*notes_pointer,
                                                   first_child_number,
                                                   number_of_children))
              .release());
    } else {
      auto *percussions_pointer =
          percussions_model_pointer->percussions_pointer;
      Q_ASSERT(percussions_pointer != nullptr);
      undo_stack_pointer->push(
          std::make_unique<RemovePercussions>(
              percussions_model_pointer, first_child_number,
              copy_items((*percussions_pointer), first_child_number,
                         number_of_children))
              .release());
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

  auto *view_menu_pointer =
      std::make_unique<QMenu>(tr("&View"), this).release();

  auto *view_controls_checkbox_pointer =
      std::make_unique<QAction>(tr("&Controls"), view_menu_pointer).release();

  view_controls_checkbox_pointer->setCheckable(true);
  view_controls_checkbox_pointer->setChecked(true);
  connect(view_controls_checkbox_pointer, &QAction::toggled,
          dock_widget_pointer, &QWidget::setVisible);
  view_menu_pointer->addAction(view_controls_checkbox_pointer);

  menu_bar_pointer->addMenu(view_menu_pointer);

  auto *play_menu_pointer =
      std::make_unique<QMenu>(tr("&Play"), this).release();

  play_action_pointer->setEnabled(false);
  play_action_pointer->setShortcuts(QKeySequence::Print);
  connect(play_action_pointer, &QAction::triggered, this, [this]() {
    const auto &chords = chords_model_pointer->chords;

    const auto &range = get_only_range(table_view_pointer);
    auto first_child_number = get_first_child_number(range);
    auto number_of_children = get_number_of_children(range);

    stop_playing();
    initialize_play();
    if (current_model_type == chords_type) {
      if (first_child_number > 0) {
        for (qsizetype chord_number = 0; chord_number < first_child_number;
             chord_number = chord_number + 1) {
          modulate(chords.at(chord_number));
        }
      }
      play_chords(first_child_number, number_of_children);
    } else if (current_model_type == notes_type) {
      if (current_chord_number > 0) {
        for (qsizetype chord_number = 0; chord_number < current_chord_number;
             chord_number = chord_number + 1) {
          modulate(chords.at(chord_number));
        }
      }
      const auto &chord = chords.at(current_chord_number);
      modulate(chord);
      play_notes(current_chord_number, chord, first_child_number,
                 number_of_children);
    } else {
      if (current_chord_number > 0) {
        for (qsizetype chord_number = 0; chord_number < current_chord_number;
             chord_number = chord_number + 1) {
          modulate(chords.at(chord_number));
        }
      }
      const auto &chord = chords.at(current_chord_number);
      modulate(chord);
      play_percussions(current_chord_number, chord, first_child_number,
                       number_of_children);
    }
  });
  play_menu_pointer->addAction(play_action_pointer);

  stop_playing_action_pointer->setEnabled(true);
  play_menu_pointer->addAction(stop_playing_action_pointer);
  connect(stop_playing_action_pointer, &QAction::triggered, this,
          &SongEditor::stop_playing);
  stop_playing_action_pointer->setShortcuts(QKeySequence::Cancel);

  menu_bar_pointer->addMenu(play_menu_pointer);

  auto *controls_form_pointer =
      std::make_unique<QFormLayout>(controls_pointer).release();

  gain_editor_pointer->setMinimum(0);
  gain_editor_pointer->setMaximum(MAX_GAIN);
  gain_editor_pointer->setSingleStep(GAIN_STEP);
  connect(gain_editor_pointer, &QDoubleSpinBox::valueChanged, this,
          &SongEditor::set_gain);
  set_gain_directly(chords_model_pointer->gain);
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

  controls_pointer->setLayout(controls_form_pointer);

  dock_widget_pointer->setWidget(controls_pointer);
  dock_widget_pointer->setFeatures(QDockWidget::NoDockWidgetFeatures);
  addDockWidget(Qt::LeftDockWidgetArea, dock_widget_pointer);

  connect(table_view_pointer, &QAbstractItemView::doubleClicked, this,
          [this](const QModelIndex &index) {
            if (current_model_type == chords_type) {
              auto row = index.row();
              auto column = index.column();
              if (column == chord_notes_column) {
                undo_stack_pointer->push(
                    std::make_unique<EditNotes>(this, row).release());
              } else if (column == chord_percussions_column) {
                undo_stack_pointer->push(
                    std::make_unique<EditPercussions>(this, row).release());
              }
            }
          });

  setWindowTitle("Justly");
  setCentralWidget(table_view_pointer);

  connect(undo_stack_pointer, &QUndoStack::cleanChanged, this, [this]() {
    save_action_pointer->setEnabled(!undo_stack_pointer->isClean() &&
                                    !current_file.isEmpty());
  });

  const auto *primary_screen_pointer = QGuiApplication::primaryScreen();
  Q_ASSERT(primary_screen_pointer != nullptr);
  resize(sizeHint().width(),
         primary_screen_pointer->availableGeometry().height());

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
  insert_after_action_pointer->setEnabled(anything_selected);
  delete_action_pointer->setEnabled(anything_selected);
  remove_rows_action_pointer->setEnabled(anything_selected);
  play_action_pointer->setEnabled(anything_selected);
  paste_action_pointer->setEnabled(anything_selected);
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

void SongEditor::edit_notes(qsizetype chord_number) {
  Q_ASSERT(current_model_type == chords_type);
  Q_ASSERT(notes_model_pointer->notes_pointer == nullptr);
  current_chord_number = chord_number;
  notes_model_pointer->begin_reset_model();
  notes_model_pointer->notes_pointer =
      &chords_model_pointer->chords[chord_number].notes;
  notes_model_pointer->parent_chord_number = chord_number;
  notes_model_pointer->end_reset_model();
  current_model_type = notes_type;
  back_to_chords_action_pointer->setEnabled(true);
  set_model(notes_model_pointer);
}

void SongEditor::edit_percussions(qsizetype chord_number) {
  Q_ASSERT(current_model_type == chords_type);
  Q_ASSERT(percussions_model_pointer->percussions_pointer == nullptr);
  current_chord_number = chord_number;
  percussions_model_pointer->begin_reset_model();
  percussions_model_pointer->percussions_pointer =
      &chords_model_pointer->chords[chord_number].percussions;
  percussions_model_pointer->end_reset_model();
  current_model_type = percussion_type;
  back_to_chords_action_pointer->setEnabled(true);
  set_model(percussions_model_pointer);
}

void SongEditor::notes_to_chords() {
  set_model(chords_model_pointer);
  notes_model_pointer->begin_reset_model();
  notes_model_pointer->notes_pointer = nullptr;
  notes_model_pointer->parent_chord_number = -1;
  notes_model_pointer->end_reset_model();
  current_model_type = chords_type;
  current_chord_number = -1;
  back_to_chords_action_pointer->setEnabled(false);
}

void SongEditor::percussions_to_chords() {
  set_model(chords_model_pointer);
  percussions_model_pointer->begin_reset_model();
  percussions_model_pointer->percussions_pointer = nullptr;
  percussions_model_pointer->end_reset_model();
  current_model_type = chords_type;
  current_chord_number = -1;
  back_to_chords_action_pointer->setEnabled(false);
}

void SongEditor::back_to_chords() {
  if (current_model_type == notes_type) {
    undo_stack_pointer->push(
        std::make_unique<NotesToChords>(this, current_chord_number).release());
  } else if (current_model_type == percussion_type) {
    undo_stack_pointer->push(
        std::make_unique<PercussionsToChords>(this, current_chord_number)
            .release());
  } else {
    Q_ASSERT(false);
  }
}

void SongEditor::set_gain_directly(double new_gain) const {
  set_value_no_signals(gain_editor_pointer, new_gain);
  chords_model_pointer->gain = new_gain;
  fluid_synth_set_gain(synth_pointer, static_cast<float>(new_gain));
}

void SongEditor::set_starting_key_directly(double new_value) const {
  set_value_no_signals(starting_key_editor_pointer, new_value);
  chords_model_pointer->starting_key = new_value;
}

void SongEditor::set_starting_velocity_directly(double new_value) const {
  set_value_no_signals(starting_velocity_editor_pointer, new_value);
  chords_model_pointer->starting_velocity = new_value;
}

void SongEditor::set_starting_tempo_directly(double new_value) const {
  set_value_no_signals(starting_tempo_editor_pointer, new_value);
  chords_model_pointer->starting_tempo = new_value;
}

void SongEditor::set_gain(double new_value) {
  undo_stack_pointer->push(
      std::make_unique<SetGain>(this, chords_model_pointer->gain, new_value)
          .release());
}

void SongEditor::set_starting_key(double new_value) {
  undo_stack_pointer->push(
      std::make_unique<SetStartingKey>(
          this, this->chords_model_pointer->starting_key, new_value)
          .release());
}

void SongEditor::set_starting_velocity(double new_value) {
  undo_stack_pointer->push(
      std::make_unique<SetStartingVelocity>(
          this, this->chords_model_pointer->starting_velocity, new_value)
          .release());
}

void SongEditor::set_starting_tempo(double new_value) {
  undo_stack_pointer->push(
      std::make_unique<SetStartingTempo>(
          this, this->chords_model_pointer->starting_tempo, new_value)
          .release());
}

void SongEditor::insert_row(qsizetype child_number) {
  if (current_model_type == chords_type) {
    undo_stack_pointer->push(std::make_unique<InsertChord>(
                                 chords_model_pointer, child_number, Chord())
                                 .release());
  } else if (current_model_type == notes_type) {
    undo_stack_pointer->push(
        std::make_unique<InsertNote>(notes_model_pointer, child_number, Note())
            .release());
  } else {
    undo_stack_pointer->push(
        std::make_unique<InsertPercussion>(percussions_model_pointer,
                                           child_number, Percussion())
            .release());
  }
}

void SongEditor::delete_cells() {
  const auto &range = get_only_range(table_view_pointer);
  auto first_child_number = get_first_child_number(range);
  auto number_of_children = get_number_of_children(range);
  auto left_column = range.left();
  auto right_column = range.right();

  if (current_model_type == chords_type) {
    undo_stack_pointer->push(
        std::make_unique<SetChordsCells>(
            chords_model_pointer, first_child_number,
            to_chord_column(left_column), to_chord_column(right_column),
            copy_items(chords_model_pointer->chords, first_child_number,
                       number_of_children),
            QList<Chord>(number_of_children))
            .release());
  } else if (current_model_type == notes_type) {
    auto *notes_pointer = notes_model_pointer->notes_pointer;
    Q_ASSERT(notes_pointer != nullptr);
    undo_stack_pointer->push(
        std::make_unique<SetNotesCells>(
            notes_model_pointer, first_child_number,
            to_note_column(left_column), to_note_column(right_column),
            copy_items(*notes_pointer, first_child_number, number_of_children),
            QList<Note>(number_of_children))
            .release());
  } else {
    auto *percussions_pointer = percussions_model_pointer->percussions_pointer;
    Q_ASSERT(percussions_pointer != nullptr);
    undo_stack_pointer->push(
        std::make_unique<SetPercussionsCells>(
            percussions_model_pointer, first_child_number,
            to_percussion_column(left_column),
            to_percussion_column(right_column),
            copy_items((*percussions_pointer), first_child_number,
                       number_of_children),
            QList<Percussion>(number_of_children))
            .release());
  }
}

void SongEditor::copy() const {
  const auto &range = get_only_range(table_view_pointer);
  auto first_child_number = get_first_child_number(range);
  auto number_of_children = get_number_of_children(range);
  auto left_column = range.left();
  auto right_column = range.right();

  if (current_model_type == chords_type) {
    copy_json(
        nlohmann::json({{"left_column", to_chord_column(left_column)},
                        {"right_column", to_chord_column(right_column)},
                        {"chords", chords_to_json(chords_model_pointer->chords,
                                                  first_child_number,
                                                  number_of_children)}}),
        CHORDS_CELLS_MIME);
  } else if (current_model_type == notes_type) {
    auto *notes_pointer = notes_model_pointer->notes_pointer;
    Q_ASSERT(notes_pointer != nullptr);
    copy_json(nlohmann::json(
                  {{"left_column", to_note_column(left_column)},
                   {"right_column", to_note_column(right_column)},
                   {"notes", notes_to_json(*notes_pointer, first_child_number,
                                           number_of_children)}}),
              NOTES_CELLS_MIME);
  } else {
    auto *percussions_pointer = percussions_model_pointer->percussions_pointer;
    Q_ASSERT(percussions_pointer != nullptr);
    copy_json(nlohmann::json(
                  {{"left_column", to_percussion_column(left_column)},
                   {"right_column", to_percussion_column(right_column)},
                   {"percussions", percussions_to_json((*percussions_pointer),
                                                       first_child_number,
                                                       number_of_children)}}),
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
  fluid_settings_setstr(settings_pointer, "audio.driver", "pulseaudio");
#else
  fluid_settings_setstr(settings_pointer, "audio.driver",
                        default_driver.c_str());
#endif

#ifndef NO_REALTIME_AUDIO
  audio_driver_pointer =
      new_fluid_audio_driver(settings_pointer, synth_pointer);
#endif
  if (audio_driver_pointer == nullptr) {
    QString message;
    QTextStream stream(&message);
    stream << SongEditor::tr("Cannot start audio driver \"") << default_driver
           << SongEditor::tr("\"");
#ifdef NO_WARN_AUDIO
    qWarning("%s", message.toStdString().c_str());
#else
    QMessageBox::warning(this, SongEditor::tr("Audio driver error"), message);
#endif
  }
}

void SongEditor::initialize_play() {
  current_key = chords_model_pointer->starting_key;
  current_velocity = chords_model_pointer->starting_velocity;
  current_tempo = chords_model_pointer->starting_tempo;
  current_time = fluid_sequencer_get_tick(sequencer_pointer);
  for (qsizetype index = 0; index < NUMBER_OF_MIDI_CHANNELS;
       index = index + 1) {
    Q_ASSERT(index < channel_schedules.size());
    channel_schedules[index] = 0;
  }
}

auto SongEditor::get_open_channel_number(
    qsizetype chord_number, qsizetype item_number,
    const QString &item_description) -> qsizetype {
  for (qsizetype channel_index = 0; channel_index < NUMBER_OF_MIDI_CHANNELS;
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

void SongEditor::change_instrument(int channel_number, int16_t bank_number,
                                   int16_t preset_number) const {
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
}

void SongEditor::play_note(int channel_number, int16_t midi_number,
                           const Rational &beats,
                           const Rational &velocity_ratio,
                           const Rational &tempo_ratio, int time_offset,
                           int chord_number, int item_number,
                           const QString &item_description) {
  auto velocity = current_velocity * rational_to_double(velocity_ratio);
  int16_t new_velocity = 1;
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
    new_velocity = static_cast<int16_t>(std::round(velocity));
  }
  fluid_event_noteon(event_pointer, channel_number, midi_number, new_velocity);
  send_event_at(current_time + time_offset);

  auto end_time =
      current_time +
      get_beat_time(current_tempo * rational_to_double(tempo_ratio)) *
          rational_to_double(beats) * MILLISECONDS_PER_SECOND;

  fluid_event_noteoff(event_pointer, channel_number, midi_number);
  send_event_at(end_time);
  Q_ASSERT(channel_number < channel_schedules.size());
  channel_schedules[channel_number] = end_time;
  update_final_time(end_time);
}

void SongEditor::play_notes(qsizetype chord_number, const Chord &chord,
                            qsizetype first_note_index,
                            qsizetype number_of_notes) {
  for (auto note_index = first_note_index;
       note_index < first_note_index + number_of_notes;
       note_index = note_index + 1) {
    auto channel_number = get_open_channel_number(chord_number, note_index,
                                                  SongEditor::tr("note"));
    if (channel_number != -1) {
      const auto &note = chord.notes.at(note_index);

      const auto *instrument_pointer = note.instrument_pointer;
      Q_ASSERT(instrument_pointer != nullptr);
      change_instrument(channel_number, instrument_pointer->bank_number,
                        instrument_pointer->preset_number);

      auto midi_float =
          get_midi(current_key * interval_to_double(note.interval));
      auto closest_midi = round(midi_float);
      auto midi_number = static_cast<int16_t>(closest_midi);

      fluid_event_pitch_bend(
          event_pointer, channel_number,
          static_cast<int>(
              std::round((midi_float - closest_midi + ZERO_BEND_HALFSTEPS) *
                         BEND_PER_HALFSTEP)));
      send_event_at(current_time + 1);

      play_note(channel_number, midi_number, note.beats, note.velocity_ratio,
                note.tempo_ratio, 2, chord_number, note_index,
                SongEditor::tr("note"));
    }
  }
}

void SongEditor::play_percussions(qsizetype chord_number, const Chord &chord,
                                  qsizetype first_percussion_number,
                                  qsizetype number_of_percussions) {
  for (auto percussion_number = first_percussion_number;
       percussion_number < first_percussion_number + number_of_percussions;
       percussion_number = percussion_number + 1) {
    auto channel_number = get_open_channel_number(
        chord_number, percussion_number, SongEditor::tr("percussion"));
    if (channel_number != -1) {
      const auto &percussion = chord.percussions.at(percussion_number);

      const auto *percussion_set_pointer = percussion.percussion_set_pointer;
      Q_ASSERT(percussion_set_pointer != nullptr);

      change_instrument(channel_number, percussion_set_pointer->bank_number,
                        percussion_set_pointer->preset_number);

      const auto *percussion_instrument_pointer =
          percussion.percussion_instrument_pointer;
      Q_ASSERT(percussion_instrument_pointer != nullptr);

      play_note(channel_number, percussion_instrument_pointer->midi_number,
                percussion.beats, percussion.velocity_ratio,
                percussion.tempo_ratio, 1, chord_number, percussion_number,
                SongEditor::tr("percussion"));
    }
  }
}

void SongEditor::play_chords(qsizetype first_chord_number,
                             qsizetype number_of_chords, int wait_frames) {
  const auto &chords = chords_model_pointer->chords;

  auto start_time = current_time + wait_frames;
  current_time = start_time;
  update_final_time(start_time);
  for (auto chord_number = first_chord_number;
       chord_number < first_chord_number + number_of_chords;
       chord_number = chord_number + 1) {
    const auto &chord = chords.at(chord_number);

    modulate(chord);
    play_notes(chord_number, chord, 0, chord.notes.size());
    play_percussions(chord_number, chord, 0, chord.percussions.size());
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
  auto *dialog_pointer =
      std::make_unique<QFileDialog>(this, caption, current_folder, filter)
          .release();

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

  if (current_model_type != chords_type) {
    back_to_chords();
  }

  const auto &chords = chords_model_pointer->chords;
  if (!chords.empty()) {
    Q_ASSERT(chords_model_pointer != nullptr);
    remove_chords(*chords_model_pointer, 0, chords.size());
  }

  if (json_song.contains("chords")) {
    const auto &json_chords = json_song["chords"];

    Q_ASSERT(chords_model_pointer != nullptr);
    auto &chords = chords_model_pointer->chords;

    chords_model_pointer->begin_insert_rows(chords.size(), json_chords.size());
    json_to_chords(chords, json_chords, json_chords.size());
    chords_model_pointer->end_insert_rows();
  }

  current_file = filename;

  undo_stack_pointer->clear();
  undo_stack_pointer->setClean();
}

void SongEditor::save_as_file(const QString &filename) {
  const auto &chords = chords_model_pointer->chords;

  std::ofstream file_io(filename.toStdString().c_str());

  nlohmann::json json_song;
  json_song["gain"] = chords_model_pointer->gain;
  json_song["starting_key"] = chords_model_pointer->starting_key;
  json_song["starting_tempo"] = chords_model_pointer->starting_tempo;
  json_song["starting_velocity"] = chords_model_pointer->starting_velocity;

  if (!chords.empty()) {
    json_song["chords"] = chords_to_json(chords, 0, chords.size());
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
  play_chords(0, chords_model_pointer->chords.size(), START_END_MILLISECONDS);

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

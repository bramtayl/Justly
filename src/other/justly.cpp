#include "justly/justly.hpp"

#include <QAbstractItemDelegate>
#include <QAbstractItemModel>
#include <QAction>
#include <QMessageBox>
#include <QMetaObject>
#include <QMetaProperty> // IWYU pragma: keep
#include <QMetaType>
#include <QSpinBox>
#include <QString>
#include <QStyleOption>
#include <QTextStream>
#include <QUndoStack>
#include <QWidget>
#include <QtGlobal>
#include <cstddef>
#include <exception>
#include <fluidsynth.h>
#include <fstream>
#include <iomanip>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "instrument/Instrument.hpp"
#include "instrument/InstrumentEditor.hpp"
#include "interval/Interval.hpp"
#include "note_chord/Chord.hpp"
#include "other/ChordsModel.hpp"
#include "other/ChordsView.hpp"
#include "other/SongEditor.hpp"
#include "percussion/Percussion.hpp"
#include "rational/Rational.hpp"

// insert end buffer at the end of songs
static const unsigned int START_END_MILLISECONDS = 500;

[[nodiscard]] static auto get_json_value(const nlohmann::json &json,
                                         const std::string &field) {
  Q_ASSERT(json.contains(field));
  return json[field];
}

[[nodiscard]] static auto get_json_double(const nlohmann::json &json,
                                          const std::string &field) {
  const auto &json_value = get_json_value(json, field);
  Q_ASSERT(json_value.is_number());
  return json_value.get<double>();
}

void register_converters() {
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
  QMetaType::registerConverter<const Instrument *, QString>(
      [](const Instrument *instrument_pointer) {
        Q_ASSERT(instrument_pointer != nullptr);
        return QString::fromStdString(instrument_pointer->name);
      });
  QMetaType::registerConverter<const Percussion *, QString>(
      [](const Percussion *percussion_pointer) {
        Q_ASSERT(percussion_pointer != nullptr);
        return QString::fromStdString(percussion_pointer->name);
      });
}

auto make_song_editor() -> SongEditor * { return new SongEditor; }

void show_song_editor(SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->show();
}

void delete_song_editor(SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  delete song_editor_pointer;
}

auto get_chord_index(const SongEditor *song_editor_pointer, size_t chord_number,
                     NoteChordColumn note_chord_column) -> QModelIndex {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->chords_model_pointer->get_chord_index(
      chord_number, note_chord_column);
}

auto get_note_index(const SongEditor *song_editor_pointer, size_t chord_number,
                    size_t note_number,
                    NoteChordColumn note_chord_column) -> QModelIndex {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->chords_model_pointer->get_note_index(
      chord_number, note_number, note_chord_column);
}

auto get_chords_view_pointer(const SongEditor *song_editor_pointer)
    -> QTreeView * {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->chords_view_pointer;
}


auto get_gain(const SongEditor *song_editor_pointer) -> double {
  Q_ASSERT(song_editor_pointer != nullptr);
  return fluid_synth_get_gain(song_editor_pointer->synth_pointer);
};

auto get_starting_instrument_name(const SongEditor *song_editor_pointer)
    -> std::string {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->chords_model_pointer->starting_instrument_pointer
      ->name;
};

auto get_starting_key(const SongEditor *song_editor_pointer) -> double {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->chords_model_pointer->starting_key;
};

auto get_starting_velocity(const SongEditor *song_editor_pointer) -> double {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->chords_model_pointer->starting_velocity;
};

auto get_starting_tempo(const SongEditor *song_editor_pointer) -> double {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->chords_model_pointer->starting_tempo;
};

auto get_current_file(const SongEditor *song_editor_pointer) -> QString {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->current_file;
};

void set_gain(const SongEditor *song_editor_pointer, double new_value) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->gain_editor_pointer->setValue(new_value);
};

void set_starting_instrument_name(const SongEditor *song_editor_pointer,
                                  const std::string &new_name) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->starting_instrument_editor_pointer->setValue(
      get_instrument_pointer(new_name));
}

void set_starting_key(const SongEditor *song_editor_pointer, double new_value) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->starting_key_editor_pointer->setValue(new_value);
}

void set_starting_velocity(const SongEditor *song_editor_pointer,
                           double new_value) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->starting_velocity_editor_pointer->setValue(new_value);
}

void set_starting_tempo(const SongEditor *song_editor_pointer,
                        double new_value) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->starting_tempo_editor_pointer->setValue(new_value);
}

auto create_editor(const SongEditor *song_editor_pointer,
                   QModelIndex index) -> QWidget * {
  Q_ASSERT(song_editor_pointer != nullptr);

  auto *delegate_pointer =
      song_editor_pointer->chords_view_pointer->itemDelegate();
  Q_ASSERT(delegate_pointer != nullptr);

  auto *viewport_pointer = song_editor_pointer->chords_view_pointer->viewport();
  Q_ASSERT(viewport_pointer != nullptr);

  auto *cell_editor_pointer = delegate_pointer->createEditor(
      viewport_pointer, QStyleOptionViewItem(), index);
  Q_ASSERT(cell_editor_pointer != nullptr);

  delegate_pointer->setEditorData(cell_editor_pointer, index);

  return cell_editor_pointer;
}

void set_editor(const SongEditor *song_editor_pointer,
                QWidget *cell_editor_pointer, QModelIndex index,
                const QVariant &new_value) {
  Q_ASSERT(song_editor_pointer != nullptr);

  Q_ASSERT(cell_editor_pointer != nullptr);
  const auto *cell_editor_meta_object = cell_editor_pointer->metaObject();

  Q_ASSERT(cell_editor_meta_object != nullptr);
  cell_editor_pointer->setProperty(
      cell_editor_meta_object->userProperty().name(), new_value);

  auto *delegate_pointer =
      song_editor_pointer->chords_view_pointer->itemDelegate();
  Q_ASSERT(delegate_pointer != nullptr);

  delegate_pointer->setModelData(
      cell_editor_pointer, song_editor_pointer->chords_model_pointer, index);
}

void undo(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->undo_stack_pointer->undo();
};
void redo(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->undo_stack_pointer->redo();
};

void trigger_insert_after(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->insert_after_action_pointer->trigger();
};
void trigger_insert_into(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->insert_into_action_pointer->trigger();
};
void trigger_delete(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->delete_action_pointer->trigger();
};
void trigger_cut(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->cut_action_pointer->trigger();
};
void trigger_copy(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->copy_action_pointer->trigger();
};
void trigger_paste_cells_or_after(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->paste_cells_or_after_action_pointer->trigger();
};
void trigger_paste_into(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->paste_into_action_pointer->trigger();
};

void trigger_save(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->save_action_pointer->trigger();
};

void trigger_play(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->play_action_pointer->trigger();
};

void trigger_stop_playing(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->stop_playing_action_pointer->trigger();
};

void trigger_expand(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->expand_action_pointer->trigger();
};

void trigger_collapse(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->collapse_action_pointer->trigger();
};

void open_file(SongEditor *song_editor_pointer, const QString &filename) {
  Q_ASSERT(song_editor_pointer != nullptr);

  std::ifstream file_io(filename.toStdString().c_str());
  nlohmann::json json_song;
  try {
    json_song = nlohmann::json::parse(file_io);
  } catch (const nlohmann::json::parse_error &parse_error) {
    QMessageBox::warning(song_editor_pointer, SongEditor::tr("Parsing error"),
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
               {"required",
                {"starting_key", "starting_tempo", "starting_velocity",
                 "starting_instrument"}},
               {"properties",
                {{"starting_instrument",
                  get_instrument_schema("the starting instrument")},
                 {"gain",
                  {{"type", "number"},
                   {"description", "the gain (speaker volume)"},
                   {"minimum", 0},
                   {"maximum", MAX_GAIN}}},
                 {"starting_key",
                  {{"type", "number"},
                   {"description", "the starting key, in Hz"},
                   {"minimum", MIN_STARTING_KEY},
                   {"maximum", MAX_STARTING_KEY}}},
                 {"starting_tempo",
                  {{"type", "number"},
                   {"description", "the starting tempo, in bpm"},
                   {"minimum", MIN_STARTING_TEMPO},
                   {"maximum", MAX_STARTING_TEMPO}}},
                 {"starting_velocity",
                  {{"type", "number"},
                   {"description", "the starting velocity (note force)"},
                   {"minimum", 0},
                   {"maximum", MAX_VELOCITY}}},
                 {"chords", get_chords_schema()}}}}));
  try {
    song_validator.validate(json_song);
  } catch (const std::exception &error) {
    QMessageBox::warning(song_editor_pointer, SongEditor::tr("Schema error"),
                         error.what());
    return;
  }

  if (json_song.contains("gain")) {
    set_gain(song_editor_pointer, get_json_double(json_song, "gain"));
  }

  if (json_song.contains("starting_key")) {
    set_starting_key(song_editor_pointer,
                     get_json_double(json_song, "starting_key"));
  }

  if (json_song.contains("starting_velocity")) {
    set_starting_velocity(song_editor_pointer,
                          get_json_double(json_song, "starting_velocity"));
  }

  if (json_song.contains("starting_tempo")) {
    set_starting_tempo(song_editor_pointer,
                       get_json_double(json_song, "starting_tempo"));
  }

  if (json_song.contains("starting_instrument")) {
    const auto &starting_instrument_value =
        get_json_value(json_song, "starting_instrument");
    Q_ASSERT(starting_instrument_value.is_string());
    set_starting_instrument_name(song_editor_pointer,
                                 starting_instrument_value.get<std::string>());
  }

  const auto &chords = song_editor_pointer->chords_model_pointer->chords;
  if (!chords.empty()) {
    song_editor_pointer->chords_model_pointer->remove_chords(0, chords.size());
  }

  if (json_song.contains("chords")) {
    song_editor_pointer->chords_model_pointer->append_json_chords(
        json_song["chords"]);
  }

  song_editor_pointer->current_file = filename;

  song_editor_pointer->undo_stack_pointer->clear();
  song_editor_pointer->undo_stack_pointer->setClean();
}

void save_as_file(SongEditor *song_editor_pointer, const QString &filename) {
  Q_ASSERT(song_editor_pointer != nullptr);
  std::ofstream file_io(filename.toStdString().c_str());

  nlohmann::json json_song;
  json_song["gain"] = song_editor_pointer->chords_model_pointer->gain;
  json_song["starting_key"] = get_starting_key(song_editor_pointer);
  json_song["starting_tempo"] = get_starting_tempo(song_editor_pointer);
  json_song["starting_velocity"] = get_starting_velocity(song_editor_pointer);
  json_song["starting_instrument"] =
      get_starting_instrument_name(song_editor_pointer);

  const auto &chords = song_editor_pointer->chords_model_pointer->chords;
  if (!chords.empty()) {
    json_song["chords"] = chords_to_json(chords, 0, chords.size());
  }

  file_io << std::setw(4) << json_song;
  file_io.close();
  song_editor_pointer->current_file = filename;

  song_editor_pointer->undo_stack_pointer->setClean();
}

void export_to_file(SongEditor *song_editor_pointer,
                    const QString &output_file) {
  Q_ASSERT(song_editor_pointer != nullptr);
  stop_playing(song_editor_pointer->sequencer_pointer,
               song_editor_pointer->event_pointer);

  delete_audio_driver(song_editor_pointer);
  auto file_result = fluid_settings_setstr(
      song_editor_pointer->settings_pointer, "audio.file.name",
      output_file.toStdString().c_str());
  Q_ASSERT(file_result == FLUID_OK);

  auto unlock_result = fluid_settings_setint(
      song_editor_pointer->settings_pointer, "synth.lock-memory", 0);
  Q_ASSERT(unlock_result == FLUID_OK);

  auto finished = false;
  auto finished_timer_id = fluid_sequencer_register_client(
      song_editor_pointer->sequencer_pointer, "finished timer",
      [](unsigned int /*time*/, fluid_event_t * /*event*/,
         fluid_sequencer_t * /*seq*/, void *data_pointer) {
        auto *finished_pointer = static_cast<bool *>(data_pointer);
        Q_ASSERT(finished_pointer != nullptr);
        *finished_pointer = true;
      },
      &finished);
  Q_ASSERT(finished_timer_id >= 0);

  initialize_play(song_editor_pointer);
  play_chords(song_editor_pointer, 0,
              song_editor_pointer->chords_model_pointer->chords.size(),
              START_END_MILLISECONDS);

  fluid_event_set_dest(song_editor_pointer->event_pointer, finished_timer_id);
  fluid_event_timer(song_editor_pointer->event_pointer, nullptr);
  send_event_at(song_editor_pointer->sequencer_pointer,
                song_editor_pointer->event_pointer,
                song_editor_pointer->final_time + START_END_MILLISECONDS);

  auto *renderer_pointer =
      new_fluid_file_renderer(song_editor_pointer->synth_pointer);
  Q_ASSERT(renderer_pointer != nullptr);
  while (!finished) {
    auto process_result = fluid_file_renderer_process_block(renderer_pointer);
    Q_ASSERT(process_result == FLUID_OK);
  }
  delete_fluid_file_renderer(renderer_pointer);

  fluid_event_set_dest(song_editor_pointer->event_pointer,
                       song_editor_pointer->sequencer_id);
  auto lock_result = fluid_settings_setint(
      song_editor_pointer->settings_pointer, "synth.lock-memory", 1);
  Q_ASSERT(lock_result == FLUID_OK);
  start_real_time(song_editor_pointer);
}
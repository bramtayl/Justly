#include <QtCore/QAbstractItemModel>
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaType>
#include <QtCore/QMimeData>
#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>
#include <QtCore/QStringListModel>
#include <QtCore/QTextStream>
#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/Qt>
#include <QtCore/QtGlobal>
#include <QtGui/QAction>
#include <QtGui/QClipboard>
#include <QtGui/QCloseEvent> // IWYU pragma: keep
#include <QtGui/QGuiApplication>
#include <QtGui/QIcon>
#include <QtGui/QKeySequence>
#include <QtGui/QPixmap>
#include <QtGui/QScreen>
#include <QtGui/QUndoStack>
#include <QtWidgets/QAbstractItemDelegate>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QAbstractScrollArea>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QItemEditorFactory>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QTableView>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fluidsynth.h>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "justly/justly.hpp"

static const auto BEND_PER_HALFSTEP = 4096;
static const auto C_0_MIDI = 12;
static const auto CENTS_PER_HALFSTEP = 100;
static const auto CONCERT_A_FREQUENCY = 440;
static const auto CONCERT_A_MIDI = 69;
static const auto DEFAULT_GAIN = 5;
static const auto DEFAULT_STARTING_KEY = 220;
static const auto DEFAULT_STARTING_TEMPO = 100;
static const auto DEFAULT_STARTING_VELOCITY = 64;
static const auto GAIN_STEP = 0.1;
static const auto HALFSTEPS_PER_OCTAVE = 12;
static const auto MAX_GAIN = 10;
static const auto MAX_OCTAVE = 9;
static const auto MAX_RATIONAL_DENOMINATOR = 999;
static const auto MAX_RATIONAL_NUMERATOR = 999;
static const auto MAX_STARTING_KEY = 999;
static const auto MAX_STARTING_TEMPO = 999;
static const auto MAX_VELOCITY = 127;
static const auto MILLISECONDS_PER_MINUTE = 60000;
static const auto NUMBER_OF_MIDI_CHANNELS = 16;
static const auto OCTAVE_RATIO = 2.0;
static const auto START_END_MILLISECONDS = 500;
static const auto ZERO_BEND_HALFSTEPS = 2;

enum ControlId {
  gain_id,
  starting_key_id,
  starting_velocity_id,
  starting_tempo_id
};

enum Degree {
  c_degree,
  c_sharp_degree,
  d_degree,
  e_flat_degree,
  e_degree,
  f_degree,
  f_sharp_degree,
  g_degree,
  a_flat_degree,
  a_degree,
  b_flat_degree,
  b_degree
};

enum ModelType { chords_type, pitched_notes_type, unpitched_notes_type };

template <typename Thing>
[[nodiscard]] static auto get_reference(Thing *thing_pointer) -> Thing & {
  Q_ASSERT(thing_pointer != nullptr);
  return *thing_pointer;
}

template <typename Thing>
[[nodiscard]] static auto
get_const_reference(const Thing *thing_pointer) -> const Thing & {
  Q_ASSERT(thing_pointer != nullptr);
  return *thing_pointer;
}

template <typename Iterable>
[[nodiscard]] static auto get_only(const Iterable &iterable) {
  Q_ASSERT(iterable.size() == 1);
  return iterable.at(0);
}

template <typename Item>
[[nodiscard]] static auto copy_items(const QList<Item> &items,
                                     const int first_row_number,
                                     const int number_of_rows) {
  QList<Item> copied;
  std::copy(items.cbegin() + first_row_number,
            items.cbegin() + first_row_number + number_of_rows,
            std::back_inserter(copied));
  return copied;
}

[[nodiscard]] static auto to_int(const double value) {
  return static_cast<int>(std::round(value));
}

template <typename SubType>
[[nodiscard]] static auto variant_to(const QVariant &variant) {
  Q_ASSERT(variant.canConvert<SubType>());
  return variant.value<SubType>();
}

[[nodiscard]] static auto get_json_value(const nlohmann::json &json_data,
                                         const char *const field_name) {
  Q_ASSERT(json_data.is_object());
  Q_ASSERT(json_data.contains(field_name));
  return json_data[field_name];
}

[[nodiscard]] static auto get_json_int(const nlohmann::json &json_data,
                                       const char *const field_name) {
  const auto &json_value = get_json_value(json_data, field_name);
  Q_ASSERT(json_value.is_number_integer());
  return json_value.get<int>();
}

static auto get_json_double(const nlohmann::json &json_song,
                            const char *const field_name) -> double {
  const auto &json_value = get_json_value(json_song, field_name);
  Q_ASSERT(json_value.is_number());
  return json_value.get<double>();
}

static void add_int_to_json(nlohmann::json &json_object,
                            const char *const field_name, const int value,
                            const int default_value) {
  Q_ASSERT(json_object.is_object());
  if (value != default_value) {
    json_object[field_name] = value;
  }
}

static void add_words_to_json(nlohmann::json &json_row, const QString &words) {
  Q_ASSERT(json_row.is_object());
  if (!words.isEmpty()) {
    json_row["words"] = words.toStdString().c_str();
  }
}

[[nodiscard]] static auto get_number_schema(const char *const type,
                                            const int minimum,
                                            const int maximum) {
  Q_ASSERT(type != nullptr);
  return nlohmann::json(
      {{"type", type}, {"minimum", minimum}, {"maximum", maximum}});
}

[[nodiscard]] static auto get_rational_fields_schema() {
  return nlohmann::json(
      {{"numerator", get_number_schema("integer", 1, MAX_RATIONAL_NUMERATOR)},
       {"denominator",
        get_number_schema("integer", 1, MAX_RATIONAL_DENOMINATOR)}});
}

[[nodiscard]] static auto
get_object_schema(const nlohmann::json &properties_json) {
  return nlohmann::json({{"type", "object"}, {"properties", properties_json}});
}

template <std::derived_from<QWidget> SubWidget>
[[nodiscard]] static auto get_default_size() -> const QSize & {
  static const auto default_size = SubWidget(nullptr).sizeHint();
  return default_size;
}

[[nodiscard]] static auto
make_validator(const nlohmann::json &required_json,
               const nlohmann::json &properties_json) {
  return nlohmann::json_schema::json_validator(
      nlohmann::json({{"type", "object"},
                      {"&schema", "http://json-schema.org/draft-07/schema#"},
                      {"required", required_json},
                      {"properties", properties_json}}));
}

[[nodiscard]] static auto get_midi(const double key) {
  Q_ASSERT(key > 0);
  return HALFSTEPS_PER_OCTAVE * log2(key / CONCERT_A_FREQUENCY) +
         CONCERT_A_MIDI;
}

static void show_warning(QWidget &parent, const char *const title,
                         const QString &message) {
  QMessageBox::warning(&parent, QObject::tr(title), message);
}

static void check_fluid_ok(const int fluid_result) {
  Q_ASSERT(fluid_result == FLUID_OK);
}

static void set_fluid_int(fluid_settings_t *settings_pointer,
                          const char *const field, const int value) {
  check_fluid_ok(fluid_settings_setint(settings_pointer, field, value));
}

[[nodiscard]] static auto get_soundfont_id(fluid_synth_t *synth_pointer) {
  const auto soundfont_file = QDir(QCoreApplication::applicationDirPath())
                                  .filePath("../share/MuseScore_General.sf2")
                                  .toStdString();
  Q_ASSERT(std::filesystem::exists(soundfont_file));

  const auto soundfont_id =
      fluid_synth_sfload(synth_pointer, soundfont_file.c_str(), 1);
  Q_ASSERT(soundfont_id >= 0);
  return soundfont_id;
}

static void send_event_at(fluid_sequencer_t *const sequencer_pointer,
                          fluid_event_t *const event_pointer,
                          const double time) {
  Q_ASSERT(time >= 0);
  check_fluid_ok(
      fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                              static_cast<unsigned int>(std::round(time)), 1));
}

[[nodiscard]] static auto make_audio_driver(
    QWidget &parent, fluid_settings_t *const settings_pointer,
    fluid_synth_t *const synth_pointer) -> fluid_audio_driver_t * {
#ifndef NO_REALTIME_AUDIO
  auto *const audio_driver_pointer =
      new_fluid_audio_driver(settings_pointer, synth_pointer);
  if (audio_driver_pointer == nullptr) {
    show_warning(parent, "Audio driver error",
                 QObject::tr("Cannot start audio driver"));
  }
  return audio_driver_pointer;
#else
  return nullptr;
#endif
}

static void
delete_audio_driver(fluid_audio_driver_t *const audio_driver_pointer) {
  if (audio_driver_pointer != nullptr) {
    delete_fluid_audio_driver(audio_driver_pointer);
  }
}

static void stop_playing(fluid_event_t *const event_pointer,
                         fluid_sequencer_t *const sequencer_pointer) {
  fluid_sequencer_remove_events(sequencer_pointer, -1, -1, -1);

  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       channel_number = channel_number + 1) {
    fluid_event_all_sounds_off(event_pointer, channel_number);
    fluid_sequencer_send_now(sequencer_pointer, event_pointer);
  }
}

[[nodiscard]] static auto get_selection(const QAbstractItemView &switch_table) {
  return get_const_reference(switch_table.selectionModel()).selection();
}

[[nodiscard]] static auto
get_only_range(const QAbstractItemView &switch_table) {
  return get_only(get_selection(switch_table));
}

[[nodiscard]] static auto get_next_row(const QAbstractItemView &switch_table) {
  return get_only(get_selection(switch_table)).bottom() + 1;
}

static void double_click_column(QAbstractItemView &switch_table,
                                const int chord_number,
                                const int chord_column) {
  switch_table.doubleClicked(get_const_reference(switch_table.model())
                                 .index(chord_number, chord_column));
}

[[nodiscard]] static auto get_number_of_rows(const QItemSelectionRange &range) {
  Q_ASSERT(range.isValid());
  return range.bottom() - range.top() + 1;
}

[[nodiscard]] static auto make_action(const char *const name) {
  return QAction(QObject::tr(name));
}

static void add_menu_action(
    QMenu &menu, QAction &action,
    const QKeySequence::StandardKey key_sequence = QKeySequence::StandardKey(),
    const bool enabled = true) {
  action.setShortcuts(key_sequence);
  action.setEnabled(enabled);
  menu.addAction(&action);
}

static void clear_and_clean(QUndoStack &undo_stack) {
  undo_stack.clear();
  undo_stack.setClean();
}

static void add_control(QFormLayout &controls_form, const char *const label,
                        QDoubleSpinBox &spin_box, const int minimum,
                        const int maximum, const char *const suffix,
                        const double single_step = 1, const int decimals = 0) {
  Q_ASSERT(suffix != nullptr);
  spin_box.setMinimum(minimum);
  spin_box.setMaximum(maximum);
  spin_box.setSuffix(QObject::tr(suffix));
  spin_box.setSingleStep(single_step);
  spin_box.setDecimals(decimals);
  controls_form.addRow(QObject::tr(label), &spin_box);
}

// a subnamed should have the following methods:
// static auto SubNamed::get_all_nameds() -> const QList<SubNamed>&;
// static auto get_field_name() -> const char*;
struct Named {
  QString name;

  explicit Named(const char *const name_input) : name(name_input){};
};

[[nodiscard]] static auto get_name_or_empty(const Named *named_pointer) {
  if (named_pointer == nullptr) {
    return QString("");
  }
  return named_pointer->name;
}

template <std::derived_from<Named> SubNamed>
[[nodiscard]] static auto get_by_name(const QString &name) -> const SubNamed & {
  const auto &all_nameds = SubNamed::get_all_nameds();
  const auto named_pointer =
      std::find_if(all_nameds.cbegin(), all_nameds.cend(),
                   [name](const SubNamed &item) { return item.name == name; });
  Q_ASSERT(named_pointer != nullptr);
  return *named_pointer;
}

template <std::derived_from<Named> SubNamed>
static void add_named_to_json(nlohmann::json &json_row,
                              const SubNamed *named_pointer) {
  Q_ASSERT(json_row.is_object());
  if (named_pointer != nullptr) {
    json_row[SubNamed::get_field_name()] = named_pointer->name.toStdString();
  }
}

template <std::derived_from<Named> SubNamed>
[[nodiscard]] static auto json_field_to_named_pointer(
    const nlohmann::json &json_row) -> const SubNamed * {
  Q_ASSERT(json_row.is_object());

  const char *const field_name = SubNamed::get_field_name();
  Q_ASSERT(field_name != nullptr);
  if (json_row.contains(field_name)) {
    const auto &json_named = json_row[field_name];
    Q_ASSERT(json_named.is_string());
    return &get_by_name<SubNamed>(
        QString::fromStdString(json_named.get<std::string>()));
  };
  return nullptr;
}

template <std::derived_from<Named> SubNamed>
static void add_named_schema(nlohmann::json &json_row) {
  Q_ASSERT(json_row.is_object());

  std::vector<std::string> names;
  const auto &all_nameds = SubNamed::get_all_nameds();
  std::transform(all_nameds.cbegin(), all_nameds.cend(),
                 std::back_inserter(names),
                 [](const SubNamed &item) { return item.name.toStdString(); });
  json_row[SubNamed::get_field_name()] =
      nlohmann::json({{"type", "string"}, {"enum", std::move(names)}});
}

struct PercussionInstrument : public Named {
  short midi_number;

  PercussionInstrument(const char *const name, const short midi_number_input)
      : Named(name), midi_number(midi_number_input) {}

  [[nodiscard]] static auto
  get_all_nameds() -> const QList<PercussionInstrument> & {
    static const QList<PercussionInstrument> all_percussions({
        PercussionInstrument("Acoustic or Low Bass Drum", 35),
        PercussionInstrument("Acoustic Snare", 38),
        PercussionInstrument("Belltree", 84),
        PercussionInstrument("Castanets", 85),
        PercussionInstrument("Chinese Cymbal", 52),
        PercussionInstrument("Closed Hi-hat", 42),
        PercussionInstrument("Cowbell", 56),
        PercussionInstrument("Crash Cymbal 1", 49),
        PercussionInstrument("Crash Cymbal 2", 57),
        PercussionInstrument("Drum sticks", 31),
        PercussionInstrument("Electric or High Bass Drum", 36),
        PercussionInstrument("Electric Snare or Rimshot", 40),
        PercussionInstrument("Hand Clap", 39),
        PercussionInstrument("High Floor Tom", 43),
        PercussionInstrument("High Q or Filter Snap", 27),
        PercussionInstrument("High Tom", 50),
        PercussionInstrument("High-Mid Tom", 48),
        PercussionInstrument("Jingle Bell", 83),
        PercussionInstrument("Low Floor Tom", 41),
        PercussionInstrument("Low Tom", 45),
        PercussionInstrument("Low-Mid Tom", 47),
        PercussionInstrument("Metronome Bell", 34),
        PercussionInstrument("Metronome Click", 33),
        PercussionInstrument("Mute Surdo", 86),
        PercussionInstrument("Open Hi-hat", 46),
        PercussionInstrument("Open Surdo", 87),
        PercussionInstrument("Pedal Hi-hat", 44),
        PercussionInstrument("Ride Bell", 53),
        PercussionInstrument("Ride Cymbal 1", 51),
        PercussionInstrument("Scratch Pull", 30),
        PercussionInstrument("Scratch Push", 29),
        PercussionInstrument("Shaker", 82),
        PercussionInstrument("Side Stick", 37),
        PercussionInstrument("Slap Noise", 28),
        PercussionInstrument("Splash Cymbal", 55),
        PercussionInstrument("Square Click", 32),
        PercussionInstrument("Tambourine", 54),
    });
    return all_percussions;
  }

  [[nodiscard]] static auto get_field_name() { return "percussion_instrument"; }
};

Q_DECLARE_METATYPE(const PercussionInstrument *);

struct Program : public Named {
  short bank_number;
  short preset_number;

  Program(const char *const name, const short bank_number_input,
          const short preset_number_input)
      : Named(name), bank_number(bank_number_input),
        preset_number(preset_number_input){};
};

template <std::derived_from<Program> SubProgram>
[[nodiscard]] static auto get_programs(const bool is_percussion) {
  auto *const settings_pointer = new_fluid_settings();
  auto *const synth_pointer = new_fluid_synth(settings_pointer);

  fluid_sfont_t *const soundfont_pointer = fluid_synth_get_sfont_by_id(
      synth_pointer, get_soundfont_id(synth_pointer));
  Q_ASSERT(soundfont_pointer != nullptr);

  fluid_sfont_iteration_start(soundfont_pointer);
  auto *preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);
  static const std::set<QString> skip_names(
      {"Marching Snare", "OldMarchingBass", "Marching Cymbals", "Marching Bass",
       "OldMarchingTenor", "Marching Tenor",
       // dummy programs
       "Basses Fast", "Basses Pizzicato", "Basses Slow", "Basses Tremolo",
       "Celli Fast", "Celli Pizzicato", "Celli Slow", "Celli Tremolo",
       "Violas Fast", "Violas Pizzicato", "Violas Slow", "Violas Tremolo",
       "Violins Fast", "Violins Pizzicato", "Violins Slow", "Violins Tremolo",
       "Violins2 Fast", "Violins2 Pizzicato", "Violins2 Slow",
       "Violins2 Tremolo",
       // expressive programs
       "5th Saw Wave Expr.", "Accordion Expr.", "Alto Sax Expr.",
       "Atmosphere Expr.", "Bandoneon Expr.", "Baritone Sax Expr.",
       "Bass & Lead Expr.", "Basses Fast Expr.", "Basses Slow Expr.",
       "Basses Trem Expr.", "Bassoon Expr.", "Bottle Chiff Expr.",
       "Bowed Glass Expr.", "Brass 2 Expr.", "Brass Section Expr.",
       "Calliope Lead Expr.", "Celli Fast Expr.", "Celli Slow Expr.",
       "Celli Trem Expr.", "Cello Expr.", "Charang Expr.", "Chiffer Lead Expr.",
       "Choir Aahs Expr.", "Church Organ 2 Expr.", "Church Organ Expr.",
       "Clarinet Expr.", "Contrabass Expr.", "Detuned Org. 1 Expr.",
       "Detuned Org. 2 Expr.", "Detuned Saw Expr.", "Drawbar Organ Expr.",
       "Echo Drops Expr.", "English Horn Expr.", "Fiddle Expr.", "Flute Expr.",
       "French Horns Expr.", "Goblin Expr.", "Halo Pad Expr.",
       "Harmonica Expr.", "Hmn. Mute Tpt. Expr.", "It. Accordion Expr.",
       "Metal Pad Expr.", "Oboe Expr.", "Ocarina Expr.", "Pan Flute Expr.",
       "Perc. Organ Expr.", "Piccolo Expr.", "Polysynth Expr.",
       "Recorder Expr.", "Reed Organ Expr.", "Rock Organ Expr.",
       "Saw Lead Expr.", "Shakuhachi Expr.", "Shenai Expr.", "Sine Wave Expr.",
       "Slow Violin Expr.", "Solo Vox Expr.", "Soprano Sax Expr.",
       "Soundtrack Expr.", "Space Voice Expr.", "Square Lead Expr.",
       "Star Theme Expr.", "Strings Fast Expr.", "Strings Slow Expr.",
       "Strings Trem Expr.", "Sweep Pad Expr.", "Syn. Strings 1 Expr.",
       "Syn. Strings 2 Expr.", "Synth Brass 1 Expr.", "Synth Brass 2 Expr.",
       "Synth Brass 3 Expr.", "Synth Brass 4 Expr.", "Synth Strings3 Expr.",
       "Synth Voice Expr.", "Tenor Sax Expr.", "Trombone Expr.",
       "Trumpet Expr.", "Tuba Expr.", "Viola Expr.", "Violas Fast Expr.",
       "Violas Slow Expr.", "Violas Trem Expr.", "Violin Expr.",
       "Violins Fast Expr.", "Violins Slow Expr.", "Violins Trem Expr.",
       "Violins2 Fast Expr.", "Violins2 Slow Expr.", "Violins2 Trem Expr.",
       "Voice Oohs Expr.", "Warm Pad Expr.", "Whistle Expr.",
       // not working?
       "Temple Blocks"});

  static const std::set<QString> percussion_set_names(
      {"Brush 1",    "Brush 2",    "Brush",      "Electronic", "Jazz 1",
       "Jazz 2",     "Jazz 3",     "Jazz 4",     "Jazz",       "Orchestra Kit",
       "Power 1",    "Power 2",    "Power 3",    "Power",      "Room 1",
       "Room 2",     "Room 3",     "Room 4",     "Room 5",     "Room 6",
       "Room 7",     "Room",       "Standard 1", "Standard 2", "Standard 3",
       "Standard 4", "Standard 5", "Standard 6", "Standard 7", "Standard",
       "TR-808"});

  QList<SubProgram> programs;
  while (preset_pointer != nullptr) {
    const auto *const name = fluid_preset_get_name(preset_pointer);
    if (!skip_names.contains(name) &&
        is_percussion == percussion_set_names.contains(name)) {
      programs.push_back(SubProgram(
          name, static_cast<short>(fluid_preset_get_banknum(preset_pointer)),
          static_cast<short>(fluid_preset_get_num(preset_pointer))));
    }
    preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);
  }

  delete_fluid_synth(synth_pointer);
  delete_fluid_settings(settings_pointer);

  std::sort(programs.begin(), programs.end(),
            [](const SubProgram &instrument_1, const SubProgram &instrument_2) {
              return instrument_1.name <= instrument_2.name;
            });

  return programs;
}

struct PercussionSet : public Program {
  PercussionSet(const char *const name, const short bank_number,
                const short preset_number)
      : Program(name, bank_number, preset_number){};

  [[nodiscard]] static auto get_all_nameds() -> const QList<PercussionSet> & {
    static const auto all_percussion_sets = get_programs<PercussionSet>(true);
    return all_percussion_sets;
  }

  [[nodiscard]] static auto get_field_name() { return "percussion_set"; };
};

Q_DECLARE_METATYPE(const PercussionSet *);

struct Instrument : public Program {
  Instrument(const char *const name, const short bank_number,
             const short preset_number)
      : Program(name, bank_number, preset_number){};

  [[nodiscard]] static auto get_all_nameds() -> const QList<Instrument> & {
    static const auto all_instruments = get_programs<Instrument>(false);
    return all_instruments;
  }

  [[nodiscard]] static auto get_field_name() { return "instrument"; };
};

Q_DECLARE_METATYPE(const Instrument *);

// a sub rational should have the following method:
// SubRational(const nlohmann::json &json_rational);
struct AbstractRational {
  int numerator = 1;
  int denominator = 1;

  AbstractRational() = default;
  AbstractRational(const int numerator_input, const int denominator_input)
      : numerator(numerator_input), denominator(denominator_input) {}

  explicit AbstractRational(const nlohmann::json &json_rational)
      : numerator(json_rational.value("numerator", 1)),
        denominator(json_rational.value("denominator", 1)) {}

  virtual ~AbstractRational() = default;

  [[nodiscard]] auto operator==(const AbstractRational &other_rational) const {
    return numerator == other_rational.numerator &&
           denominator == other_rational.denominator;
  }

  [[nodiscard]] virtual auto is_default() const -> bool {
    return numerator == 1 && denominator == 1;
  }

  [[nodiscard]] virtual auto to_double() const -> double {
    Q_ASSERT(denominator != 0);
    return 1.0 * numerator / denominator;
  }

  virtual void to_json(nlohmann::json &json_rational) const {
    add_int_to_json(json_rational, "numerator", numerator, 1);
    add_int_to_json(json_rational, "denominator", denominator, 1);
  }
};

template <std::derived_from<AbstractRational> SubRational>
[[nodiscard]] static auto
json_field_to_abstract_rational(const nlohmann::json &json_row,
                                const char *const field_name) {
  Q_ASSERT(json_row.is_object());

  if (json_row.contains(field_name)) {
    return SubRational(json_row[field_name]);
  }
  return SubRational();
}

static void add_abstract_rational_to_json(nlohmann::json &json_row,
                                          const AbstractRational &rational,
                                          const char *const column_name) {
  Q_ASSERT(json_row.is_object());

  if (!rational.is_default()) {
    nlohmann::json json_rational = nlohmann::json::object();
    rational.to_json(json_rational);
    json_row[column_name] = std::move(json_rational);
  }
}

struct Rational : public AbstractRational {
  Rational() = default;

  Rational(const int numerator, const int denominator)
      : AbstractRational(numerator, denominator) {}

  explicit Rational(const nlohmann::json &json_rational)
      : AbstractRational(json_rational) {}
};

Q_DECLARE_METATYPE(Rational);

struct Interval : public AbstractRational {
  int octave = 0;

  Interval() = default;

  Interval(const int numerator, const int denominator, const int octave_input)
      : AbstractRational(numerator, denominator), octave(octave_input) {}

  explicit Interval(const nlohmann::json &json_rational)
      : AbstractRational(json_rational),
        octave(json_rational.value("octave", 0)) {}

  [[nodiscard]] auto operator==(const Interval &other_interval) const {
    return AbstractRational::operator==(other_interval) &&
           octave == other_interval.octave;
  }

  [[nodiscard]] auto is_default() const -> bool override {
    return AbstractRational::is_default() && octave == 0;
  }

  [[nodiscard]] auto to_double() const -> double override {
    return AbstractRational::to_double() * pow(OCTAVE_RATIO, octave);
  }

  void to_json(nlohmann::json &json_interval) const override {
    AbstractRational::to_json(json_interval);
    add_int_to_json(json_interval, "octave", octave, 0);
  }
};

Q_DECLARE_METATYPE(Interval);

static void add_pitched_fields_to_schema(nlohmann::json &schema) {
  add_named_schema<Instrument>(schema);
  auto interval_fields_schema = get_rational_fields_schema();
  interval_fields_schema["octave"] =
      get_number_schema("integer", -MAX_OCTAVE, MAX_OCTAVE);
  schema["interval"] = get_object_schema(interval_fields_schema);
}

static void add_unpitched_fields_to_schema(nlohmann::json &schema) {
  add_named_schema<PercussionSet>(schema);
  add_named_schema<PercussionInstrument>(schema);
}

template <std::derived_from<Named> SubNamed>
struct NamedEditor : public QComboBox {
  explicit NamedEditor(QWidget *const parent_pointer)
      : QComboBox(parent_pointer) {
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
    // force scrollbar for combo box
    setStyleSheet("combobox-popup: 0;");
  }

  [[nodiscard]] auto value() const -> const SubNamed * {
    const auto row = currentIndex();
    if (row == 0) {
      return nullptr;
    }
    return &SubNamed::get_all_nameds().at(row - 1);
  }

  void setValue(const SubNamed *new_value) {
    setCurrentIndex(new_value == nullptr
                        ? 0
                        : static_cast<int>(std::distance(
                              SubNamed::get_all_nameds().data(), new_value)) +
                              1);
  }
};

struct InstrumentEditor : public NamedEditor<Instrument> {
  Q_OBJECT
  Q_PROPERTY(const Instrument *value READ value WRITE setValue USER true)
public:
  explicit InstrumentEditor(QWidget *const parent_pointer)
      : NamedEditor<Instrument>(parent_pointer) {}
};

struct PercussionSetEditor : public NamedEditor<PercussionSet> {
  Q_OBJECT
  Q_PROPERTY(const PercussionSet *value READ value WRITE setValue USER true)
public:
  explicit PercussionSetEditor(QWidget *const parent_pointer_input)
      : NamedEditor<PercussionSet>(parent_pointer_input) {}
};

struct PercussionInstrumentEditor : public NamedEditor<PercussionInstrument> {
  Q_OBJECT
  Q_PROPERTY(
      const PercussionInstrument *value READ value WRITE setValue USER true)
public:
  explicit PercussionInstrumentEditor(QWidget *const parent_pointer)
      : NamedEditor<PercussionInstrument>(parent_pointer) {}
};

struct AbstractRationalEditor : public QFrame {
  QSpinBox &numerator_box = *(new QSpinBox);
  QSpinBox &denominator_box = *(new QSpinBox);
  QHBoxLayout &row_layout = *(new QHBoxLayout(this));

  explicit AbstractRationalEditor(QWidget *const parent_pointer)
      : QFrame(parent_pointer) {
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
    row_layout.setContentsMargins(1, 0, 1, 0);
  }

  void setValue(const AbstractRational &new_value) const {
    numerator_box.setValue(new_value.numerator);
    denominator_box.setValue(new_value.denominator);
  }
};

struct IntervalEditor : public AbstractRationalEditor {
  Q_OBJECT
  Q_PROPERTY(Interval interval READ value WRITE setValue USER true)

public:
  QSpinBox &octave_box = *(new QSpinBox);

  explicit IntervalEditor(QWidget *const parent_pointer)
      : AbstractRationalEditor(parent_pointer) {
    octave_box.setMinimum(-MAX_OCTAVE);
    octave_box.setMaximum(MAX_OCTAVE);

    row_layout.addWidget(
        new QLabel("o")); // NOLINT(cppcoreguidelines-owning-memory)
    row_layout.addWidget(&octave_box);
  }

  [[nodiscard]] auto value() const {
    return Interval(numerator_box.value(), denominator_box.value(),
                    octave_box.value());
  }

  void setValue(const Interval &new_value) const {
    AbstractRationalEditor::setValue(new_value);
    octave_box.setValue(new_value.octave);
  }
};

struct RationalEditor : public AbstractRationalEditor {
  Q_OBJECT
  Q_PROPERTY(Rational rational READ value WRITE setValue USER true)

public:
  explicit RationalEditor(QWidget *const parent_pointer)
      : AbstractRationalEditor(parent_pointer) {}

  [[nodiscard]] auto value() const {
    return Rational(numerator_box.value(), denominator_box.value());
  }
};

[[nodiscard]] static auto get_row_fields_schema() {
  return nlohmann::json(
      {{"beats", get_object_schema(get_rational_fields_schema())},
       {"velocity_ratio", get_object_schema(get_rational_fields_schema())},
       {"words", nlohmann::json({{"type", "string"}})}});
}

// In addition to the following, a sub-row should have the following methods:
// SubRow(const nlohmann::json& json_row);
// static auto is_column_editable(int column_number) -> bool;
// (optional)
// static auto get_column_name(int column_number) -> QString;
// static auto get_number_of_columns() -> int;
// static auto get_fields_schema() -> nlohmann::json;
// static auto get_plural_field_for() -> const char *;
// void void copy_column_from(const SubRow &template_row, int column_number);
struct Row {
  Rational beats;
  Rational velocity_ratio;
  QString words;

  Row() = default;

  explicit Row(const nlohmann::json &json_chord)
      : beats(json_field_to_abstract_rational<Rational>(json_chord, "beats")),
        velocity_ratio(json_field_to_abstract_rational<Rational>(
            json_chord, "velocity_ratio")),
        words([](const nlohmann::json &json_row) {
          Q_ASSERT(json_row.is_object());
          if (json_row.contains("words")) {
            return QString::fromStdString(json_row["words"]);
          }
          return QString("");
        }(json_chord)) {}

  virtual ~Row() = default;

  [[nodiscard]] static auto is_column_editable(const int /*column_number*/) {
    return true;
  }

  [[nodiscard]] virtual auto get_data(int column_number) const -> QVariant = 0;

  virtual void set_data(int column, const QVariant &new_value) = 0;
  virtual void column_to_json(nlohmann::json &json_row,
                              int column_number) const = 0;
};

[[nodiscard]] static auto
row_to_json(const Row &row, const int left_column,
            const int right_column) -> nlohmann::json {
  auto json_row = nlohmann::json::object();
  for (auto column_number = left_column; column_number <= right_column;
       column_number++) {
    row.column_to_json(json_row, column_number);
  }
  return json_row;
}

template <std::derived_from<Row> SubRow>
static void partial_json_to_rows(QList<SubRow> &new_rows,
                                 const nlohmann::json &json_rows,
                                 const int number_of_rows) {
  Q_ASSERT(json_rows.is_array());
  std::transform(
      json_rows.cbegin(), json_rows.cbegin() + number_of_rows,
      std::back_inserter(new_rows),
      [](const nlohmann::json &json_row) { return SubRow(json_row); });
}

template <std::derived_from<Row> SubRow>
static void json_to_rows(QList<SubRow> &rows, const nlohmann::json &json_rows) {
  partial_json_to_rows(rows, json_rows, static_cast<int>(json_rows.size()));
}

template <std::derived_from<Row> SubRow>
static void add_rows_to_json(nlohmann::json &json_chord,
                             const QList<SubRow> &rows) {
  Q_ASSERT(json_chord.is_object());
  if (!rows.empty()) {
    nlohmann::json json_rows = nlohmann::json::array();
    std::transform(rows.cbegin(), rows.cend(), std::back_inserter(json_rows),
                   [](const SubRow &row) -> nlohmann::json {
                     return row_to_json(row, 0,
                                        row.get_number_of_columns() - 1);
                   });
    json_chord[SubRow::get_plural_field_for()] = std::move(json_rows);
  }
}

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto
json_field_to_rows(const nlohmann::json &json_object) {
  Q_ASSERT(json_object.is_object());
  const char *const field = SubRow::get_plural_field_for();
  if (json_object.contains(field)) {
    QList<SubRow> rows;
    const auto &json_rows = json_object[field];
    json_to_rows(rows, json_rows);
    return rows;
  }
  return QList<SubRow>();
}

template <std::derived_from<Row> SubRow>
static void add_row_array_schema(nlohmann::json &schema) {
  Q_ASSERT(schema.is_object());
  schema[SubRow::get_plural_field_for()] = nlohmann::json(
      {{"type", "array"},
       {"items", get_object_schema(SubRow::get_fields_schema())}});
}

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto get_cells_mime() -> const QString & {
  static const auto cells_mime = [] {
    QString cells_mime;
    QTextStream stream(&cells_mime);
    stream << "application/prs." << SubRow::get_plural_field_for()
           << "_cells+json";
    return cells_mime;
  }();
  return cells_mime;
}

[[nodiscard]] static auto get_milliseconds(const double beats_per_minute,
                                           const Row &row) {
  return row.beats.to_double() * MILLISECONDS_PER_MINUTE / beats_per_minute;
}

// A subnote should have the following method:
// static auto get_note_type() -> const char*;
struct Note : Row {
  Note() = default;
  explicit Note(const nlohmann::json &json_note) : Row(json_note){};

  [[nodiscard]] virtual auto get_closest_midi(
      QWidget &parent, fluid_sequencer_t *sequencer_pointer,
      fluid_event_t *event_pointer, double current_time, double current_key,
      const PercussionInstrument *current_percussion_instrument_pointer,
      int channel_number, int chord_number, int note_number) const -> short = 0;

  [[nodiscard]] virtual auto
  get_program(QWidget &parent, const Instrument *current_instrument_pointer,
              const PercussionSet *current_percussion_set_pointer,
              int chord_number, int note_number) const -> const Program & = 0;
};

template <std::derived_from<Note> SubNote>
static void add_note_location(QTextStream &stream, const int chord_number,
                              const int note_number) {
  stream << QObject::tr(" for chord ") << chord_number + 1
         << QObject::tr(SubNote::get_note_type()) << note_number + 1;
}

template <std::derived_from<Note> SubNote, std::derived_from<Named> SubNamed>
[[nodiscard]] static auto
substitute_named_for(QWidget &parent, const SubNamed *sub_named_pointer,
                     const SubNamed *current_sub_named_pointer,
                     const char *const default_one, const int chord_number,
                     const int note_number, const char *const missing_title,
                     const char *const missing_message,
                     const char *const default_message) -> const SubNamed & {
  if (sub_named_pointer == nullptr) {
    sub_named_pointer = current_sub_named_pointer;
  };
  if (sub_named_pointer == nullptr) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr(missing_message);
    add_note_location<SubNote>(stream, chord_number, note_number);
    stream << QObject::tr(default_message);
    show_warning(parent, missing_title, message);
    sub_named_pointer = &get_by_name<SubNamed>(default_one);
  }
  return *sub_named_pointer;
}

struct UnpitchedNote : Note {
  const PercussionSet *percussion_set_pointer = nullptr;
  const PercussionInstrument *percussion_instrument_pointer = nullptr;

  UnpitchedNote() = default;

  explicit UnpitchedNote(const nlohmann::json &json_note)
      : Note(json_note),
        percussion_set_pointer(
            json_field_to_named_pointer<PercussionSet>(json_note)),
        percussion_instrument_pointer(
            json_field_to_named_pointer<PercussionInstrument>(json_note)) {}

  [[nodiscard]] auto get_closest_midi(
      QWidget &parent, fluid_sequencer_t *const /*sequencer_pointer*/,
      fluid_event_t *const /*event_pointer*/, const double /*current_time*/,
      const double /*current_key*/,
      const PercussionInstrument *current_percussion_instrument_pointer,
      const int /*channel_number*/, const int chord_number,
      const int note_number) const -> short override {
    return substitute_named_for<UnpitchedNote>(
               parent, percussion_instrument_pointer,
               current_percussion_instrument_pointer, "Tambourine",
               chord_number, note_number, "Percussion instrument error",
               "No percussion instrument", ". Using Tambourine.")
        .midi_number;
  };

  [[nodiscard]] auto
  get_program(QWidget &parent,
              const Instrument *const /*current_instrument_pointer*/,
              const PercussionSet *const current_percussion_set_pointer,
              const int chord_number,
              const int note_number) const -> const Program & override {
    return substitute_named_for<UnpitchedNote>(
        parent, percussion_set_pointer, current_percussion_set_pointer,
        "Marimba", chord_number, note_number, "Percussion set error",
        "No percussion set", ". Using Standard.");
  };

  [[nodiscard]] static auto get_fields_schema() {
    auto schema = get_row_fields_schema();
    add_unpitched_fields_to_schema(schema);
    return schema;
  }

  [[nodiscard]] static auto get_note_type() { return ", unpitched note "; }

  [[nodiscard]] static auto get_plural_field_for() { return "unpitched_notes"; }

  [[nodiscard]] static auto get_column_editor_size(const int column_number) {
    switch (column_number) {
    case unpitched_note_percussion_set_column:
      return get_default_size<PercussionSetEditor>();
    case unpitched_note_percussion_instrument_column:
      return get_default_size<PercussionInstrumentEditor>();
    case unpitched_note_beats_column:
    case unpitched_note_velocity_ratio_column:
      return get_default_size<RationalEditor>();
    case unpitched_note_words_column:
      return QSize();
    default:
      Q_ASSERT(false);
      return QSize();
    }
  }

  [[nodiscard]] static auto get_column_name(const int column_number) {
    switch (column_number) {
    case unpitched_note_percussion_set_column:
      return "Percussion set";
    case unpitched_note_percussion_instrument_column:
      return "Percussion instrument";
    case unpitched_note_beats_column:
      return "Beats";
    case unpitched_note_velocity_ratio_column:
      return "Velocity ratio";
    case unpitched_note_words_column:
      return "Words";
    default:
      Q_ASSERT(false);
      return "";
    }
  }

  [[nodiscard]] static auto get_number_of_columns() {
    return number_of_unpitched_note_columns;
  }

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override {
    switch (column_number) {
    case unpitched_note_percussion_set_column:
      return QVariant::fromValue(percussion_set_pointer);
    case unpitched_note_percussion_instrument_column:
      return QVariant::fromValue(percussion_instrument_pointer);
    case unpitched_note_beats_column:
      return QVariant::fromValue(beats);
    case unpitched_note_velocity_ratio_column:
      return QVariant::fromValue(velocity_ratio);
    case unpitched_note_words_column:
      return words;
    default:
      Q_ASSERT(false);
      return {};
    }
  }

  void set_data(const int column_number, const QVariant &new_value) override {
    switch (column_number) {
    case unpitched_note_percussion_set_column:
      percussion_set_pointer = variant_to<const PercussionSet *>(new_value);
      break;
    case unpitched_note_percussion_instrument_column:
      percussion_instrument_pointer =
          variant_to<const PercussionInstrument *>(new_value);
      break;
    case unpitched_note_beats_column:
      beats = variant_to<Rational>(new_value);
      break;
    case unpitched_note_velocity_ratio_column:
      velocity_ratio = variant_to<Rational>(new_value);
      break;
    case unpitched_note_words_column:
      words = variant_to<QString>(new_value);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void copy_column_from(const UnpitchedNote &template_row,
                        const int column_number) {
    switch (column_number) {
    case unpitched_note_percussion_set_column:
      percussion_set_pointer = template_row.percussion_set_pointer;
      break;
    case unpitched_note_percussion_instrument_column:
      percussion_instrument_pointer =
          template_row.percussion_instrument_pointer;
      break;
    case unpitched_note_beats_column:
      beats = template_row.beats;
      break;
    case unpitched_note_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    case unpitched_note_words_column:
      words = template_row.words;
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void column_to_json(nlohmann::json &json_percussion,
                      const int column_number) const override {
    switch (column_number) {
    case unpitched_note_percussion_set_column:
      add_named_to_json(json_percussion, percussion_set_pointer);
      break;
    case unpitched_note_percussion_instrument_column:
      add_named_to_json(json_percussion, percussion_instrument_pointer);
      break;
    case unpitched_note_beats_column:
      add_abstract_rational_to_json(json_percussion, beats, "beats");
      break;
    case unpitched_note_velocity_ratio_column:
      add_abstract_rational_to_json(json_percussion, velocity_ratio,
                                    "velocity_ratio");
      break;
    case unpitched_note_words_column:
      add_words_to_json(json_percussion, words);
      break;
    default:
      Q_ASSERT(false);
    }
  }
};

struct PitchedNote : Note {
  const Instrument *instrument_pointer = nullptr;
  Interval interval;

  PitchedNote() = default;

  explicit PitchedNote(const nlohmann::json &json_note)
      : Note(json_note),
        instrument_pointer(json_field_to_named_pointer<Instrument>(json_note)),
        interval(
            json_field_to_abstract_rational<Interval>(json_note, "interval")) {}

  [[nodiscard]] auto
  get_closest_midi(QWidget & /*parent*/,
                   fluid_sequencer_t *const sequencer_pointer,
                   fluid_event_t *const event_pointer,
                   const double current_time, const double current_key,
                   const PercussionInstrument
                       *const /*current_percussion_instrument_pointer*/,
                   const int channel_number, const int /*chord_number*/,
                   const int /*note_number*/) const -> short override {
    const auto midi_float = get_midi(current_key * interval.to_double());
    const auto closest_midi = static_cast<short>(round(midi_float));

    fluid_event_pitch_bend(
        event_pointer, channel_number,
        to_int((midi_float - closest_midi + ZERO_BEND_HALFSTEPS) *
               BEND_PER_HALFSTEP));
    send_event_at(sequencer_pointer, event_pointer, current_time);
    return closest_midi;
  }

  [[nodiscard]] auto
  get_program(QWidget &parent, const Instrument *current_instrument_pointer,
              const PercussionSet * /*current_percussion_set_pointer*/,
              const int chord_number,
              const int note_number) const -> const Program & override {
    return substitute_named_for<PitchedNote>(
        parent, instrument_pointer, current_instrument_pointer, "Marimba",
        chord_number, note_number, "Instrument error", "No instrument",
        ". Using Marimba.");
  }

  [[nodiscard]] static auto get_fields_schema() {
    auto schema = get_row_fields_schema();
    add_pitched_fields_to_schema(schema);
    return schema;
  }

  [[nodiscard]] static auto get_plural_field_for() { return "pitched_notes"; }

  [[nodiscard]] static auto get_column_editor_size(int column_number) {
    switch (column_number) {
    case pitched_note_instrument_column:
      return get_default_size<InstrumentEditor>();
    case pitched_note_interval_column:
      return get_default_size<IntervalEditor>();
    case pitched_note_beats_column:
    case pitched_note_velocity_ratio_column:
      return get_default_size<RationalEditor>();
    case pitched_note_words_column:
      return QSize();
    default:
      Q_ASSERT(false);
      return QSize();
    }
  }

  [[nodiscard]] static auto get_note_type() { return ", pitched note "; }

  [[nodiscard]] static auto get_column_name(const int column_number) {
    switch (column_number) {
    case pitched_note_instrument_column:
      return "Instrument";
    case pitched_note_interval_column:
      return "Interval";
    case pitched_note_beats_column:
      return "Beats";
    case pitched_note_velocity_ratio_column:
      return "Velocity ratio";
    case pitched_note_words_column:
      return "Words";
    default:
      Q_ASSERT(false);
      return "";
    }
  }

  [[nodiscard]] static auto get_number_of_columns() {
    return number_of_pitched_note_columns;
  }

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override {
    switch (column_number) {
    case pitched_note_instrument_column:
      return QVariant::fromValue(instrument_pointer);
    case pitched_note_interval_column:
      return QVariant::fromValue(interval);
    case pitched_note_beats_column:
      return QVariant::fromValue(beats);
    case pitched_note_velocity_ratio_column:
      return QVariant::fromValue(velocity_ratio);
    case pitched_note_words_column:
      return words;
    default:
      Q_ASSERT(false);
      return {};
    }
  }

  void set_data(const int column_number, const QVariant &new_value) override {
    switch (column_number) {
    case pitched_note_instrument_column:
      instrument_pointer = variant_to<const Instrument *>(new_value);
      break;
    case pitched_note_interval_column:
      interval = variant_to<Interval>(new_value);
      break;
    case pitched_note_beats_column:
      beats = variant_to<Rational>(new_value);
      break;
    case pitched_note_velocity_ratio_column:
      velocity_ratio = variant_to<Rational>(new_value);
      break;
    case pitched_note_words_column:
      words = variant_to<QString>(new_value);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void copy_column_from(const PitchedNote &template_row,
                        const int column_number) {
    switch (column_number) {
    case pitched_note_instrument_column:
      instrument_pointer = template_row.instrument_pointer;
      break;
    case pitched_note_interval_column:
      interval = template_row.interval;
      break;
    case pitched_note_beats_column:
      beats = template_row.beats;
      break;
    case pitched_note_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    case pitched_note_words_column:
      words = template_row.words;
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void column_to_json(nlohmann::json &json_note,
                      const int column_number) const override {
    switch (column_number) {
    case pitched_note_instrument_column:
      add_named_to_json(json_note, instrument_pointer);
      break;
    case pitched_note_interval_column:
      add_abstract_rational_to_json(json_note, interval, "interval");
      break;
    case pitched_note_beats_column:
      add_abstract_rational_to_json(json_note, beats, "beats");
      break;
    case pitched_note_velocity_ratio_column:
      add_abstract_rational_to_json(json_note, velocity_ratio,
                                    "velocity_ratio");
      break;
    case pitched_note_words_column:
      add_words_to_json(json_note, words);
      break;
    default:
      Q_ASSERT(false);
      break;
    }
  }
};

struct Chord : public Row {
  const Instrument *instrument_pointer = nullptr;
  const PercussionSet *percussion_set_pointer = nullptr;
  const PercussionInstrument *percussion_instrument_pointer = nullptr;
  Interval interval;
  Rational tempo_ratio;
  QList<PitchedNote> pitched_notes;
  QList<UnpitchedNote> unpitched_notes;

  Chord() = default;

  explicit Chord(const nlohmann::json &json_chord)
      : Row(json_chord),
        instrument_pointer(json_field_to_named_pointer<Instrument>(json_chord)),
        percussion_set_pointer(
            json_field_to_named_pointer<PercussionSet>(json_chord)),
        percussion_instrument_pointer(
            json_field_to_named_pointer<PercussionInstrument>(json_chord)),
        interval(
            json_field_to_abstract_rational<Interval>(json_chord, "interval")),
        tempo_ratio(json_field_to_abstract_rational<Rational>(json_chord,
                                                              "tempo_ratio")),
        pitched_notes(json_field_to_rows<PitchedNote>(json_chord)),
        unpitched_notes(json_field_to_rows<UnpitchedNote>(json_chord)) {}

  [[nodiscard]] static auto get_fields_schema() {
    auto schema = get_row_fields_schema();
    add_pitched_fields_to_schema(schema);
    add_unpitched_fields_to_schema(schema);
    schema["tempo_ratio"] = get_object_schema(get_rational_fields_schema());
    add_row_array_schema<PitchedNote>(schema);
    add_row_array_schema<UnpitchedNote>(schema);
    return schema;
  }

  [[nodiscard]] static auto get_plural_field_for() { return "chords"; }

  [[nodiscard]] static auto get_column_editor_size(const int column_number) {
    switch (column_number) {
    case chord_instrument_column:
      return get_default_size<InstrumentEditor>();
    case chord_percussion_set_column:
      return get_default_size<PercussionSetEditor>();
    case chord_percussion_instrument_column:
      return get_default_size<PercussionInstrumentEditor>();
    case chord_interval_column:
      return get_default_size<IntervalEditor>();
    case chord_beats_column:
    case chord_velocity_ratio_column:
    case chord_tempo_ratio_column:
      return get_default_size<RationalEditor>();
    case chord_words_column:
    case chord_pitched_notes_column:
    case chord_unpitched_notes_column:
      return QSize();
    default:
      Q_ASSERT(false);
      return QSize();
    }
  }

  [[nodiscard]] static auto get_column_name(const int column_number) {
    switch (column_number) {
    case chord_instrument_column:
      return "Instrument";
    case chord_percussion_set_column:
      return "Percussion set";
    case chord_percussion_instrument_column:
      return "Percussion instrument";
    case chord_interval_column:
      return "Interval";
    case chord_beats_column:
      return "Beats";
    case chord_velocity_ratio_column:
      return "Velocity ratio";
    case chord_tempo_ratio_column:
      return "Tempo ratio";
    case chord_words_column:
      return "Words";
    case chord_pitched_notes_column:
      return "Pitched notes";
    case chord_unpitched_notes_column:
      return "Unpitched notes";
    default:
      Q_ASSERT(false);
      return "";
    }
  }

  [[nodiscard]] static auto get_number_of_columns() {
    return number_of_chord_columns;
  }

  [[nodiscard]] static auto is_column_editable(const int column_number) {
    return column_number != chord_pitched_notes_column &&
           column_number != chord_unpitched_notes_column;
  }

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override {
    switch (column_number) {
    case chord_instrument_column:
      return QVariant::fromValue(instrument_pointer);
    case chord_percussion_set_column:
      return QVariant::fromValue(percussion_set_pointer);
    case chord_percussion_instrument_column:
      return QVariant::fromValue(percussion_instrument_pointer);
    case chord_interval_column:
      return QVariant::fromValue(interval);
    case chord_beats_column:
      return QVariant::fromValue(beats);
    case chord_velocity_ratio_column:
      return QVariant::fromValue(velocity_ratio);
    case chord_tempo_ratio_column:
      return QVariant::fromValue(tempo_ratio);
    case chord_words_column:
      return words;
    case chord_pitched_notes_column:
      return pitched_notes.size();
    case chord_unpitched_notes_column:
      return unpitched_notes.size();
    default:
      Q_ASSERT(false);
      return {};
    }
  }

  void set_data(const int column_number, const QVariant &new_value) override {
    switch (column_number) {
    case chord_instrument_column:
      instrument_pointer = variant_to<const Instrument *>(new_value);
      break;
    case chord_percussion_set_column:
      percussion_set_pointer = variant_to<const PercussionSet *>(new_value);
      break;
    case chord_percussion_instrument_column:
      percussion_instrument_pointer =
          variant_to<const PercussionInstrument *>(new_value);
      break;
    case chord_interval_column:
      interval = variant_to<Interval>(new_value);
      break;
    case chord_beats_column:
      beats = variant_to<Rational>(new_value);
      break;
    case chord_velocity_ratio_column:
      velocity_ratio = variant_to<Rational>(new_value);
      break;
    case chord_tempo_ratio_column:
      tempo_ratio = variant_to<Rational>(new_value);
      break;
    case chord_words_column:
      words = variant_to<QString>(new_value);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void copy_column_from(const Chord &template_row, const int column_number) {
    switch (column_number) {
    case chord_instrument_column:
      instrument_pointer = template_row.instrument_pointer;
      break;
    case chord_percussion_set_column:
      percussion_set_pointer = template_row.percussion_set_pointer;
      break;
    case chord_percussion_instrument_column:
      percussion_instrument_pointer =
          template_row.percussion_instrument_pointer;
      break;
    case chord_interval_column:
      interval = template_row.interval;
      break;
    case chord_beats_column:
      beats = template_row.beats;
      break;
    case chord_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    case chord_tempo_ratio_column:
      tempo_ratio = template_row.tempo_ratio;
      break;
    case chord_words_column:
      words = template_row.words;
      break;
    case chord_pitched_notes_column:
      pitched_notes = template_row.pitched_notes;
      break;
    case chord_unpitched_notes_column:
      unpitched_notes = template_row.unpitched_notes;
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void column_to_json(nlohmann::json &json_chord,
                      const int column_number) const override {
    switch (column_number) {
    case chord_instrument_column:
      add_named_to_json(json_chord, instrument_pointer);
      break;
    case chord_percussion_set_column:
      add_named_to_json(json_chord, percussion_set_pointer);
      break;
    case chord_percussion_instrument_column:
      add_named_to_json(json_chord, percussion_instrument_pointer);
      break;
    case chord_interval_column:
      add_abstract_rational_to_json(json_chord, interval, "interval");
      break;
    case chord_beats_column:
      add_abstract_rational_to_json(json_chord, beats, "beats");
      break;
    case chord_velocity_ratio_column:
      add_abstract_rational_to_json(json_chord, velocity_ratio,
                                    "velocity_ratio");
      break;
    case chord_tempo_ratio_column:
      add_abstract_rational_to_json(json_chord, tempo_ratio, "tempo_ratio");
      break;
    case chord_words_column:
      add_words_to_json(json_chord, words);
      break;
    case chord_pitched_notes_column:
      add_rows_to_json(json_chord, pitched_notes);
      break;
    case chord_unpitched_notes_column:
      add_rows_to_json(json_chord, unpitched_notes);
      break;
    default:
      Q_ASSERT(false);
    }
  }
};

[[nodiscard]] static auto get_mime_description(const QString &mime_type) {
  if (mime_type == get_cells_mime<Chord>()) {
    return QObject::tr("chords cells");
  }
  if (mime_type == get_cells_mime<PitchedNote>()) {
    return QObject::tr("pitched notes cells");
  }
  if (mime_type == get_cells_mime<UnpitchedNote>()) {
    return QObject::tr("unpitched notes cells");
  }
  return mime_type;
}

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto
parse_clipboard(QWidget &parent) -> std::optional<nlohmann::json> {
  const auto &mime_data = get_const_reference(
      get_const_reference(QGuiApplication::clipboard()).mimeData());
  const auto &mime_type = get_cells_mime<SubRow>();
  if (!mime_data.hasFormat(mime_type)) {
    const auto formats = mime_data.formats();
    if (formats.empty()) {
      show_warning(parent, "Empty paste error",
                   QObject::tr("Nothing to paste!"));
      return {};
    };
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Cannot paste ") << get_mime_description(formats[0])
           << QObject::tr(" as ") << get_mime_description(mime_type);
    show_warning(parent, "MIME type error", message);
    return {};
  }
  const auto &copied_text = mime_data.data(mime_type).toStdString();
  nlohmann::json copied;
  try {
    copied = nlohmann::json::parse(copied_text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    show_warning(parent, "Parsing error", parse_error.what());
    return {};
  }
  if (copied.empty()) {
    show_warning(parent, "Empty paste", QObject::tr("Nothing to paste!"));
    return {};
  }
  static const auto cells_validator = []() {
    const auto last_column = SubRow::get_number_of_columns() - 1;
    nlohmann::json cells_schema(
        {{"left_column", get_number_schema("integer", 0, last_column)},
         {"right_column", get_number_schema("integer", 0, last_column)}});
    add_row_array_schema<SubRow>(cells_schema);
    return make_validator(nlohmann::json({"left_column", "right_column",
                                          SubRow::get_plural_field_for()}),
                          cells_schema);
  }();

  try {
    cells_validator.validate(copied);
  } catch (const std::exception &error) {
    show_warning(parent, "Schema error", error.what());
    return {};
  }
  return copied;
}

struct Song {
  double starting_key = DEFAULT_STARTING_KEY;
  double starting_velocity = DEFAULT_STARTING_VELOCITY;
  double starting_tempo = DEFAULT_STARTING_TEMPO;
  QList<Chord> chords;
};

[[nodiscard]] static auto get_key_text(const Song &song, const int chord_number,
                                       const double ratio = 1) {
  const auto &chords = song.chords;
  auto key = song.starting_key;
  for (auto previous_chord_number = 0; previous_chord_number <= chord_number;
       previous_chord_number++) {
    key = key * chords.at(previous_chord_number).interval.to_double();
  }
  key = key * ratio;
  const auto midi_float = get_midi(key);
  const auto closest_midi = to_int(midi_float);
  const auto difference_from_c = closest_midi - C_0_MIDI;
  const auto octave =
      difference_from_c / HALFSTEPS_PER_OCTAVE; // floor integer division
  const auto degree = difference_from_c - octave * HALFSTEPS_PER_OCTAVE;
  const auto cents = to_int((midi_float - closest_midi) * CENTS_PER_HALFSTEP);

  QString result;
  QTextStream stream(&result);
  stream << key << QObject::tr(" Hz; ") << QObject::tr([](int degree) {
    switch (degree) {
    case c_degree:
      return "C";
    case c_sharp_degree:
      return "C♯";
    case d_degree:
      return "D";
    case e_flat_degree:
      return "E♭";
    case e_degree:
      return "E";
    case f_degree:
      return "F";
    case f_sharp_degree:
      return "F♯";
    case g_degree:
      return "G";
    case a_flat_degree:
      return "A♭";
    case a_degree:
      return "A";
    case b_flat_degree:
      return "B♭";
    case b_degree:
      return "B";
    default:
      Q_ASSERT(false);
      return "";
    }
  }(degree))
         << octave;
  if (cents != 0) {
    stream << QObject::tr(cents >= 0 ? " + " : " − ") << abs(cents)
           << QObject::tr(" cents");
  }
  return result;
}

struct Player {
  // data
  QWidget &parent;

  // play state fields
  QList<double> channel_schedules = QList<double>(NUMBER_OF_MIDI_CHANNELS, 0);

  const Instrument *current_instrument_pointer = nullptr;
  const PercussionSet *current_percussion_set_pointer = nullptr;
  const PercussionInstrument *current_percussion_instrument_pointer = nullptr;

  double current_time = 0;
  double final_time = 0;

  double current_key = 0;
  double current_velocity = 0;
  double current_tempo = 0;

  fluid_settings_t *const settings_pointer = []() {
    fluid_settings_t *const settings_pointer = new_fluid_settings();
    Q_ASSERT(settings_pointer != nullptr);
    const auto cores = std::thread::hardware_concurrency();
    if (cores > 0) {
      set_fluid_int(settings_pointer, "synth.cpu-cores",
                    static_cast<int>(cores));
    }
#ifdef __linux__
    fluid_settings_setstr(settings_pointer, "audio.driver", "pulseaudio");
#endif
    return settings_pointer;
  }();
  fluid_event_t *const event_pointer = new_fluid_event();
  fluid_sequencer_t *const sequencer_pointer = new_fluid_sequencer2(0);
  fluid_synth_t *const synth_pointer = new_fluid_synth(settings_pointer);
  const unsigned int soundfont_id = get_soundfont_id(synth_pointer);
  const fluid_seq_id_t sequencer_id =
      fluid_sequencer_register_fluidsynth(sequencer_pointer, synth_pointer);

  fluid_audio_driver_t *audio_driver_pointer =
      make_audio_driver(parent, settings_pointer, synth_pointer);

  // methods
  explicit Player(QWidget &parent_input) : parent(parent_input) {
    fluid_event_set_dest(event_pointer, sequencer_id);
  }

  ~Player() {
    stop_playing(event_pointer, sequencer_pointer);
    delete_audio_driver(audio_driver_pointer);
    fluid_sequencer_unregister_client(sequencer_pointer, sequencer_id);
    delete_fluid_sequencer(sequencer_pointer);
    delete_fluid_event(event_pointer);
    delete_fluid_synth(synth_pointer);
    delete_fluid_settings(settings_pointer);
  }

  // prevent moving and copying
  Player(const Player &) = delete;
  auto operator=(const Player &) -> Player = delete;
  Player(Player &&) = delete;
  auto operator=(Player &&) -> Player = delete;
};

[[nodiscard]] static auto player_get_gain(const Player &player) {
  return fluid_synth_get_gain(player.synth_pointer);
}

static void modulate(Player &player, const Chord &chord) {
  player.current_key = player.current_key * chord.interval.to_double();
  player.current_velocity =
      player.current_velocity * chord.velocity_ratio.to_double();
  player.current_tempo = player.current_tempo * chord.tempo_ratio.to_double();
  const auto *chord_instrument_pointer = chord.instrument_pointer;
  if (chord_instrument_pointer != nullptr) {
    player.current_instrument_pointer = chord_instrument_pointer;
  }

  const auto *chord_percussion_set_pointer = chord.percussion_set_pointer;
  if (chord_percussion_set_pointer != nullptr) {
    player.current_percussion_set_pointer = chord_percussion_set_pointer;
  }

  const auto *chord_percussion_instrument_pointer =
      chord.percussion_instrument_pointer;
  if (chord_percussion_instrument_pointer != nullptr) {
    player.current_percussion_instrument_pointer =
        chord_percussion_instrument_pointer;
  }
}

static void update_final_time(Player &player, const double new_final_time) {
  if (new_final_time > player.final_time) {
    player.final_time = new_final_time;
  }
}

template <std::derived_from<Note> SubNote>
static void play_notes(Player &player, const int chord_number,
                       const QList<SubNote> &sub_notes,
                       const int first_note_number, const int number_of_notes) {
  auto &parent = player.parent;
  auto *const sequencer_pointer = player.sequencer_pointer;
  auto *const event_pointer = player.event_pointer;
  auto &channel_schedules = player.channel_schedules;
  const auto soundfont_id = player.soundfont_id;

  const auto current_time = player.current_time;
  const auto current_velocity = player.current_velocity;
  const auto current_tempo = player.current_tempo;
  const auto current_key = player.current_key;
  const auto *current_percussion_instrument_pointer =
      player.current_percussion_instrument_pointer;

  for (auto note_number = first_note_number;
       note_number < first_note_number + number_of_notes;
       note_number = note_number + 1) {
    auto channel_number = -1;
    for (auto channel_index = 0; channel_index < NUMBER_OF_MIDI_CHANNELS;
         channel_index = channel_index + 1) {
      if (current_time >= channel_schedules.at(channel_index)) {
        channel_number = channel_index;
      }
    }
    if (channel_number == -1) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Out of MIDI channels");
      add_note_location<SubNote>(stream, chord_number, note_number);
      stream << QObject::tr(". Not playing note.");
      show_warning(parent, "MIDI channel error", message);
      return;
    }
    const auto &sub_note = sub_notes.at(note_number);

    const Program &program = sub_note.get_program(
        player.parent, player.current_instrument_pointer,
        player.current_percussion_set_pointer, chord_number, note_number);

    fluid_event_program_select(event_pointer, channel_number, soundfont_id,
                               program.bank_number, program.preset_number);
    send_event_at(sequencer_pointer, event_pointer, current_time);

    const auto midi_number = sub_note.get_closest_midi(
        parent, sequencer_pointer, event_pointer, current_time, current_key,
        current_percussion_instrument_pointer, channel_number, chord_number,
        note_number);

    auto velocity = static_cast<short>(
        std::round(current_velocity * sub_note.velocity_ratio.to_double()));
    if (velocity > MAX_VELOCITY) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Velocity ") << velocity << QObject::tr(" exceeds ")
             << MAX_VELOCITY;
      add_note_location<SubNote>(stream, chord_number, note_number);
      stream << QObject::tr(". Playing with velocity ") << MAX_VELOCITY;
      show_warning(parent, "Velocity error", message);
      velocity = MAX_VELOCITY;
    }
    fluid_event_noteon(event_pointer, channel_number, midi_number, velocity);
    send_event_at(sequencer_pointer, event_pointer, current_time);

    const auto end_time =
        current_time + get_milliseconds(current_tempo, sub_note);

    fluid_event_noteoff(event_pointer, channel_number, midi_number);
    send_event_at(sequencer_pointer, event_pointer, end_time);

    channel_schedules[channel_number] = end_time;
  }
}

template <std::derived_from<Note> SubNote>
static void play_all_notes(Player &player, const int chord_number,
                           const QList<SubNote> &sub_notes) {
  play_notes(player, chord_number, sub_notes, 0,
             static_cast<int>(sub_notes.size()));
}

template <std::derived_from<Row> SubRow> struct SetCell;

template <std::derived_from<Row> SubRow>
struct RowsModel : public QAbstractTableModel {
  QList<SubRow> *rows_pointer = nullptr;
  QUndoStack &undo_stack;

  explicit RowsModel(QUndoStack &undo_stack_input)
      : undo_stack(undo_stack_input){};

  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent_index*/) const -> int override {
    return static_cast<int>(get_const_reference(rows_pointer).size());
  }

  [[nodiscard]] auto
  columnCount(const QModelIndex & /*parent_index*/) const -> int override {
    return SubRow::get_number_of_columns();
  }

  [[nodiscard]] auto headerData(const int section,
                                const Qt::Orientation orientation,
                                const int role) const -> QVariant override {
    if (role != Qt::DisplayRole) {
      return {};
    }
    switch (orientation) {
    case Qt::Horizontal:
      return QObject::tr(SubRow::get_column_name(section));
    case Qt::Vertical:
      return section + 1;
    default:
      Q_ASSERT(false);
      return {};
    }
  }

  [[nodiscard]] auto
  flags(const QModelIndex &index) const -> Qt::ItemFlags override {
    const auto uneditable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (SubRow::is_column_editable(index.column())) {
      return uneditable | Qt::ItemIsEditable;
    }
    return uneditable;
  }

  [[nodiscard]] virtual auto
  get_status(const int /*row_number*/) const -> QString {
    return "";
  }

  [[nodiscard]] auto data(const QModelIndex &index,
                          const int role) const -> QVariant override {
    Q_ASSERT(index.isValid());
    const auto row_number = index.row();

    if (role == Qt::StatusTipRole) {
      return get_status(row_number);
    }

    const auto column_number = index.column();

    if (role == Qt::SizeHintRole) {
      const auto &editor_size = SubRow::get_column_editor_size(column_number);
      if (!editor_size.isValid()) {
        return {};
      }
      return editor_size;
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
      return get_const_reference(rows_pointer)
          .at(row_number)
          .get_data(index.column());
    }

    return {};
  }

  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             const int role) -> bool override {
    if (role != Qt::EditRole) {
      return false;
    };
    undo_stack.push(
        new SetCell<SubRow>( // NOLINT(cppcoreguidelines-owning-memory)
            *this, index, new_value));
    return true;
  }

  // don't inline these functions because they use protected methods
  void set_cell(const QModelIndex &set_index, const QVariant &new_value) {
    const auto row_number = set_index.row();
    const auto column_number = set_index.column();

    get_reference(rows_pointer)[row_number].set_data(column_number, new_value);
    dataChanged(set_index, set_index);
  }

  void set_cells(const int first_row_number, const QList<SubRow> &new_rows,
                 const int left_column, const int right_column) {
    auto &rows = get_reference(rows_pointer);
    const auto number_of_new_rows = new_rows.size();
    for (auto replace_number = 0; replace_number < number_of_new_rows;
         replace_number++) {
      auto &row = rows[first_row_number + replace_number];
      const auto &new_row = new_rows.at(replace_number);
      for (auto column_number = left_column; column_number <= right_column;
           column_number++) {
        row.copy_column_from(new_row, column_number);
      }
    }
    dataChanged(index(first_row_number, left_column),
                index(first_row_number + number_of_new_rows - 1, right_column));
  }

  void delete_cells(const int first_row_number, const int number_of_rows,
                    const int left_column, const int right_column) {
    auto &rows = get_reference(rows_pointer);
    for (auto replace_number = 0; replace_number < number_of_rows;
         replace_number++) {
      auto &row = rows[first_row_number + replace_number];
      const SubRow empty_row;
      for (auto column_number = left_column; column_number <= right_column;
           column_number++) {
        row.copy_column_from(empty_row, column_number);
      }
    }
    dataChanged(index(first_row_number, left_column),
                index(first_row_number + number_of_rows - 1, right_column));
  }

  void insert_json_rows(const int first_row_number,
                        const nlohmann::json &json_rows) {
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + static_cast<int>(json_rows.size()) - 1);
    json_to_rows(get_reference(rows_pointer), json_rows);
    endInsertRows();
  }

  void insert_rows(const int first_row_number, const QList<SubRow> &new_rows) {
    auto &rows = get_reference(rows_pointer);
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + new_rows.size() - 1);
    std::copy(new_rows.cbegin(), new_rows.cend(),
              std::inserter(rows, rows.begin() + first_row_number));
    endInsertRows();
  }

  void insert_row(const int row_number, const SubRow &new_row) {
    beginInsertRows(QModelIndex(), row_number, row_number);
    auto &rows = get_reference(rows_pointer);
    rows.insert(rows.begin() + row_number, new_row);
    endInsertRows();
  }

  void remove_rows(const int first_row_number, int number_of_rows) {
    auto &rows = get_reference(rows_pointer);
    beginRemoveRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_rows - 1);
    rows.erase(rows.begin() + first_row_number,
               rows.begin() + first_row_number + number_of_rows);
    endRemoveRows();
  }

  void set_rows_pointer(QList<SubRow> *const new_rows_pointer = nullptr) {
    beginResetModel();
    rows_pointer = new_rows_pointer;
    endResetModel();
  }
};

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto
get_rows(RowsModel<SubRow> &rows_model) -> QList<SubRow> & {
  return get_reference(rows_model.rows_pointer);
}

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto
get_const_rows(const RowsModel<SubRow> &rows_model) -> const QList<SubRow> & {
  return get_const_reference(rows_model.rows_pointer);
}

template <std::derived_from<Row> SubRow>
static void
copy_from_model(QMimeData &mime_data, const RowsModel<SubRow> &rows_model,
                const int first_row_number, const int number_of_rows,
                const int left_column, const int right_column) {
  const auto &rows = get_const_reference(rows_model.rows_pointer);
  nlohmann::json copied_json = nlohmann::json::array();
  std::transform(
      rows.cbegin() + first_row_number,
      rows.cbegin() + first_row_number + number_of_rows,
      std::back_inserter(copied_json),
      [left_column, right_column](const SubRow &row) -> nlohmann::json {
        return row_to_json(row, left_column, right_column);
      });

  const nlohmann::json copied(
      {{"left_column", left_column},
       {"right_column", right_column},
       {SubRow::get_plural_field_for(), std::move(copied_json)}});

  std::stringstream json_text;
  json_text << std::setw(4) << copied;

  mime_data.setData(get_cells_mime<SubRow>(), json_text.str().c_str());
}

template <std::derived_from<Row> SubRow> struct SetCell : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  QModelIndex index;
  const QVariant old_value;
  const QVariant new_value;

  explicit SetCell(RowsModel<SubRow> &rows_model_input,
                   const QModelIndex &index_input, QVariant new_value_input)
      : rows_model(rows_model_input), index(index_input),
        old_value(index.data()), new_value(std::move(new_value_input)) {}

  void undo() override { rows_model.set_cell(index, old_value); }

  void redo() override { rows_model.set_cell(index, new_value); }
};

template <std::derived_from<Row> SubRow>
struct DeleteCells : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int first_row_number;
  const int left_column;
  const int right_column;
  const QList<SubRow> old_rows;

  explicit DeleteCells(RowsModel<SubRow> &rows_model_input,
                       const int first_row_number_input,
                       const int number_of_rows, const int left_column_input,
                       const int right_column_input)
      : rows_model(rows_model_input), first_row_number(first_row_number_input),
        left_column(left_column_input), right_column(right_column_input),
        old_rows(copy_items(get_const_rows(rows_model), first_row_number,
                            number_of_rows)) {}

  void undo() override {
    rows_model.set_cells(first_row_number, old_rows, left_column, right_column);
  }

  void redo() override {
    rows_model.delete_cells(first_row_number, old_rows.size(), left_column,
                            right_column);
  }
};

template <std::derived_from<Row> SubRow> struct SetCells : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int first_row_number;
  const int left_column;
  const int right_column;
  const QList<SubRow> old_rows;
  const QList<SubRow> new_rows;

  explicit SetCells(RowsModel<SubRow> &rows_model_input,
                    const int first_row_number_input,
                    const nlohmann::json &json_cells,
                    QList<SubRow> old_rows_input, QList<SubRow> new_rows_input)
      : rows_model(rows_model_input), first_row_number(first_row_number_input),
        left_column(get_json_int(json_cells, "left_column")),
        right_column(get_json_int(json_cells, "right_column")),
        old_rows(std::move(old_rows_input)),
        new_rows(std::move(new_rows_input)) {}

  void undo() override {
    rows_model.set_cells(first_row_number, old_rows, left_column, right_column);
  }

  void redo() override {
    rows_model.set_cells(first_row_number, new_rows, left_column, right_column);
  }
};

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto
make_paste_cells_command(QWidget &parent, const int first_row_number,
                         RowsModel<SubRow> &rows_model) -> QUndoCommand * {
  const auto maybe_json_cells = parse_clipboard<SubRow>(parent);
  if (!maybe_json_cells.has_value()) {
    return nullptr;
  }
  const auto &json_cells = maybe_json_cells.value();
  const auto &json_rows =
      get_json_value(json_cells, SubRow::get_plural_field_for());

  auto &rows = get_rows(rows_model);

  const auto number_of_rows =
      std::min({static_cast<int>(json_rows.size()),
                static_cast<int>(rows.size()) - first_row_number});

  QList<SubRow> new_rows;
  partial_json_to_rows(new_rows, json_rows, number_of_rows);

  return new SetCells( // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, first_row_number, json_cells,
      copy_items(rows, first_row_number, number_of_rows), std::move(new_rows));
}

template <std::derived_from<Row> SubRow>
struct InsertRow : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int row_number;
  const SubRow new_row;

  InsertRow(RowsModel<SubRow> &rows_model_input, const int row_number_input,
            SubRow new_row_input = SubRow())
      : rows_model(rows_model_input), row_number(row_number_input),
        new_row(std::move(new_row_input)) {}

  void undo() override { rows_model.remove_rows(row_number, 1); }

  void redo() override { rows_model.insert_row(row_number, new_row); }
};

template <std::derived_from<Row> SubRow>
static void
insert_or_remove(RowsModel<SubRow> &rows_model, const int first_row_number,
                 const QList<SubRow> &new_rows, const bool should_insert) {
  if (should_insert) {
    rows_model.insert_rows(first_row_number, new_rows);
  } else {
    rows_model.remove_rows(first_row_number, new_rows.size());
  }
}

template <std::derived_from<Row> SubRow>
struct InsertRemoveRows : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int first_row_number;
  const QList<SubRow> new_rows;
  const bool backwards;
  InsertRemoveRows(RowsModel<SubRow> &rows_model_input,
                   const int first_row_number_input,
                   QList<SubRow> new_rows_input, const bool backwards_input)
      : rows_model(rows_model_input), first_row_number(first_row_number_input),
        new_rows(std::move(new_rows_input)), backwards(backwards_input) {}

  void undo() override {
    insert_or_remove(rows_model, first_row_number, new_rows, backwards);
  }

  void redo() override {
    insert_or_remove(rows_model, first_row_number, new_rows, !backwards);
  }
};

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto
make_paste_insert_command(QWidget &parent, RowsModel<SubRow> &rows_model,
                          const int row_number) -> QUndoCommand * {
  const auto maybe_json_cells = parse_clipboard<SubRow>(parent);
  if (!maybe_json_cells.has_value()) {
    return nullptr;
  }
  const auto &json_cells = maybe_json_cells.value();
  const auto &json_rows =
      get_json_value(json_cells, SubRow::get_plural_field_for());

  QList<SubRow> new_rows;
  json_to_rows(new_rows, json_rows);
  return new InsertRemoveRows( // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, row_number, std::move(new_rows), false);
}

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto
make_remove_command(RowsModel<SubRow> &rows_model, const int first_row_number,
                    const int number_of_rows) -> QUndoCommand * {
  return new InsertRemoveRows( // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, first_row_number,
      copy_items(get_rows(rows_model), first_row_number, number_of_rows), true);
}

struct ChordsModel : public RowsModel<Chord> {
  Song &song;

  explicit ChordsModel(QUndoStack &undo_stack, Song &song_input)
      : RowsModel(undo_stack), song(song_input) {
    rows_pointer = &song.chords;
  }

  [[nodiscard]] auto
  get_status(const int row_number) const -> QString override {
    return get_key_text(song, row_number);
  }
};

struct PitchedNotesModel : public RowsModel<PitchedNote> {
  Song &song;
  int parent_chord_number = -1;

  explicit PitchedNotesModel(QUndoStack &undo_stack, Song &song_input)
      : RowsModel<PitchedNote>(undo_stack), song(song_input) {}

  [[nodiscard]] auto
  get_status(const int row_number) const -> QString override {
    return get_key_text(
        song, parent_chord_number,
        get_const_reference(rows_pointer).at(row_number).interval.to_double());
  }
};

struct SwitchTable : public QTableView {
  ModelType current_model_type = chords_type;
  int current_chord_number = -1;

  ChordsModel chords_model;
  PitchedNotesModel pitched_notes_model;
  RowsModel<UnpitchedNote> unpitched_notes_model;

  SwitchTable(QUndoStack &undo_stack, Song &song)
      : chords_model(ChordsModel(undo_stack, song)),
        pitched_notes_model(PitchedNotesModel(undo_stack, song)),
        unpitched_notes_model(RowsModel<UnpitchedNote>(undo_stack)) {
    setSelectionMode(QAbstractItemView::ContiguousSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    get_reference(horizontalHeader())
        .setSectionResizeMode(QHeaderView::ResizeToContents);
    get_reference(verticalHeader())
        .setSectionResizeMode(QHeaderView::ResizeToContents);

    setMouseTracking(true);

    QObject::connect(&get_reference(horizontalHeader()),
                     &QHeaderView::geometriesChanged, this,
                     [this]() { updateGeometry(); });
    QObject::connect(&get_reference(verticalHeader()),
                     &QHeaderView::geometriesChanged, this,
                     [this]() { updateGeometry(); });
  }
};

static void copy_selection(const SwitchTable &switch_table) {
  const auto &range = get_only_range(switch_table);
  const auto first_row_number = range.top();
  const auto number_of_rows = get_number_of_rows(range);
  const auto left_column = range.left();
  const auto right_column = range.right();
  auto &mime_data = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QMimeData);

  switch (switch_table.current_model_type) {
  case chords_type:
    copy_from_model(mime_data, switch_table.chords_model, first_row_number,
                    number_of_rows, left_column, right_column);
    break;
  case pitched_notes_type:
    copy_from_model(mime_data, switch_table.pitched_notes_model,
                    first_row_number, number_of_rows, left_column,
                    right_column);
    break;
  case unpitched_notes_type:
    copy_from_model(mime_data, switch_table.unpitched_notes_model,
                    first_row_number, number_of_rows, left_column,
                    right_column);
    break;
  default:
    Q_ASSERT(false);
    return;
  }
  get_reference(QGuiApplication::clipboard()).setMimeData(&mime_data);
}

struct SwitchColumn : public QWidget {
  QLabel &editing_text = *(new QLabel(QObject::tr("Editing chords")));
  SwitchTable &switch_table;

  SwitchColumn(QUndoStack &undo_stack, Song &song)
      : switch_table(*(new SwitchTable(undo_stack, song))) {
    auto &table_column_layout = // NOLINT(cppcoreguidelines-owning-memory)
        *(new QVBoxLayout(this));

    table_column_layout.addWidget(&editing_text, 0, Qt::AlignLeft);
    table_column_layout.addWidget(&switch_table, 0, Qt::AlignLeft);
  }
};

static void set_starting_double(Song &song, Player &player,
                                const ControlId control_id,
                                QDoubleSpinBox &spin_box,
                                const double set_value) {
  switch (control_id) {
  case gain_id:
    fluid_synth_set_gain(player.synth_pointer, static_cast<float>(set_value));
    break;
  case starting_key_id:
    song.starting_key = set_value;
    break;
  case starting_velocity_id:
    song.starting_velocity = set_value;
    break;
  case starting_tempo_id:
    song.starting_tempo = set_value;
  }
  const QSignalBlocker blocker(spin_box);
  spin_box.setValue(set_value);
}

struct SetStartingDouble : public QUndoCommand {
  Song &song;
  Player &player;
  QDoubleSpinBox &spin_box;
  const ControlId control_id;
  const double old_value;
  double new_value;

  explicit SetStartingDouble(Song &song_input, Player &player_input,
                             QDoubleSpinBox &spin_box_input,
                             const ControlId command_id_input,
                             const double old_value_input,
                             const double new_value_input)
      : song(song_input), player(player_input), spin_box(spin_box_input),
        control_id(command_id_input), old_value(old_value_input),
        new_value(new_value_input) {}

  [[nodiscard]] auto id() const -> int override { return control_id; }

  [[nodiscard]] auto
  mergeWith(const QUndoCommand *const next_command_pointer) -> bool override {
    Q_ASSERT(next_command_pointer != nullptr);
    new_value = get_const_reference(dynamic_cast<const SetStartingDouble *>(
                                        next_command_pointer))
                    .new_value;
    return true;
  }

  void undo() override {
    set_starting_double(song, player, control_id, spin_box, old_value);
  }

  void redo() override {
    set_starting_double(song, player, control_id, spin_box, new_value);
  }
};

static void set_double(Song &song, Player &player, QUndoStack &undo_stack,
                       QDoubleSpinBox &spin_box, const ControlId control_id,
                       const double old_value, const double new_value) {
  undo_stack.push(
      new SetStartingDouble( // NOLINT(cppcoreguidelines-owning-memory)
          song, player, spin_box, control_id, old_value, new_value));
}

struct Controls : public QWidget {
  QDoubleSpinBox &gain_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_key_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_velocity_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_tempo_editor = *(new QDoubleSpinBox);
  explicit Controls(Song &song, Player &player, QUndoStack &undo_stack) {
    auto &gain_editor = this->gain_editor;
    auto &starting_key_editor = this->starting_key_editor;
    auto &starting_velocity_editor = this->starting_velocity_editor;
    auto &starting_tempo_editor = this->starting_tempo_editor;

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    auto &controls_form = // NOLINT(cppcoreguidelines-owning-memory)
        *(new QFormLayout(this));

    add_control(controls_form, "&Gain:", gain_editor, 0, MAX_GAIN, "/10",
                GAIN_STEP, 1);
    add_control(controls_form, "Starting &key:", starting_key_editor, 1,
                MAX_STARTING_KEY, " hz");
    add_control(controls_form, "Starting &velocity:", starting_velocity_editor,
                1, MAX_VELOCITY, "/127");
    add_control(controls_form, "Starting &tempo:", starting_tempo_editor, 1,
                MAX_STARTING_TEMPO, " bpm");

    QObject::connect(
        &gain_editor, &QDoubleSpinBox::valueChanged, this,
        [&song, &player, &undo_stack, &gain_editor](double new_value) {
          set_double(song, player, undo_stack, gain_editor, gain_id,
                     player_get_gain(player), new_value);
        });
    QObject::connect(
        &starting_key_editor, &QDoubleSpinBox::valueChanged, this,
        [&song, &player, &undo_stack, &starting_key_editor](double new_value) {
          set_double(song, player, undo_stack, starting_key_editor,
                     starting_key_id, song.starting_key, new_value);
        });
    QObject::connect(
        &starting_velocity_editor, &QDoubleSpinBox::valueChanged, this,
        [&song, &player, &undo_stack,
         &starting_velocity_editor](double new_value) {
          set_double(song, player, undo_stack, starting_velocity_editor,
                     starting_velocity_id, song.starting_velocity, new_value);
        });
    QObject::connect(
        &starting_tempo_editor, &QDoubleSpinBox::valueChanged, this,
        [&song, &player, &undo_stack,
         &starting_tempo_editor](double new_value) {
          set_double(song, player, undo_stack, starting_tempo_editor,
                     starting_tempo_id, song.starting_tempo, new_value);
        });

    gain_editor.setValue(DEFAULT_GAIN);
    starting_key_editor.setValue(song.starting_key);
    starting_velocity_editor.setValue(song.starting_velocity);
    starting_tempo_editor.setValue(song.starting_tempo);

    clear_and_clean(undo_stack);
  }
};

struct SongWidget : public QWidget {
  Song song;
  Player player = Player(*this);
  QUndoStack undo_stack = QUndoStack(nullptr);
  QString current_file;
  QString current_folder =
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

  Controls &controls = *(new Controls(song, player, undo_stack));
  SwitchColumn &switch_column;

  explicit SongWidget() : switch_column(*(new SwitchColumn(undo_stack, song))) {
    auto &row_layout = // NOLINT(cppcoreguidelines-owning-memory)
        *(new QHBoxLayout(this));
    row_layout.addWidget(&controls, 0, Qt::AlignTop);
    row_layout.addWidget(&switch_column, 0, Qt::AlignTop);
  }

  ~SongWidget() override { undo_stack.disconnect(); }

  // prevent moving and copying
  SongWidget(const SongWidget &) = delete;
  auto operator=(const SongWidget &) -> SongWidget = delete;
  SongWidget(SongWidget &&) = delete;
  auto operator=(SongWidget &&) -> SongWidget = delete;
};

static void initialize_play(SongWidget &song_widget) {
  auto &player = song_widget.player;
  const auto &song = song_widget.song;

  player.current_instrument_pointer = nullptr;
  player.current_percussion_set_pointer = nullptr;
  player.current_percussion_instrument_pointer = nullptr;
  player.current_key = song.starting_key;
  player.current_velocity = song.starting_velocity;
  player.current_tempo = song.starting_tempo;
  player.current_time = fluid_sequencer_get_tick(player.sequencer_pointer);

  auto &channel_schedules = player.channel_schedules;
  for (auto index = 0; index < NUMBER_OF_MIDI_CHANNELS; index = index + 1) {
    Q_ASSERT(index < channel_schedules.size());
    channel_schedules[index] = 0;
  }
}

static void play_chords(SongWidget &song_widget, const int first_chord_number,
                        const int number_of_chords, const int wait_frames = 0) {
  auto &player = song_widget.player;
  const auto &song = song_widget.song;

  const auto start_time = player.current_time + wait_frames;
  player.current_time = start_time;
  update_final_time(player, start_time);
  const auto &chords = song.chords;
  for (auto chord_number = first_chord_number;
       chord_number < first_chord_number + number_of_chords;
       chord_number = chord_number + 1) {
    const auto &chord = chords.at(chord_number);

    modulate(player, chord);
    play_all_notes(player, chord_number, chord.pitched_notes);
    play_all_notes(player, chord_number, chord.unpitched_notes);
    const auto new_current_time =
        player.current_time + get_milliseconds(player.current_tempo, chord);
    player.current_time = new_current_time;
    update_final_time(player, new_current_time);
  }
}

static void export_song_to_file(SongWidget &song_widget,
                                const QString &output_file) {
  auto &player = song_widget.player;
  const auto &song = song_widget.song;

  auto *const settings_pointer = player.settings_pointer;
  auto *const event_pointer = player.event_pointer;
  auto *const sequencer_pointer = player.sequencer_pointer;

  stop_playing(event_pointer, sequencer_pointer);
  delete_audio_driver(player.audio_driver_pointer);
  check_fluid_ok(fluid_settings_setstr(settings_pointer, "audio.file.name",
                                       output_file.toStdString().c_str()));

  set_fluid_int(settings_pointer, "synth.lock-memory", 0);

  auto finished = false;
  const auto finished_timer_id = fluid_sequencer_register_client(
      player.sequencer_pointer, "finished timer",
      [](unsigned int /*time*/, fluid_event_t * /*event*/,
         fluid_sequencer_t * /*seq*/, void *data_pointer) {
        auto *finished_pointer = static_cast<bool *>(data_pointer);
        Q_ASSERT(finished_pointer != nullptr);
        *finished_pointer = true;
      },
      &finished);
  Q_ASSERT(finished_timer_id >= 0);

  initialize_play(song_widget);
  play_chords(song_widget, 0, static_cast<int>(song.chords.size()),
              START_END_MILLISECONDS);

  fluid_event_set_dest(event_pointer, finished_timer_id);
  fluid_event_timer(event_pointer, nullptr);
  send_event_at(sequencer_pointer, event_pointer,
                player.final_time + START_END_MILLISECONDS);

  auto *const renderer_pointer = new_fluid_file_renderer(player.synth_pointer);
  Q_ASSERT(renderer_pointer != nullptr);
  while (!finished) {
    check_fluid_ok(fluid_file_renderer_process_block(renderer_pointer));
  }
  delete_fluid_file_renderer(renderer_pointer);

  fluid_event_set_dest(event_pointer, player.sequencer_id);
  set_fluid_int(settings_pointer, "synth.lock-memory", 1);
  player.audio_driver_pointer = make_audio_driver(
      player.parent, player.settings_pointer, player.synth_pointer);
}

static void modulate_before_chord(SongWidget &song_widget,
                                  const int next_chord_number) {
  auto &player = song_widget.player;
  const auto &chords = song_widget.song.chords;
  if (next_chord_number > 0) {
    for (auto chord_number = 0; chord_number < next_chord_number;
         chord_number = chord_number + 1) {
      modulate(player, chords.at(chord_number));
    }
  }
}

static void insert_table_row(SongWidget &song_widget, const int row_number) {
  auto &switch_table = song_widget.switch_column.switch_table;
  const auto current_model_type = switch_table.current_model_type;
  QUndoCommand *undo_command = nullptr;
  if (current_model_type == chords_type) {
    undo_command = new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
        switch_table.chords_model, row_number);
  } else {
    const auto &chord_beats =
        get_const_rows(
            switch_table.chords_model)[switch_table.current_chord_number]
            .beats;
    if (current_model_type == pitched_notes_type) {
      PitchedNote new_pitched_note;
      new_pitched_note.beats = chord_beats;
      undo_command = new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
          switch_table.pitched_notes_model, row_number,
          std::move(new_pitched_note));
    } else {
      UnpitchedNote new_unpitched_note;
      new_unpitched_note.beats = chord_beats;
      undo_command = new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
          switch_table.unpitched_notes_model, row_number,
          std::move(new_unpitched_note));
    }
  }
  song_widget.undo_stack.push(undo_command);
}

static void paste_insert(SongWidget &song_widget, const int row_number) {
  auto &switch_table = song_widget.switch_column.switch_table;

  QUndoCommand *undo_command = nullptr;
  switch (switch_table.current_model_type) {
  case chords_type:
    undo_command = make_paste_insert_command(
        switch_table, switch_table.chords_model, row_number);
    break;
  case pitched_notes_type:
    undo_command = make_paste_insert_command(
        switch_table, switch_table.pitched_notes_model, row_number);
    break;
  case unpitched_notes_type:
    undo_command = make_paste_insert_command(
        switch_table, switch_table.unpitched_notes_model, row_number);
    break;
  default:
    Q_ASSERT(false);
    return;
  }
  if (undo_command == nullptr) {
    return;
  }
  song_widget.undo_stack.push(undo_command);
}

static void delete_table_cells(SongWidget &song_widget) {
  auto &undo_stack = song_widget.undo_stack;
  auto &switch_table = song_widget.switch_column.switch_table;

  const auto &range = get_only_range(switch_table);
  const auto first_row_number = range.top();
  const auto number_of_rows = get_number_of_rows(range);
  const auto left_column = range.left();
  const auto right_column = range.right();

  QUndoCommand *undo_command = nullptr;
  switch (switch_table.current_model_type) {
  case chords_type:
    undo_command = new DeleteCells( // NOLINT(cppcoreguidelines-owning-memory)
        switch_table.chords_model, first_row_number, number_of_rows,
        left_column, right_column);
    break;
  case pitched_notes_type:
    undo_command = new DeleteCells( // NOLINT(cppcoreguidelines-owning-memory)
        switch_table.pitched_notes_model, first_row_number, number_of_rows,
        left_column, right_column);
    break;
  case unpitched_notes_type:
    undo_command = new DeleteCells( // NOLINT(cppcoreguidelines-owning-memory)
        switch_table.unpitched_notes_model, first_row_number, number_of_rows,
        left_column, right_column);
    break;
  default:
    Q_ASSERT(false);
    return;
  }
  undo_stack.push(undo_command);
}

[[nodiscard]] static auto make_file_dialog(
    SongWidget &song_widget, const char *const caption, const QString &filter,
    const QFileDialog::AcceptMode accept_mode, const QString &suffix,
    const QFileDialog::FileMode file_mode) -> QFileDialog & {
  auto &dialog = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QFileDialog(&song_widget, QObject::tr(caption),
                        song_widget.current_folder, filter));

  dialog.setAcceptMode(accept_mode);
  dialog.setDefaultSuffix(suffix);
  dialog.setFileMode(file_mode);

  return dialog;
}

[[nodiscard]] static auto can_discard_changes(SongWidget &song_widget) {
  return song_widget.undo_stack.isClean() ||
         QMessageBox::question(&song_widget, QObject::tr("Unsaved changes"),
                               QObject::tr("Discard unsaved changes?")) ==
             QMessageBox::Yes;
}

[[nodiscard]] static auto get_selected_file(SongWidget &song_widget,
                                            const QFileDialog &dialog) {
  song_widget.current_folder = dialog.directory().absolutePath();
  return get_only(dialog.selectedFiles());
}

static void save_song_as_file(SongWidget &song_widget,
                              const QString &filename) {
  const auto &song = song_widget.song;
  std::ofstream file_io(filename.toStdString().c_str());

  nlohmann::json json_song = nlohmann::json::object();
  json_song["gain"] = player_get_gain(song_widget.player);
  json_song["starting_key"] = song.starting_key;
  json_song["starting_tempo"] = song.starting_tempo;
  json_song["starting_velocity"] = song.starting_velocity;

  add_rows_to_json(json_song, song.chords);

  file_io << std::setw(4) << json_song;
  file_io.close();
  song_widget.current_file = filename;

  song_widget.undo_stack.setClean();
}

static void open_song_file(SongWidget &song_widget, const QString &filename) {
  auto &undo_stack = song_widget.undo_stack;
  auto &controls = song_widget.controls;
  auto &chords_model = song_widget.switch_column.switch_table.chords_model;
  const auto number_of_chords = chords_model.rowCount(QModelIndex());
  std::ifstream file_io(filename.toStdString().c_str());
  nlohmann::json json_song;
  try {
    json_song = nlohmann::json::parse(file_io);
  } catch (const nlohmann::json::parse_error &parse_error) {
    show_warning(song_widget, "Parsing error", parse_error.what());
    return;
  }
  file_io.close();

  static const auto song_validator = []() {
    nlohmann::json song_schema(
        {{"gain", get_number_schema("number", 0, MAX_GAIN)},
         {"starting_key", get_number_schema("number", 1, MAX_STARTING_KEY)},
         {"starting_tempo", get_number_schema("number", 1, MAX_STARTING_TEMPO)},
         {"starting_velocity", get_number_schema("number", 0, MAX_VELOCITY)}});
    add_row_array_schema<Chord>(song_schema);
    return make_validator(nlohmann::json({
                              "gain",
                              "starting_key",
                              "starting_tempo",
                              "starting_velocity",
                          }),
                          song_schema);
  }();
  try {
    song_validator.validate(json_song);
  } catch (const std::exception &error) {
    show_warning(song_widget, "Schema error", error.what());
    return;
  }

  controls.gain_editor.setValue(get_json_double(json_song, "gain"));
  controls.starting_key_editor.setValue(
      get_json_double(json_song, "starting_key"));
  controls.starting_velocity_editor.setValue(
      get_json_double(json_song, "starting_velocity"));
  controls.starting_tempo_editor.setValue(
      get_json_double(json_song, "starting_tempo"));

  if (number_of_chords > 0) {
    chords_model.remove_rows(0, number_of_chords);
  }

  if (json_song.contains("chords")) {
    chords_model.insert_json_rows(0, json_song["chords"]);
  }

  song_widget.current_file = filename;

  clear_and_clean(undo_stack);
}

struct FileMenu : public QMenu {
  QAction save_action = make_action("&Save");
  QAction open_action = make_action("&Open");
  QAction save_as_action = make_action("&Save As...");
  QAction export_action = make_action("&Export recording");

  explicit FileMenu(const char *const name, SongWidget &song_widget)
      : QMenu(QObject::tr(name)) {
    auto &save_action = this->save_action;
    add_menu_action(*this, open_action, QKeySequence::Open);
    addSeparator();
    add_menu_action(*this, save_action, QKeySequence::Save, false);
    add_menu_action(*this, save_as_action, QKeySequence::SaveAs);
    add_menu_action(*this, export_action);

    QObject::connect(&song_widget.undo_stack, &QUndoStack::cleanChanged, this,
                     [&save_action, &song_widget]() {
                       save_action.setEnabled(
                           !song_widget.undo_stack.isClean() &&
                           !song_widget.current_file.isEmpty());
                     });

    QObject::connect(&open_action, &QAction::triggered, this, [&song_widget]() {
      if (can_discard_changes(song_widget)) {
        auto &dialog = make_file_dialog(
            song_widget, "Open — Justly", "JSON file (*.json)",
            QFileDialog::AcceptOpen, ".json", QFileDialog::ExistingFile);
        if (dialog.exec() != 0) {
          open_song_file(song_widget, get_selected_file(song_widget, dialog));
        }
      }
    });

    QObject::connect(&save_action, &QAction::triggered, this, [&song_widget]() {
      save_song_as_file(song_widget, song_widget.current_file);
    });

    QObject::connect(
        &save_as_action, &QAction::triggered, this, [&song_widget]() {
          auto &dialog = make_file_dialog(
              song_widget, "Save As — Justly", "JSON file (*.json)",
              QFileDialog::AcceptSave, ".json", QFileDialog::AnyFile);

          if (dialog.exec() != 0) {
            save_song_as_file(song_widget,
                              get_selected_file(song_widget, dialog));
          }
        });

    QObject::connect(
        &export_action, &QAction::triggered, this, [&song_widget]() {
          auto &dialog = make_file_dialog(
              song_widget, "Export — Justly", "WAV file (*.wav)",
              QFileDialog::AcceptSave, ".wav", QFileDialog::AnyFile);
          dialog.setLabelText(QFileDialog::Accept, "Export");
          if (dialog.exec() != 0) {
            export_song_to_file(song_widget,
                                get_selected_file(song_widget, dialog));
          }
        });
  }
};

struct PasteMenu : public QMenu {
  QAction paste_over_action = make_action("&Over");
  QAction paste_into_action = make_action("&Into start");
  QAction paste_after_action = make_action("&After");
  explicit PasteMenu(const char *const name) : QMenu(QObject::tr(name)) {
    add_menu_action(*this, paste_over_action, QKeySequence::Paste, false);
    add_menu_action(*this, paste_into_action);
    add_menu_action(*this, paste_after_action, QKeySequence::StandardKey(),
                    false);
  }
};

struct InsertMenu : public QMenu {
  QAction insert_after_action = make_action("&After");
  QAction insert_into_action = make_action("&Into start");

  explicit InsertMenu(const char *const name) : QMenu(QObject::tr(name)) {
    add_menu_action(*this, insert_after_action,
                    QKeySequence::InsertLineSeparator, false);
    add_menu_action(*this, insert_into_action, QKeySequence::AddTab);
  }
};

struct PlayMenu : public QMenu {
  QAction play_action = make_action("&Play selection");
  QAction stop_playing_action = make_action("&Stop playing");

  explicit PlayMenu(const char *const name, SongWidget &song_widget)
      : QMenu(QObject::tr(name)) {
    add_menu_action(*this, play_action, QKeySequence::Print, false);
    add_menu_action(*this, stop_playing_action, QKeySequence::Cancel);

    const auto &player = song_widget.player;
    QObject::connect(&play_action, &QAction::triggered, this, [&song_widget]() {
      const auto &song = song_widget.song;
      auto &player = song_widget.player;
      const auto &switch_table = song_widget.switch_column.switch_table;

      const auto current_model_type = switch_table.current_model_type;

      const auto &range = get_only_range(switch_table);
      const auto first_row_number = range.top();
      const auto number_of_rows = get_number_of_rows(range);

      stop_playing(player.event_pointer, player.sequencer_pointer);
      initialize_play(song_widget);

      if (current_model_type == chords_type) {
        modulate_before_chord(song_widget, first_row_number);
        play_chords(song_widget, first_row_number, number_of_rows);
      } else {
        const auto current_chord_number = switch_table.current_chord_number;
        modulate_before_chord(song_widget, current_chord_number);
        const auto &chord = song.chords.at(current_chord_number);
        modulate(player, chord);
        if (current_model_type == pitched_notes_type) {
          play_notes(player, current_chord_number, chord.pitched_notes,
                     first_row_number, number_of_rows);
        } else {
          play_notes(player, current_chord_number, chord.unpitched_notes,
                     first_row_number, number_of_rows);
        }
      }
    });

    QObject::connect(
        &stop_playing_action, &QAction::triggered, this, [&player]() {
          stop_playing(player.event_pointer, player.sequencer_pointer);
        });
  }
};

struct EditMenu : public QMenu {
  QAction cut_action = make_action("&Cut");
  QAction copy_action = make_action("&Copy");
  PasteMenu paste_menu = PasteMenu("&Paste");
  InsertMenu insert_menu = InsertMenu("&Insert");
  QAction delete_cells_action = make_action("&Delete cells");
  QAction remove_rows_action = make_action("&Remove rows");
  QAction back_to_chords_action = make_action("&Back to chords");

  EditMenu(const char *const name, SongWidget &song_widget)
      : QMenu(QObject::tr(name)) {
    auto &undo_stack = song_widget.undo_stack;
    auto &switch_table = song_widget.switch_column.switch_table;

    auto &undo_action = get_reference(undo_stack.createUndoAction(this));
    undo_action.setShortcuts(QKeySequence::Undo);

    auto &redo_action = get_reference(undo_stack.createRedoAction(this));
    redo_action.setShortcuts(QKeySequence::Redo);

    addAction(&undo_action);
    addAction(&redo_action);
    addSeparator();

    add_menu_action(*this, cut_action, QKeySequence::Cut);
    add_menu_action(*this, copy_action, QKeySequence::Copy);
    addMenu(&paste_menu);
    addSeparator();

    addMenu(&insert_menu);
    add_menu_action(*this, delete_cells_action, QKeySequence::Delete, false);
    add_menu_action(*this, remove_rows_action, QKeySequence::DeleteStartOfWord,
                    false);
    addSeparator();

    add_menu_action(*this, back_to_chords_action, QKeySequence::Back, false);

    QObject::connect(&cut_action, &QAction::triggered, this, [&song_widget]() {
      copy_selection(song_widget.switch_column.switch_table);
      delete_table_cells(song_widget);
    });

    QObject::connect(&copy_action, &QAction::triggered, &switch_table,
                     [&switch_table]() { copy_selection(switch_table); });

    QObject::connect(
        &paste_menu.paste_over_action, &QAction::triggered, this,
        [&song_widget]() {
          auto &switch_table = song_widget.switch_column.switch_table;
          const auto first_row_number = get_only_range(switch_table).top();

          QUndoCommand *undo_command = nullptr;
          switch (switch_table.current_model_type) {
          case chords_type:
            undo_command = make_paste_cells_command(
                switch_table, first_row_number, switch_table.chords_model);
            break;
          case pitched_notes_type:
            undo_command =
                make_paste_cells_command(switch_table, first_row_number,
                                         switch_table.pitched_notes_model);
            break;
          case unpitched_notes_type:
            undo_command =
                make_paste_cells_command(switch_table, first_row_number,
                                         switch_table.unpitched_notes_model);
            break;
          default:
            Q_ASSERT(false);
            return;
          }
          if (undo_command == nullptr) {
            return;
          }
          song_widget.undo_stack.push(undo_command);
        });

    QObject::connect(&paste_menu.paste_into_action, &QAction::triggered, this,
                     [&song_widget]() { paste_insert(song_widget, 0); });

    QObject::connect(
        &paste_menu.paste_after_action, &QAction::triggered, this,
        [&song_widget]() {
          paste_insert(song_widget,
                       get_next_row(song_widget.switch_column.switch_table));
        });

    QObject::connect(
        &insert_menu.insert_after_action, &QAction::triggered, this,
        [&song_widget]() {
          insert_table_row(
              song_widget,
              get_next_row(song_widget.switch_column.switch_table));
        });

    QObject::connect(&insert_menu.insert_into_action, &QAction::triggered, this,
                     [&song_widget]() { insert_table_row(song_widget, 0); });

    QObject::connect(&delete_cells_action, &QAction::triggered, this,
                     [&song_widget]() { delete_table_cells(song_widget); });

    QObject::connect(
        &remove_rows_action, &QAction::triggered, this, [&song_widget]() {
          auto &switch_table = song_widget.switch_column.switch_table;
          auto &undo_stack = song_widget.undo_stack;

          const auto &range = get_only_range(switch_table);
          const auto first_row_number = range.top();
          const auto number_of_rows = get_number_of_rows(range);

          QUndoCommand *undo_command = nullptr;
          switch (switch_table.current_model_type) {
          case chords_type:
            undo_command = make_remove_command(
                switch_table.chords_model, first_row_number, number_of_rows);
            break;
          case pitched_notes_type:
            undo_command =
                make_remove_command(switch_table.pitched_notes_model,
                                    first_row_number, number_of_rows);
            break;
          case unpitched_notes_type:
            undo_command =
                make_remove_command(switch_table.unpitched_notes_model,
                                    first_row_number, number_of_rows);
            break;
          default:
            Q_ASSERT(false);
            return;
          }
          undo_stack.push(undo_command);
        });
  }
};

struct SongMenuBar : public QMenuBar {
  FileMenu file_menu;
  EditMenu edit_menu;
  PlayMenu play_menu;

  explicit SongMenuBar(SongWidget &song_widget)
      : file_menu(FileMenu("&File", song_widget)),
        edit_menu(EditMenu("&Edit", song_widget)),
        play_menu(PlayMenu("&Play", song_widget)) {
    addMenu(&file_menu);
    addMenu(&edit_menu);
    addMenu(&play_menu);
  }
};

static void update_actions(SongMenuBar &song_menu_bar,
                           const QItemSelectionModel &selector) {
  const auto anything_selected = !selector.selection().empty();

  auto &edit_menu = song_menu_bar.edit_menu;

  edit_menu.cut_action.setEnabled(anything_selected);
  edit_menu.copy_action.setEnabled(anything_selected);
  edit_menu.paste_menu.paste_over_action.setEnabled(anything_selected);
  edit_menu.paste_menu.paste_after_action.setEnabled(anything_selected);
  edit_menu.insert_menu.insert_after_action.setEnabled(anything_selected);
  edit_menu.delete_cells_action.setEnabled(anything_selected);
  edit_menu.remove_rows_action.setEnabled(anything_selected);
  song_menu_bar.play_menu.play_action.setEnabled(anything_selected);
}

static void set_model(SongMenuBar &song_menu_bar,
                      QAbstractItemView &switch_table,
                      QAbstractItemModel &model) {
  auto *const old_selection_model_pointer = switch_table.selectionModel();
  switch_table.setModel(&model);
  delete old_selection_model_pointer; // NOLINT(cppcoreguidelines-owning-memory)
  auto &selection_model = get_reference(switch_table.selectionModel());
  update_actions(song_menu_bar, selection_model);
  QObject::connect(&selection_model, &QItemSelectionModel::selectionChanged,
                   &switch_table, [&song_menu_bar, &selection_model]() {
                     update_actions(song_menu_bar, selection_model);
                   });
}

static void edit_children_or_back(SongMenuBar &song_menu_bar,
                                  SwitchColumn &switch_column,
                                  const int chord_number, const bool is_pitched,
                                  const bool should_edit_children) {
  auto &edit_menu = song_menu_bar.edit_menu;
  auto &editing_text = switch_column.editing_text;
  auto &switch_table = switch_column.switch_table;

  edit_menu.back_to_chords_action.setEnabled(should_edit_children);
  song_menu_bar.file_menu.open_action.setEnabled(!should_edit_children);

  if (should_edit_children) {
    QString label_text;
    QTextStream stream(&label_text);
    stream << QObject::tr(is_pitched ? "Editing pitched notes for chord "
                                     : "Editing unpitched notes for chord ")
           << chord_number + 1;
    editing_text.setText(label_text);
    Q_ASSERT(switch_table.current_model_type == chords_type);
    switch_table.current_chord_number = chord_number;
    auto &chord = get_rows(switch_table.chords_model)[chord_number];
    if (is_pitched) {
      switch_table.current_model_type = pitched_notes_type;
      auto &pitched_notes_model = switch_table.pitched_notes_model;
      pitched_notes_model.parent_chord_number = chord_number;
      pitched_notes_model.set_rows_pointer(&chord.pitched_notes);
      set_model(song_menu_bar, switch_table, pitched_notes_model);
    } else {
      switch_table.current_model_type = unpitched_notes_type;
      auto &unpitched_notes_model = switch_table.unpitched_notes_model;
      unpitched_notes_model.set_rows_pointer(&chord.unpitched_notes);
      set_model(song_menu_bar, switch_table, unpitched_notes_model);
    }
  } else {
    set_model(song_menu_bar, switch_table, switch_table.chords_model);

    editing_text.setText(QObject::tr("Editing chords"));
    switch_table.current_model_type = chords_type;
    switch_table.current_chord_number = -1;

    if (is_pitched) {
      auto &pitched_notes_model = switch_table.pitched_notes_model;
      pitched_notes_model.set_rows_pointer();
      pitched_notes_model.parent_chord_number = -1;
    } else {
      switch_table.unpitched_notes_model.set_rows_pointer();
    }
  }
}

struct EditChildrenOrBack : public QUndoCommand {
  SongMenuBar &song_menu_bar;
  SwitchColumn &switch_column;
  const int chord_number;
  const bool is_pitched;
  const bool backwards;

  explicit EditChildrenOrBack(SongMenuBar &song_menu_bar_input,
                              SwitchColumn &switch_column_input,
                              const int chord_number_input,
                              const bool is_notes_input,
                              const bool backwards_input)
      : song_menu_bar(song_menu_bar_input), switch_column(switch_column_input),
        chord_number(chord_number_input), is_pitched(is_notes_input),
        backwards(backwards_input) {}

  void undo() override {
    edit_children_or_back(song_menu_bar, switch_column, chord_number,
                          is_pitched, backwards);
  }

  void redo() override {
    edit_children_or_back(song_menu_bar, switch_column, chord_number,
                          is_pitched, !backwards);
  }
};

static void add_edit_children_or_back(SongMenuBar &song_menu_bar,
                                      SongWidget &song_widget,
                                      const int chord_number,
                                      const bool is_pitched,
                                      const bool backwards) {
  song_widget.undo_stack.push(
      new EditChildrenOrBack( // NOLINT(cppcoreguidelines-owning-memory)
          song_menu_bar, song_widget.switch_column, chord_number, is_pitched,
          backwards));
}

struct SongEditor : public QMainWindow {
public:
  SongWidget &song_widget = *(new SongWidget);
  SongMenuBar &song_menu_bar = *(new SongMenuBar(song_widget));

  explicit SongEditor() {
    auto &song_menu_bar = this->song_menu_bar;
    auto &song_widget = this->song_widget;

    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    get_reference(statusBar()).showMessage("");

    setWindowTitle("Justly");
    setCentralWidget(&song_widget);
    setMenuBar(&song_menu_bar);
    resize(get_const_reference(QGuiApplication::primaryScreen())
               .availableGeometry()
               .size());

    QObject::connect(
        &song_menu_bar.edit_menu.back_to_chords_action, &QAction::triggered,
        this, [&song_menu_bar, &song_widget]() {
          auto &switch_table = song_widget.switch_column.switch_table;
          add_edit_children_or_back(
              song_menu_bar, song_widget, switch_table.current_chord_number,
              switch_table.current_model_type == pitched_notes_type, true);
        });

    QObject::connect(
        &song_widget.switch_column.switch_table,
        &QAbstractItemView::doubleClicked, this,
        [&song_menu_bar, &song_widget](const QModelIndex &index) {
          if (song_widget.switch_column.switch_table.current_model_type ==
              chords_type) {
            const auto column = index.column();
            const auto is_pitched = column == chord_pitched_notes_column;
            if (is_pitched || (column == chord_unpitched_notes_column)) {
              add_edit_children_or_back(song_menu_bar, song_widget, index.row(),
                                        is_pitched, false);
            }
          }
        });

    set_model(song_menu_bar, switch_table, switch_table.chords_model);
    clear_and_clean(undo_stack);
  };

  void closeEvent(QCloseEvent *const close_event_pointer) override {
    if (!can_discard_changes(song_widget)) {
      get_reference(close_event_pointer).ignore();
      return;
    }
    QMainWindow::closeEvent(close_event_pointer);
  };
};

void set_up() {
  QApplication::setApplicationDisplayName("Justly");

  const auto icon_file = QDir(QCoreApplication::applicationDirPath())
                             .filePath("../share/Justly.svg");
  Q_ASSERT(QFile::exists(icon_file));
  const QPixmap pixmap(icon_file);
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
    const auto numerator = interval.numerator;
    const auto denominator = interval.denominator;
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
  QItemEditorFactory::setDefaultFactory(&factory);
}

auto make_song_editor() -> SongEditor & {
  return *(new SongEditor); // NOLINT(cppcoreguidelines-owning-memory)
}

void show_song_editor(SongEditor &song_editor) { return song_editor.show(); }

void delete_song_editor(SongEditor &song_editor) {
  delete &song_editor; // NOLINT(cppcoreguidelines-owning-memory)
}

auto get_table_view(const SongEditor &song_editor) -> QAbstractItemView & {
  return song_editor.song_widget.switch_column.switch_table;
}

auto get_chords_model(SongEditor &song_editor) -> QAbstractItemModel & {
  return song_editor.song_widget.switch_column.switch_table.chords_model;
}

auto get_pitched_notes_model(SongEditor &song_editor) -> QAbstractItemModel & {
  return song_editor.song_widget.switch_column.switch_table.pitched_notes_model;
}

auto get_unpitched_notes_model(SongEditor &song_editor)
    -> QAbstractItemModel & {
  return song_editor.song_widget.switch_column.switch_table
      .unpitched_notes_model;
}

void trigger_edit_pitched_notes(SongEditor &song_editor, int chord_number) {
  double_click_column(song_editor.song_widget.switch_column.switch_table,
                      chord_number, chord_pitched_notes_column);
}

void trigger_edit_unpitched_notes(SongEditor &song_editor, int chord_number) {
  double_click_column(song_editor.song_widget.switch_column.switch_table,
                      chord_number, chord_unpitched_notes_column);
}

void trigger_back_to_chords(SongEditor &song_editor) {
  song_editor.song_menu_bar.edit_menu.back_to_chords_action.trigger();
}

auto get_gain(const SongEditor &song_editor) -> double {
  return player_get_gain(song_editor.song_widget.player);
}

auto get_starting_key(const SongEditor &song_editor) -> double {
  return song_editor.song_widget.song.starting_key;
}

auto get_starting_velocity(const SongEditor &song_editor) -> double {
  return song_editor.song_widget.song.starting_velocity;
}

auto get_starting_tempo(const SongEditor &song_editor) -> double {
  return song_editor.song_widget.song.starting_tempo;
}

auto get_current_file(const SongEditor &song_editor) -> QString {
  return song_editor.song_widget.current_file;
}

void set_gain(const SongEditor &song_editor, double new_value) {
  song_editor.song_widget.controls.gain_editor.setValue(new_value);
}

void set_starting_key(const SongEditor &song_editor, double new_value) {
  song_editor.song_widget.controls.starting_key_editor.setValue(new_value);
}

void set_starting_velocity(const SongEditor &song_editor, double new_value) {
  song_editor.song_widget.controls.starting_velocity_editor.setValue(new_value);
}

void set_starting_tempo(const SongEditor &song_editor, double new_value) {
  song_editor.song_widget.controls.starting_tempo_editor.setValue(new_value);
}

auto create_editor(const QAbstractItemView &table_view,
                   QModelIndex index) -> QWidget & {
  auto &delegate = get_reference(table_view.itemDelegate());
  auto &cell_editor = get_reference(delegate.createEditor(
      &get_reference(table_view.viewport()), QStyleOptionViewItem(), index));
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

void undo(SongEditor &song_editor) {
  song_editor.song_widget.undo_stack.undo();
}

void trigger_insert_after(SongEditor &song_editor) {
  song_editor.song_menu_bar.edit_menu.insert_menu.insert_after_action.trigger();
}

void trigger_insert_into(SongEditor &song_editor) {
  song_editor.song_menu_bar.edit_menu.insert_menu.insert_into_action.trigger();
}

void trigger_delete_cells(SongEditor &song_editor) {
  song_editor.song_menu_bar.edit_menu.delete_cells_action.trigger();
}

void trigger_remove_rows(SongEditor &song_editor) {
  song_editor.song_menu_bar.edit_menu.remove_rows_action.trigger();
}

void trigger_cut(SongEditor &song_editor) {
  song_editor.song_menu_bar.edit_menu.cut_action.trigger();
}

void trigger_copy(SongEditor &song_editor) {
  song_editor.song_menu_bar.edit_menu.copy_action.trigger();
}

void trigger_paste_over(SongEditor &song_editor) {
  song_editor.song_menu_bar.edit_menu.paste_menu.paste_over_action.trigger();
}

void trigger_paste_into(SongEditor &song_editor) {
  song_editor.song_menu_bar.edit_menu.paste_menu.paste_into_action.trigger();
}

void trigger_paste_after(SongEditor &song_editor) {
  song_editor.song_menu_bar.edit_menu.paste_menu.paste_after_action.trigger();
}

void trigger_save(SongEditor &song_editor) {
  song_editor.song_menu_bar.file_menu.save_action.trigger();
}

void trigger_play(SongEditor &song_editor) {
  song_editor.song_menu_bar.play_menu.play_action.trigger();
}

void trigger_stop_playing(SongEditor &song_editor) {
  song_editor.song_menu_bar.play_menu.stop_playing_action.trigger();
}

void open_file(SongEditor &song_editor, const QString &filename) {
  open_song_file(song_editor.song_widget, filename);
}

void save_as_file(SongEditor &song_editor, const QString &filename) {
  save_song_as_file(song_editor.song_widget, filename);
}

void export_to_file(SongEditor &song_editor, const QString &output_file) {
  export_song_to_file(song_editor.song_widget, output_file);
}

#include "justly.moc"
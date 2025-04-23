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
#include <QtWidgets/QAbstractItemView>
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
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
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
#include <limits>
#include <map>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <tinyxml2.h>
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
static const auto MAX_RELEASE_TIME = 6000;
static const auto FIFTH_HALFSTEPS = 7;
static const auto FIVE = 5;
static const auto GAIN_STEP = 0.1;
static const auto HALFSTEPS_PER_OCTAVE = 12;
static const auto MAX_GAIN = 10;
static const auto MAX_OCTAVE = 9;
static const auto MAX_RATIONAL_DENOMINATOR = 999;
static const auto MAX_RATIONAL_NUMERATOR = 999;
static const auto MAX_STARTING_KEY = 999;
static const auto MAX_STARTING_TEMPO = 999;
static const auto MAX_VELOCITY = 127;
static const auto MAX_MIDI_NUMBER = 127;
static const auto MIDDLE_C_MIDI = 60;
static const auto MILLISECONDS_PER_MINUTE = 60000;
static const auto NUMBER_OF_MIDI_CHANNELS = 64;
static const auto OCTAVE_RATIO = 2.0;
static const auto SEVEN = 7;
static const auto START_END_MILLISECONDS = 500;
static const auto TAMBOURINE_MIDI = 54;
static const auto WORDS_WIDTH = 200;
static const auto ZERO_BEND_HALFSTEPS = 2;
static const auto QUARTER_STEP = 0.5;

enum ChangeId {
  gain_id,
  starting_key_id,
  starting_velocity_id,
  starting_tempo_id,
  replace_table_id
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

enum RowType { chord_type, pitched_note_type, unpitched_note_type };

template <typename Thing>
[[nodiscard]] static auto get_reference(Thing *thing_pointer) -> Thing & {
  Q_ASSERT(thing_pointer != nullptr);
  return *thing_pointer;
}

template <typename Iterable>
[[nodiscard]] static auto get_only(const Iterable &iterable) -> const auto & {
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
  Q_ASSERT(field_name != nullptr);
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

static void add_string_to_json(nlohmann::json &json_row,
                               const char *const field_name,
                               const QString &words) {
  Q_ASSERT(words.isValidUtf16());
  Q_ASSERT(json_row.is_object());
  if (!words.isEmpty()) {
    json_row[field_name] = words.toStdString().c_str();
  }
}

[[nodiscard]] static auto get_number_schema(const char *const type,
                                            const int minimum,
                                            const int maximum) {
  return nlohmann::json(
      {{"type", type}, {"minimum", minimum}, {"maximum", maximum}});
}

[[nodiscard]] static auto get_object_schema(nlohmann::json properties_json) {
  return nlohmann::json(
      {{"type", "object"}, {"properties", std::move(properties_json)}});
}

[[nodiscard]] static auto get_rational_fields_schema() {
  return nlohmann::json(
      {{"numerator", get_number_schema("integer", 1, MAX_RATIONAL_NUMERATOR)},
       {"denominator",
        get_number_schema("integer", 1, MAX_RATIONAL_DENOMINATOR)}});
}

[[nodiscard]] static auto get_row_fields_schema() {
  return nlohmann::json(
      {{"beats", get_object_schema(get_rational_fields_schema())},
       {"velocity_ratio", get_object_schema(get_rational_fields_schema())},
       {"words", nlohmann::json({{"type", "string"}})}});
}

template <std::derived_from<QWidget> SubWidget>
[[nodiscard]] static auto get_minimum_size() -> const QSize & {
  static const auto minimum_size = SubWidget(nullptr).minimumSizeHint();
  return minimum_size;
}

[[nodiscard]] static auto make_validator(nlohmann::json required_json,
                                         nlohmann::json properties_json) {
  return nlohmann::json_schema::json_validator(
      nlohmann::json({{"type", "object"},
                      {"&schema", "http://json-schema.org/draft-07/schema#"},
                      {"required", std::move(required_json)},
                      {"properties", std::move(properties_json)}}));
}

[[nodiscard]] static auto get_midi(const double key) {
  Q_ASSERT(key > 0);
  return HALFSTEPS_PER_OCTAVE * log2(key / CONCERT_A_FREQUENCY) +
         CONCERT_A_MIDI;
}

[[nodiscard]] static auto get_frequency(const double midi_number) {
  return pow(OCTAVE_RATIO,
             (midi_number - CONCERT_A_MIDI) / HALFSTEPS_PER_OCTAVE) *
         CONCERT_A_FREQUENCY;
}

static void check_fluid_ok(const int fluid_result) {
  Q_ASSERT(fluid_result == FLUID_OK);
}

static void set_fluid_int(fluid_settings_t &settings, const char *const field,
                          const int value) {
  Q_ASSERT(field != nullptr);
  check_fluid_ok(fluid_settings_setint(&settings, field, value));
}

[[nodiscard]] static auto get_soundfont_id(fluid_synth_t &synth) {
  const auto soundfont_file = QDir(QCoreApplication::applicationDirPath())
                                  .filePath("../share/MuseScore_General.sf2")
                                  .toStdString();
  Q_ASSERT(std::filesystem::exists(soundfont_file));

  const auto soundfont_id =
      fluid_synth_sfload(&synth, soundfont_file.c_str(), 1);
  Q_ASSERT(soundfont_id >= 0);
  return soundfont_id;
}

static void send_event_at(fluid_sequencer_t &sequencer, fluid_event_t &event,
                          const double time) {
  Q_ASSERT(time >= 0);
  check_fluid_ok(fluid_sequencer_send_at(
      &sequencer, &event, static_cast<unsigned int>(std::round(time)), 1));
}

[[nodiscard]] static auto
make_audio_driver(QWidget &parent, fluid_settings_t &settings,
                  fluid_synth_t &synth) -> fluid_audio_driver_t * {
#ifndef NO_REALTIME_AUDIO
  auto *const audio_driver_pointer = new_fluid_audio_driver(&settings, &synth);
  if (audio_driver_pointer == nullptr) {
    QMessageBox::warning(&parent, QObject::tr("Audio driver error"),
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

static void stop_playing(fluid_sequencer_t &sequencer, fluid_event_t &event) {
  fluid_sequencer_remove_events(&sequencer, -1, -1, -1);

  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       channel_number = channel_number + 1) {
    fluid_event_all_sounds_off(&event, channel_number);
    fluid_sequencer_send_now(&sequencer, &event);
  }
}

[[nodiscard]] static auto get_number_of_rows(const QItemSelectionRange &range) {
  Q_ASSERT(range.isValid());
  return range.bottom() - range.top() + 1;
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

static void add_control(QFormLayout &spin_boxes_form, const QString &label,
                        QDoubleSpinBox &spin_box, const int minimum,
                        const int maximum, const QString &suffix,
                        const double single_step = 1, const int decimals = 0) {
  Q_ASSERT(suffix.isValidUtf16());
  Q_ASSERT(label.isValidUtf16());
  spin_box.setMinimum(minimum);
  spin_box.setMaximum(maximum);
  spin_box.setSuffix(suffix);
  spin_box.setSingleStep(single_step);
  spin_box.setDecimals(decimals);
  spin_boxes_form.addRow(label, &spin_box);
}

struct Program {
  QString name;
  short bank_number;
  short preset_number;

  Program(const char *const name_input, const short bank_number_input,
          const short preset_number_input)
      : name(name_input), bank_number(bank_number_input),
        preset_number(preset_number_input){};
};

[[nodiscard]] static auto get_name_or_empty(const Program *program_pointer) {
  if (program_pointer == nullptr) {
    return QString("");
  }
  return get_reference(program_pointer).name;
}

template <typename SubProgram> // type properties
concept ProgramInterface = std::derived_from<SubProgram, Program> &&
  requires()
{
  { SubProgram::get_all_programs() } -> std::same_as<const QList<SubProgram> &>;
};

template <ProgramInterface SubProgram>
[[nodiscard]] static auto
get_by_name(const QString &name) -> const SubProgram & {
  Q_ASSERT(name.isValidUtf16());
  const auto &all_programs = SubProgram::get_all_programs();
  const auto program_pointer = std::find_if(
      all_programs.cbegin(), all_programs.cend(),
      [name](const SubProgram &item) { return item.name == name; });
  Q_ASSERT(program_pointer != nullptr);
  return *program_pointer;
}

static void add_program_to_json(nlohmann::json &json_row,
                                const char *const field_name,
                                const Program *program_pointer) {
  Q_ASSERT(field_name != nullptr);
  Q_ASSERT(json_row.is_object());
  if (program_pointer != nullptr) {
    json_row[field_name] = get_reference(program_pointer).name.toStdString();
  }
}

template <ProgramInterface SubProgram>
[[nodiscard]] static auto json_field_to_program_pointer(
    const nlohmann::json &json_row,
    const char *const field_name) -> const SubProgram * {
  Q_ASSERT(json_row.is_object());
  Q_ASSERT(field_name != nullptr);
  if (json_row.contains(field_name)) {
    const auto &json_named = json_row[field_name];
    Q_ASSERT(json_named.is_string());
    return &get_by_name<SubProgram>(
        QString::fromStdString(json_named.get<std::string>()));
  };
  return nullptr;
}

template <ProgramInterface SubProgram> static auto get_program_schema() {
  std::vector<std::string> names;
  const auto &all_programs = SubProgram::get_all_programs();
  std::transform(
      all_programs.cbegin(), all_programs.cend(), std::back_inserter(names),
      [](const SubProgram &item) { return item.name.toStdString(); });
  return nlohmann::json({{"type", "string"}, {"enum", std::move(names)}});
}

template <std::derived_from<Program> SubProgram>
[[nodiscard]] static auto get_programs(const bool is_percussion) {
  auto &settings = get_reference(new_fluid_settings());
  auto &synth = get_reference(new_fluid_synth(&settings));

  fluid_sfont_t *const soundfont_pointer =
      fluid_synth_get_sfont_by_id(&synth, get_soundfont_id(synth));
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

  delete_fluid_synth(&synth);
  delete_fluid_settings(&settings);

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

  [[nodiscard]] static auto get_all_programs() -> const QList<PercussionSet> & {
    static const auto all_percussion_sets = get_programs<PercussionSet>(true);
    return all_percussion_sets;
  }
};

Q_DECLARE_METATYPE(const PercussionSet *);

struct PercussionInstrument {
  const PercussionSet *percussion_set_pointer = nullptr;
  short midi_number = TAMBOURINE_MIDI;

  PercussionInstrument() = default;
  PercussionInstrument(const PercussionSet *const percussion_set_pointer_input,
                       const short midi_number_input)
      : percussion_set_pointer(percussion_set_pointer_input),
        midi_number(midi_number_input){};

  [[nodiscard]] auto
  operator==(const PercussionInstrument &other_percussion_instrument) const {
    return percussion_set_pointer ==
               other_percussion_instrument.percussion_set_pointer &&
           midi_number == other_percussion_instrument.midi_number;
  }
};

Q_DECLARE_METATYPE(PercussionInstrument);

[[nodiscard]] static auto percussion_instrument_is_default(
    const PercussionInstrument &percussion_instrument) {
  return percussion_instrument.percussion_set_pointer == nullptr;
}

[[nodiscard]] static auto
program_pointer_to_string(const Program *program_pointer) -> QString {
  if (program_pointer == nullptr) {
    return "";
  }
  return QObject::tr(get_reference(program_pointer).name.toStdString().c_str());
}

static void percussion_instrument_to_stream(
    QTextStream &stream, const PercussionInstrument &percussion_instrument) {
  if (percussion_instrument_is_default(percussion_instrument)) {
    return;
  }
  stream << program_pointer_to_string(
      percussion_instrument.percussion_set_pointer);
  stream << " #";
  stream << percussion_instrument.midi_number;
}

static void add_unpitched_fields_to_schema(nlohmann::json &schema) {
  schema["percussion_instrument"] = get_object_schema(nlohmann::json(
      {{"percussion_set", get_program_schema<PercussionSet>()},
       {"midi_number", get_number_schema("integer", 0, MAX_MIDI_NUMBER)}}));
}

[[nodiscard]] static auto get_array_schema(nlohmann::json fields_schema) {
  return nlohmann::json(
      {{"type", "array"},
       {"items", get_object_schema(std::move(fields_schema))}});
}

struct Instrument : public Program {
  Instrument(const char *const name, const short bank_number,
             const short preset_number)
      : Program(name, bank_number, preset_number){};

  [[nodiscard]] static auto get_all_programs() -> const QList<Instrument> & {
    static const auto all_instruments = get_programs<Instrument>(false);
    return all_instruments;
  }
};

Q_DECLARE_METATYPE(const Instrument *);

struct AbstractRational {
  int numerator = 1;
  int denominator = 1;

  AbstractRational() = default;
  AbstractRational(const int numerator_input, const int denominator_input) {
    Q_ASSERT(denominator_input != 0);
    const auto common_denominator =
        std::gcd(numerator_input, denominator_input);
    numerator = numerator_input / common_denominator;
    denominator = denominator_input / common_denominator;
  }

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

template <typename SubRational>
concept RationalInterface = std::derived_from<SubRational, AbstractRational> &&
                            requires(const nlohmann::json &json_rational) {
                              {
                                SubRational(json_rational)
                              } -> std::same_as<SubRational>;
                            };

template <RationalInterface SubRational>
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

[[nodiscard]] static auto shift_octave(AbstractRational &rational,
                                       int octave) -> int {
  while (rational.numerator % 2 == 0) {
    rational.numerator = rational.numerator / 2;
    octave = octave + 1;
  }
  while (rational.denominator % 2 == 0) {
    rational.denominator = rational.denominator / 2;
    octave = octave - 1;
  }
  return octave;
}

struct Interval : public AbstractRational {
  int octave = 0;

  Interval() = default;

  Interval(const int numerator_input, const int denominator_input,
           const int octave_input)
      : AbstractRational(numerator_input, denominator_input),
        octave(octave_input) {
    octave = shift_octave(*this, octave);
  }

  explicit Interval(const nlohmann::json &json_rational)
      : AbstractRational(json_rational),
        octave(json_rational.value("octave", 0)) {
    octave = shift_octave(*this, octave);
  }

  [[nodiscard]] auto operator==(const Interval &other_interval) const {
    return AbstractRational::operator==(other_interval) &&
           octave == other_interval.octave;
  }

  [[nodiscard]] auto operator*(const Interval &other_interval) const {
    return Interval(numerator * other_interval.numerator,
                    denominator * other_interval.denominator,
                    octave + other_interval.octave);
  }

  [[nodiscard]] auto operator/(const Interval &other_interval) const {
    return Interval(numerator * other_interval.denominator,
                    denominator * other_interval.numerator,
                    octave - other_interval.octave);
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
  schema["instrument"] = get_program_schema<Instrument>();
  auto interval_fields_schema = get_rational_fields_schema();
  interval_fields_schema["octave"] =
      get_number_schema("integer", -MAX_OCTAVE, MAX_OCTAVE);
  schema["interval"] = get_object_schema(std::move(interval_fields_schema));
}

template <ProgramInterface SubProgram> struct ProgramEditor : public QComboBox {
  explicit ProgramEditor(QWidget *const parent_pointer)
      : QComboBox(parent_pointer) {
    static auto names_model = []() {
      const auto &all_programs = SubProgram::get_all_programs();
      QList<QString> names({""});
      std::transform(all_programs.cbegin(), all_programs.cend(),
                     std::back_inserter(names), [](const SubProgram &item) {
                       return program_pointer_to_string(&item);
                     });
      return QStringListModel(names);
    }();
    setModel(&names_model);
    // force scrollbar for combo box
    setStyleSheet("combobox-popup: 0;");
  }

  [[nodiscard]] auto value() const -> const SubProgram * {
    const auto row = currentIndex();
    if (row == 0) {
      return nullptr;
    }
    return &SubProgram::get_all_programs().at(row - 1);
  }

  void setValue(const SubProgram *new_value) {
    setCurrentIndex(
        new_value == nullptr
            ? 0
            : static_cast<int>(std::distance(
                  SubProgram::get_all_programs().data(), new_value)) +
                  1);
  }
};

struct InstrumentEditor : public ProgramEditor<Instrument> {
  Q_OBJECT
  Q_PROPERTY(const Instrument *value READ value WRITE setValue USER true)
public:
  explicit InstrumentEditor(QWidget *const parent_pointer)
      : ProgramEditor<Instrument>(parent_pointer) {}
};

struct PercussionSetEditor : public ProgramEditor<PercussionSet> {
  Q_OBJECT
  Q_PROPERTY(const PercussionSet *value READ value WRITE setValue USER true)
public:
  explicit PercussionSetEditor(QWidget *const parent_pointer_input)
      : ProgramEditor<PercussionSet>(parent_pointer_input) {}
};

struct PercussionInstrumentEditor : public QFrame {
  Q_OBJECT
  Q_PROPERTY(PercussionInstrument value READ value WRITE setValue USER true)

  ProgramEditor<PercussionSet> &percussion_set_editor =
      *(new ProgramEditor<PercussionSet>(this));
  QLabel &number_text = *(new QLabel("#"));
  QSpinBox &midi_number_box = *(new QSpinBox);
  QBoxLayout &row_layout = *(new QHBoxLayout(this));

public:
  explicit PercussionInstrumentEditor(QWidget *const parent_pointer)
      : QFrame(parent_pointer) {
    setFrameStyle(QFrame::StyledPanel);
    setAutoFillBackground(true);

    midi_number_box.setMinimum(0);
    midi_number_box.setMaximum(MAX_MIDI_NUMBER);

    row_layout.addWidget(&percussion_set_editor);
    row_layout.addWidget(&number_text);
    row_layout.addWidget(&midi_number_box);
    row_layout.setContentsMargins(0, 0, 0, 0);
  }
  [[nodiscard]] auto value() const -> PercussionInstrument {
    return PercussionInstrument({percussion_set_editor.value(),
                                 static_cast<short>(midi_number_box.value())});
  }
  void setValue(const PercussionInstrument &new_value) const {
    percussion_set_editor.setValue(new_value.percussion_set_pointer);
    midi_number_box.setValue(new_value.midi_number);
  }
};

struct AbstractRationalEditor : public QFrame {
  QSpinBox &numerator_box = *(new QSpinBox);
  QLabel &slash_text = *(new QLabel("/"));
  QSpinBox &denominator_box = *(new QSpinBox);
  QBoxLayout &row_layout = *(new QHBoxLayout(this));

  explicit AbstractRationalEditor(QWidget *const parent_pointer)
      : QFrame(parent_pointer) {
    setFrameStyle(QFrame::StyledPanel);
    setAutoFillBackground(true);

    numerator_box.setMinimum(1);
    numerator_box.setMaximum(MAX_RATIONAL_NUMERATOR);

    denominator_box.setMinimum(1);
    denominator_box.setMaximum(MAX_RATIONAL_DENOMINATOR);

    row_layout.addWidget(&numerator_box);
    row_layout.addWidget(&slash_text);
    row_layout.addWidget(&denominator_box);
    row_layout.setContentsMargins(0, 0, 0, 0);
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
  QWidget &o_text = *(new QLabel("o"));
  QSpinBox &octave_box = *(new QSpinBox);

  explicit IntervalEditor(QWidget *const parent_pointer)
      : AbstractRationalEditor(parent_pointer) {
    octave_box.setMinimum(-MAX_OCTAVE);
    octave_box.setMaximum(MAX_OCTAVE);

    row_layout.addWidget(&o_text);
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
  QMetaType::registerConverter<PercussionInstrument, QString>(
      [](const PercussionInstrument &percussion_instrument) -> QString {
        QString result;
        QTextStream stream(&result);
        percussion_instrument_to_stream(stream, percussion_instrument);
        return result;
      });

  auto &factory = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QItemEditorFactory);
  factory.registerEditor(
      qMetaTypeId<Rational>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          RationalEditor>);
  factory.registerEditor(
      qMetaTypeId<PercussionInstrument>(),
      new QStandardItemEditorCreator< // NOLINT(cppcoreguidelines-owning-memory)
          PercussionInstrumentEditor>);
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

  [[nodiscard]] virtual auto get_data(int column_number) const -> QVariant = 0;

  virtual void set_data(int column, const QVariant &new_value) = 0;
  virtual void column_to_json(nlohmann::json &json_row,
                              int column_number) const = 0;
};

template <typename SubRow>
concept RowInterface =
    std::derived_from<SubRow, Row> &&
    requires(SubRow target_row, const SubRow &template_row,
             const nlohmann::json &json_row, int column_number) {
      { SubRow(json_row) } -> std::same_as<SubRow>;
      {
        target_row.copy_column_from(template_row, column_number)
      } -> std::same_as<void>;
      { SubRow::get_number_of_columns() } -> std::same_as<int>;
      { SubRow::get_column_name(column_number) } -> std::same_as<const char *>;
      { SubRow::get_cells_mime() } -> std::same_as<const char *>;
      { SubRow::get_schema() } -> std::same_as<nlohmann::json>;
      { SubRow::is_column_editable(column_number) } -> std::same_as<bool>;
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

template <RowInterface SubRow>
static void partial_json_to_rows(QList<SubRow> &new_rows,
                                 const nlohmann::json &json_rows,
                                 const int number_of_rows) {
  Q_ASSERT(json_rows.is_array());
  std::transform(
      json_rows.cbegin(), json_rows.cbegin() + number_of_rows,
      std::back_inserter(new_rows),
      [](const nlohmann::json &json_row) { return SubRow(json_row); });
}

template <RowInterface SubRow>
static void json_to_rows(QList<SubRow> &rows, const nlohmann::json &json_rows) {
  partial_json_to_rows(rows, json_rows, static_cast<int>(json_rows.size()));
}

template <RowInterface SubRow>
static void add_rows_to_json(nlohmann::json &json_chord,
                             const char *const field_name,
                             const QList<SubRow> &rows) {
  Q_ASSERT(json_chord.is_object());
  if (!rows.empty()) {
    nlohmann::json json_rows = nlohmann::json::array();
    std::transform(rows.cbegin(), rows.cend(), std::back_inserter(json_rows),
                   [](const SubRow &row) -> nlohmann::json {
                     return row_to_json(row, 0,
                                        SubRow::get_number_of_columns() - 1);
                   });
    json_chord[field_name] = std::move(json_rows);
  }
}

template <RowInterface SubRow>
[[nodiscard]] static auto json_field_to_rows(const nlohmann::json &json_object,
                                             const char *const field_name) {
  Q_ASSERT(json_object.is_object());
  if (json_object.contains(field_name)) {
    QList<SubRow> rows;
    json_to_rows(rows, json_object[field_name]);
    return rows;
  }
  return QList<SubRow>();
}

[[nodiscard]] static auto get_milliseconds(const double beats_per_minute,
                                           const Row &row) {
  return row.beats.to_double() * MILLISECONDS_PER_MINUTE / beats_per_minute;
}

struct Note : Row {
  Note() = default;
  explicit Note(const nlohmann::json &json_note) : Row(json_note){};

  [[nodiscard]] virtual auto
  get_closest_midi(QWidget &parent, fluid_sequencer_t &sequencer,
                   fluid_event_t &event, double current_time,
                   double current_key, short current_midi_number,
                   int channel_number, int chord_number,
                   int note_number) const -> std::optional<short> = 0;

  [[nodiscard]] virtual auto get_program_pointer(
      QWidget &parent, const Instrument *current_instrument_pointer,
      const PercussionSet *current_percussion_set_pointer, int chord_number,
      int note_number) const -> const Program * = 0;
};

template <typename SubNote> // type properties
concept NoteInterface = std::derived_from<SubNote, Note> &&
  requires()
{
  { SubNote::get_description() } -> std::same_as<const char *>;
};

template <NoteInterface SubNote>
static void add_note_location(QTextStream &stream, const int chord_number,
                              const int note_number) {
  stream << QObject::tr(" for chord ") << chord_number + 1
         << QObject::tr(SubNote::get_description()) << note_number + 1;
}

void add_percussion_instrument_to_json(
    nlohmann::json &json_row, const char *const field_name,
    const PercussionInstrument &percussion_instrument) {
  if (!(percussion_instrument_is_default(percussion_instrument))) {
    json_row[field_name] = nlohmann::json(
        {{"percussion_set",
          get_reference(percussion_instrument.percussion_set_pointer)
              .name.toStdString()},
         {"midi_number", percussion_instrument.midi_number}});
  }
}

static auto json_field_to_percussion_instrument(const nlohmann::json &json_row,
                                                const char *const field_name)
    -> PercussionInstrument {
  Q_ASSERT(json_row.is_object());
  Q_ASSERT(field_name != nullptr);
  if (json_row.contains(field_name)) {
    const auto &json_percussion_instrument = json_row[field_name];
    return {json_field_to_program_pointer<PercussionSet>(
                json_percussion_instrument, "percussion_set"),
            static_cast<short>(
                get_json_int(json_percussion_instrument, "midi_number"))};
  };
  return {};
}

struct UnpitchedNote : Note {
  PercussionInstrument percussion_instrument;

  UnpitchedNote() = default;

  explicit UnpitchedNote(const nlohmann::json &json_note)
      : Note(json_note),
        percussion_instrument(json_field_to_percussion_instrument(
            json_note, "percussion_instrument")) {}

  [[nodiscard]] static auto get_number_of_columns() -> int {
    return number_of_unpitched_note_columns;
  };

  [[nodiscard]] static auto get_column_name(int column_number) {
    switch (column_number) {
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
    };
  }

  [[nodiscard]] static auto get_cells_mime() {
    return "application/prs.unpitched_notes_cells+json";
  }

  [[nodiscard]] static auto get_schema() {
    auto schema = get_row_fields_schema();
    add_unpitched_fields_to_schema(schema);
    return schema;
  }

  [[nodiscard]] static auto get_description() { return ", unpitched note "; }

  [[nodiscard]] static auto is_column_editable(int /*column_number*/) -> bool {
    return true;
  }

  [[nodiscard]] static auto is_pitched() { return false; }

  [[nodiscard]] auto
  get_closest_midi(QWidget & /*parent*/, fluid_sequencer_t & /*sequencer*/,
                   fluid_event_t & /*event*/, const double /*current_time*/,
                   const double /*current_key*/,
                   const short current_midi_number,
                   const int /*channel_number*/, int /*chord_number*/,
                   int /*note_number*/) const -> std::optional<short> override {
    if (percussion_instrument_is_default(percussion_instrument)) {
      return current_midi_number;
    }
    return percussion_instrument.midi_number;
  };

  [[nodiscard]] auto
  get_program_pointer(QWidget &parent,
                      const Instrument * /*current_instrument_pointer*/,
                      const PercussionSet *current_percussion_set_pointer,
                      const int chord_number,
                      const int note_number) const -> const Program * override {
    if (percussion_instrument_is_default(percussion_instrument)) {
      if (current_percussion_set_pointer == nullptr) {
        QString message;
        QTextStream stream(&message);
        stream << QObject::tr("No percussion set");
        add_note_location<UnpitchedNote>(stream, chord_number, note_number);
        QMessageBox::warning(&parent, "Percussion set error", message);
      }
      return current_percussion_set_pointer;
    }
    return percussion_instrument.percussion_set_pointer;
  };

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override {
    switch (column_number) {
    case unpitched_note_percussion_instrument_column:
      return QVariant::fromValue(percussion_instrument);
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
    case unpitched_note_percussion_instrument_column:
      percussion_instrument = variant_to<PercussionInstrument>(new_value);
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
    case unpitched_note_percussion_instrument_column:
      percussion_instrument = template_row.percussion_instrument;
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
    case unpitched_note_percussion_instrument_column:
      add_percussion_instrument_to_json(
          json_percussion, "percussion_instrument", percussion_instrument);
      break;
    case unpitched_note_beats_column:
      add_abstract_rational_to_json(json_percussion, beats, "beats");
      break;
    case unpitched_note_velocity_ratio_column:
      add_abstract_rational_to_json(json_percussion, velocity_ratio,
                                    "velocity_ratio");
      break;
    case unpitched_note_words_column:
      add_string_to_json(json_percussion, "words", words);
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
        instrument_pointer(
            json_field_to_program_pointer<Instrument>(json_note, "instrument")),
        interval(
            json_field_to_abstract_rational<Interval>(json_note, "interval")) {}

  [[nodiscard]] static auto get_number_of_columns() -> int {
    return number_of_pitched_note_columns;
  };

  [[nodiscard]] static auto get_column_name(int column_number) {
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
    };
  }

  [[nodiscard]] static auto get_cells_mime() {
    return "application/prs.pitched_notes_cells+json";
  }

  [[nodiscard]] static auto get_schema() {
    auto schema = get_row_fields_schema();
    add_pitched_fields_to_schema(schema);
    return schema;
  }

  [[nodiscard]] static auto is_column_editable(int /*column_number*/) -> bool {
    return true;
  }

  [[nodiscard]] static auto get_description() { return ", pitched note "; }

  [[nodiscard]] static auto is_pitched() { return true; }

  [[nodiscard]] auto get_closest_midi(
      QWidget &parent, fluid_sequencer_t &sequencer, fluid_event_t &event,
      const double current_time, const double current_key,
      const short /*current_midi_number*/, const int channel_number,
      const int chord_number,
      const int note_number) const -> std::optional<short> override {
    // TODO(brandon): test this error
    const auto frequency = current_key * interval.to_double();

    static const auto minimum_frequency = get_frequency(0 - QUARTER_STEP);
    if (frequency < minimum_frequency) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Frequency ");
      stream << frequency;
      add_note_location<PitchedNote>(stream, chord_number, note_number);
      stream << QObject::tr(" less than minimum frequency ");
      stream << minimum_frequency;
      QMessageBox::warning(&parent, "Frequency error", message);
      return {};
    }

    static const auto maximum_frequency =
        get_frequency(MAX_MIDI_NUMBER + QUARTER_STEP);
    if (frequency >= maximum_frequency) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Frequency ");
      stream << frequency;
      add_note_location<PitchedNote>(stream, chord_number, note_number);
      stream << QObject::tr(" greater than or equal to maximum frequency ");
      stream << maximum_frequency;
      QMessageBox::warning(&parent, "Frequency error", message);
      return {};
    }

    const auto midi_float = get_midi(frequency);
    const auto closest_midi = static_cast<short>(round(midi_float));
    fluid_event_pitch_bend(
        &event, channel_number,
        to_int((midi_float - closest_midi + ZERO_BEND_HALFSTEPS) *
               BEND_PER_HALFSTEP));
    send_event_at(sequencer, event, current_time);
    return closest_midi;
  }

  [[nodiscard]] auto
  get_program_pointer(QWidget &parent,
                      const Instrument *const current_instrument_pointer,
                      const PercussionSet * /*current_percussion_set_pointer*/,
                      const int chord_number,
                      const int note_number) const -> const Program * override {
    if (instrument_pointer == nullptr) {
      if (current_instrument_pointer == nullptr) {
        QString message;
        QTextStream stream(&message);
        stream << "No instrument";
        add_note_location<PitchedNote>(stream, chord_number, note_number);
        QMessageBox::warning(&parent, "Instrument error", message);
      }
      return current_instrument_pointer;
    }
    return instrument_pointer;
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
      add_program_to_json(json_note, "instrument", instrument_pointer);
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
      add_string_to_json(json_note, "words", words);
      break;
    default:
      Q_ASSERT(false);
      break;
    }
  }
};

struct Chord : public Row {
  const Instrument *instrument_pointer = nullptr;
  PercussionInstrument percussion_instrument;
  Interval interval;
  Rational tempo_ratio;
  QList<PitchedNote> pitched_notes;
  QList<UnpitchedNote> unpitched_notes;

  Chord() = default;

  explicit Chord(const nlohmann::json &json_chord)
      : Row(json_chord),
        instrument_pointer(json_field_to_program_pointer<Instrument>(
            json_chord, "instrument")),
        percussion_instrument(json_field_to_percussion_instrument(
            json_chord, "percussion_instrument")),
        interval(
            json_field_to_abstract_rational<Interval>(json_chord, "interval")),
        tempo_ratio(json_field_to_abstract_rational<Rational>(json_chord,
                                                              "tempo_ratio")),
        pitched_notes(
            json_field_to_rows<PitchedNote>(json_chord, "pitched_notes")),
        unpitched_notes(
            json_field_to_rows<UnpitchedNote>(json_chord, "unpitched_notes")) {}

  [[nodiscard]] static auto get_number_of_columns() -> int {
    return number_of_chord_columns;
  };

  [[nodiscard]] static auto get_column_name(int column_number) {
    switch (column_number) {
    case chord_instrument_column:
      return "Instrument";
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
    };
  }

  [[nodiscard]] static auto get_cells_mime() {
    return "application/prs.chords_cells+json";
  }

  [[nodiscard]] static auto get_schema() {
    auto schema = get_row_fields_schema();
    add_pitched_fields_to_schema(schema);
    add_unpitched_fields_to_schema(schema);
    schema["tempo_ratio"] = get_object_schema(get_rational_fields_schema());
    schema["unpitched_notes"] = get_array_schema(UnpitchedNote::get_schema());
    schema["pitched_notes"] = get_array_schema(PitchedNote::get_schema());
    return schema;
  }

  [[nodiscard]] static auto is_column_editable(int column_number) -> bool {
    return column_number != chord_pitched_notes_column &&
           column_number != chord_unpitched_notes_column;
  }

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override {
    switch (column_number) {
    case chord_instrument_column:
      return QVariant::fromValue(instrument_pointer);
    case chord_percussion_instrument_column:
      return QVariant::fromValue(percussion_instrument);
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
    case chord_percussion_instrument_column:
      percussion_instrument = variant_to<PercussionInstrument>(new_value);
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
    case chord_percussion_instrument_column:
      percussion_instrument = template_row.percussion_instrument;
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
      add_program_to_json(json_chord, "instrument", instrument_pointer);
      break;
    case chord_percussion_instrument_column:
      add_percussion_instrument_to_json(json_chord, "percussion_instrument",
                                        percussion_instrument);
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
      add_string_to_json(json_chord, "words", words);
      break;
    case chord_pitched_notes_column:
      add_rows_to_json(json_chord, "pitched_notes", pitched_notes);
      break;
    case chord_unpitched_notes_column:
      add_rows_to_json(json_chord, "unpitched_notes", unpitched_notes);
      break;
    default:
      Q_ASSERT(false);
    }
  }
};

struct Song {
  double starting_key = DEFAULT_STARTING_KEY;
  double starting_velocity = DEFAULT_STARTING_VELOCITY;
  double starting_tempo = DEFAULT_STARTING_TEMPO;
  QList<Chord> chords;
};

static void set_double(Song &song, fluid_synth_t &synth,
                       const ChangeId control_id, QDoubleSpinBox &spin_box,
                       const double set_value) {
  switch (control_id) {
  case gain_id:
    fluid_synth_set_gain(&synth, static_cast<float>(set_value));
    break;
  case starting_key_id:
    song.starting_key = set_value;
    break;
  case starting_velocity_id:
    song.starting_velocity = set_value;
    break;
  case starting_tempo_id:
    song.starting_tempo = set_value;
    break;
  default:
    Q_ASSERT(false);
    break;
  }
  const QSignalBlocker blocker(spin_box);
  spin_box.setValue(set_value);
}

[[nodiscard]] auto get_quotient_remainder(int dividend,
                                          int divisor) -> std::tuple<int, int> {
  const int quotient = to_int(std::floor((1.0 * dividend) / divisor));
  return std::make_tuple(quotient, dividend - quotient * divisor);
}

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
  const auto [octave, degree] =
      get_quotient_remainder(closest_midi - C_0_MIDI, HALFSTEPS_PER_OCTAVE);
  const auto cents = to_int((midi_float - closest_midi) * CENTS_PER_HALFSTEP);

  static const QMap<int, QString> degrees_to_name{
      {0, QObject::tr("C")},  {1, QObject::tr("C♯")},  {2, QObject::tr("D")},
      {3, QObject::tr("E♭")}, {4, QObject::tr("E")},   {5, QObject::tr("F")},
      {6, QObject::tr("F♯")}, {7, QObject::tr("G")},   {8, QObject::tr("A♭")},
      {9, QObject::tr("A")},  {10, QObject::tr("B♭")}, {11, QObject::tr("B")},
  };

  QString result;
  QTextStream stream(&result);
  stream << key << QObject::tr(" Hz; ") << degrees_to_name[to_int(degree)]
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
  PercussionInstrument current_percussion_instrument;

  double current_time = 0;
  double final_time = 0;

  double current_key = 0;
  double current_velocity = 0;
  double current_tempo = 0;

  fluid_settings_t &settings = []() -> fluid_settings_t & {
    fluid_settings_t &settings = get_reference(new_fluid_settings());
    const auto cores = std::thread::hardware_concurrency();
    set_fluid_int(settings, "synth.midi-channels", NUMBER_OF_MIDI_CHANNELS);
    if (cores > 0) {
      set_fluid_int(settings, "synth.cpu-cores", static_cast<int>(cores));
    }
#ifdef __linux__
    fluid_settings_setstr(&settings, "audio.driver", "pulseaudio");
#endif
    return settings;
  }();
  fluid_event_t &event = get_reference(new_fluid_event());
  fluid_sequencer_t &sequencer = get_reference(new_fluid_sequencer2(0));
  fluid_synth_t &synth = get_reference(new_fluid_synth(&settings));
  const unsigned int soundfont_id = get_soundfont_id(synth);
  const fluid_seq_id_t sequencer_id =
      fluid_sequencer_register_fluidsynth(&sequencer, &synth);

  fluid_audio_driver_t *audio_driver_pointer =
      make_audio_driver(parent, settings, synth);

  // methods
  explicit Player(QWidget &parent_input) : parent(parent_input) {
    fluid_event_set_dest(&event, sequencer_id);
  }

  ~Player() {
    stop_playing(sequencer, event);
    delete_audio_driver(audio_driver_pointer);
    fluid_sequencer_unregister_client(&sequencer, sequencer_id);
    delete_fluid_sequencer(&sequencer);
    delete_fluid_event(&event);
    delete_fluid_synth(&synth);
    delete_fluid_settings(&settings);
  }

  // prevent moving and copying
  Player(const Player &) = delete;
  auto operator=(const Player &) -> Player = delete;
  Player(Player &&) = delete;
  auto operator=(Player &&) -> Player = delete;
};

static void modulate(Player &player, const Chord &chord) {
  player.current_key = player.current_key * chord.interval.to_double();
  player.current_velocity =
      player.current_velocity * chord.velocity_ratio.to_double();
  player.current_tempo = player.current_tempo * chord.tempo_ratio.to_double();
  const auto *chord_instrument_pointer = chord.instrument_pointer;
  if (chord_instrument_pointer != nullptr) {
    player.current_instrument_pointer = chord_instrument_pointer;
  }

  const auto &chord_percussion_instrument = chord.percussion_instrument;
  if (chord_percussion_instrument.percussion_set_pointer != nullptr) {
    player.current_percussion_instrument = chord_percussion_instrument;
  }
}

static void update_final_time(Player &player, const double new_final_time) {
  if (new_final_time > player.final_time) {
    player.final_time = new_final_time;
  }
}

template <NoteInterface SubNote>
[[nodiscard]] static auto play_notes(Player &player, const int chord_number,
                                     const QList<SubNote> &sub_notes,
                                     const int first_note_number,
                                     const int number_of_notes) -> bool {
  auto &parent = player.parent;
  auto &sequencer = player.sequencer;
  auto &event = player.event;
  auto &channel_schedules = player.channel_schedules;
  const auto soundfont_id = player.soundfont_id;

  const auto current_time = player.current_time;
  const auto current_velocity = player.current_velocity;
  const auto current_tempo = player.current_tempo;
  const auto current_key = player.current_key;
  const auto &current_percussion_instrument =
      player.current_percussion_instrument;
  const auto *const current_instrument_pointer =
      player.current_instrument_pointer;

  for (auto note_number = first_note_number;
       note_number < first_note_number + number_of_notes;
       note_number = note_number + 1) {
    const auto channel_number = static_cast<int>(
        std::distance(std::begin(channel_schedules),
                      std::min_element(std::begin(channel_schedules),
                                       std::end(channel_schedules))));
    const auto &sub_note = sub_notes.at(note_number);

    const auto *program_pointer = sub_note.get_program_pointer(
        parent, current_instrument_pointer,
        current_percussion_instrument.percussion_set_pointer, chord_number,
        note_number);
    if (program_pointer == nullptr) {
      return false;
    }
    const auto &program = get_reference(program_pointer);

    fluid_event_program_select(&event, channel_number, soundfont_id,
                               program.bank_number, program.preset_number);
    send_event_at(sequencer, event, current_time);

    const auto maybe_midi_number = sub_note.get_closest_midi(
        parent, sequencer, event, current_time, current_key,
        current_percussion_instrument.midi_number, channel_number, chord_number,
        note_number);
    if (!maybe_midi_number.has_value()) {
      return false;
    }
    auto midi_number = maybe_midi_number.value();

    auto velocity = static_cast<short>(
        std::round(current_velocity * sub_note.velocity_ratio.to_double()));
    if (velocity > MAX_VELOCITY) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Velocity ") << velocity << QObject::tr(" exceeds ")
             << MAX_VELOCITY;
      add_note_location<SubNote>(stream, chord_number, note_number);
      QMessageBox::warning(&parent, QObject::tr("Velocity error"), message);
      return false;
    }
    fluid_event_noteon(&event, channel_number, midi_number, velocity);
    send_event_at(sequencer, event, current_time);

    const auto end_time =
        current_time + get_milliseconds(current_tempo, sub_note);

    fluid_event_noteoff(&event, channel_number, midi_number);
    send_event_at(sequencer, event, end_time);

    channel_schedules[channel_number] = end_time + MAX_RELEASE_TIME;
  }
  return true;
}

template <NoteInterface SubNote>
[[nodiscard]] static auto
play_all_notes(Player &player, const int chord_number,
               const QList<SubNote> &sub_notes) -> bool {
  return play_notes(player, chord_number, sub_notes, 0,
                    static_cast<int>(sub_notes.size()));
}

template <RowInterface SubRow> struct RowsModel : public QAbstractTableModel {
  QItemSelectionModel *selection_model_pointer = nullptr;

  [[nodiscard]] virtual auto is_valid() const -> bool { return true; };
  [[nodiscard]] virtual auto get_rows() const -> QList<SubRow> & = 0;

  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent_index*/) const -> int override {
    if (!is_valid()) {
      return 0;
    }
    return static_cast<int>(get_rows().size());
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
      return SubRow::get_column_name(section);
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
    return SubRow::is_column_editable(index.column())
               ? (uneditable | Qt::ItemIsEditable)
               : uneditable;
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

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
      return get_rows().at(row_number).get_data(index.column());
    }

    return {};
  }

  // don't inline these functions because they use protected methods
  void set_cell(const QModelIndex &set_index, const QVariant &new_value) {
    const auto row_number = set_index.row();
    const auto column_number = set_index.column();

    get_rows()[row_number].set_data(column_number, new_value);
    dataChanged(set_index, set_index);
    get_reference(selection_model_pointer)
        .select(set_index,
                QItemSelectionModel::Select | QItemSelectionModel::Clear);
  }

  void set_cells(const QItemSelectionRange &range,
                 const QList<SubRow> &new_rows) {
    Q_ASSERT(range.isValid());

    auto &rows = get_rows();
    const auto number_of_new_rows = new_rows.size();

    const auto &top_left_index = range.topLeft();
    const auto &bottom_right_index = range.bottomRight();

    const auto first_row_number = range.top();
    const auto left_column = range.left();
    const auto right_column = range.right();

    for (auto replace_number = 0; replace_number < number_of_new_rows;
         replace_number++) {
      auto &row = rows[first_row_number + replace_number];
      const auto &new_row = new_rows.at(replace_number);
      for (auto column_number = left_column; column_number <= right_column;
           column_number++) {
        row.copy_column_from(new_row, column_number);
      }
    }
    dataChanged(top_left_index, bottom_right_index);
    get_reference(selection_model_pointer)
        .select(QItemSelection(top_left_index, bottom_right_index),
                QItemSelectionModel::Select | QItemSelectionModel::Clear);
  }

  void delete_cells(const QItemSelectionRange &range) {
    Q_ASSERT(range.isValid());

    const auto &top_left_index = range.topLeft();
    const auto &bottom_right_index = range.bottomRight();

    const auto first_row_number = range.top();
    const auto left_column = range.left();
    const auto right_column = range.right();
    const auto number_of_rows = get_number_of_rows(range);

    auto &rows = get_rows();
    for (auto replace_number = 0; replace_number < number_of_rows;
         replace_number++) {
      auto &row = rows[first_row_number + replace_number];
      const SubRow empty_row;
      for (auto column_number = left_column; column_number <= right_column;
           column_number++) {
        row.copy_column_from(empty_row, column_number);
      }
    }
    dataChanged(top_left_index, bottom_right_index);
    get_reference(selection_model_pointer)
        .select(QItemSelection(top_left_index, bottom_right_index),
                QItemSelectionModel::Select | QItemSelectionModel::Clear);
  }

  void insert_json_rows(const int first_row_number,
                        const nlohmann::json &json_rows) {
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + static_cast<int>(json_rows.size()) - 1);
    json_to_rows(get_rows(), json_rows);
    endInsertRows();
  }

  void insert_rows(const int first_row_number, const QList<SubRow> &new_rows,
                   const int left_column, const int right_column) {
    auto &rows = get_rows();
    const auto number_of_rows = static_cast<int>(new_rows.size());
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_rows - 1);
    std::copy(new_rows.cbegin(), new_rows.cend(),
              std::inserter(rows, rows.begin() + first_row_number));
    endInsertRows();
    get_reference(selection_model_pointer)
        .select(QItemSelection(
                    index(first_row_number, left_column),
                    index(first_row_number + number_of_rows - 1, right_column)),
                QItemSelectionModel::Select | QItemSelectionModel::Clear);
  }

  void insert_row(const int row_number, SubRow new_row) {
    beginInsertRows(QModelIndex(), row_number, row_number);
    auto &rows = get_rows();
    rows.insert(rows.begin() + row_number, std::move(new_row));
    endInsertRows();
    get_reference(selection_model_pointer)
        .select(index(row_number, 0), QItemSelectionModel::Select |
                                          QItemSelectionModel::Clear |
                                          QItemSelectionModel::Rows);
  }

  void remove_rows(const int first_row_number, int number_of_rows) {
    auto &rows = get_rows();
    beginRemoveRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_rows - 1);
    rows.erase(rows.begin() + first_row_number,
               rows.begin() + first_row_number + number_of_rows);
    endRemoveRows();
  }
};

template <RowInterface SubRow>
static void clear_rows(RowsModel<SubRow> &rows_model) {
  const auto number_of_rows = rows_model.rowCount(QModelIndex());
  if (number_of_rows > 0) {
    rows_model.remove_rows(0, number_of_rows);
  }
}

template <RowInterface SubRow>
static void
copy_from_model(QMimeData &mime_data, const RowsModel<SubRow> &rows_model,
                const int first_row_number, const int number_of_rows,
                const int left_column, const int right_column) {
  const auto &rows = rows_model.get_rows();
  nlohmann::json copied_json = nlohmann::json::array();
  std::transform(
      rows.cbegin() + first_row_number,
      rows.cbegin() + first_row_number + number_of_rows,
      std::back_inserter(copied_json),
      [left_column, right_column](const SubRow &row) -> nlohmann::json {
        return row_to_json(row, left_column, right_column);
      });

  const nlohmann::json copied({{"left_column", left_column},
                               {"right_column", right_column},
                               {"rows", std::move(copied_json)}});

  std::stringstream json_text;
  json_text << std::setw(4) << copied;

  mime_data.setData(SubRow::get_cells_mime(), json_text.str().c_str());
}

template <RowInterface SubRow> struct SetCell : public QUndoCommand {
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

template <RowInterface SubRow> struct DeleteCells : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const QItemSelectionRange range;
  const QList<SubRow> old_rows;

  explicit DeleteCells(RowsModel<SubRow> &rows_model_input,
                       QItemSelectionRange range_input)
      : rows_model(rows_model_input), range(std::move(range_input)),
        old_rows(copy_items(rows_model.get_rows(), range.top(),
                            get_number_of_rows(range))) {}

  void undo() override { rows_model.set_cells(range, old_rows); }

  void redo() override { rows_model.delete_cells(range); }
};

template <RowInterface SubRow> struct SetCells : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int first_row_number;
  const int number_of_rows;
  const int left_column;
  const int right_column;
  const QList<SubRow> old_rows;
  const QList<SubRow> new_rows;

  explicit SetCells(RowsModel<SubRow> &rows_model_input,
                    const int first_row_number_input,
                    const int number_of_rows_input, const int left_column_input,
                    const int right_column_input, QList<SubRow> old_rows_input,
                    QList<SubRow> new_rows_input)
      : rows_model(rows_model_input), first_row_number(first_row_number_input),
        number_of_rows(number_of_rows_input), left_column(left_column_input),
        right_column(right_column_input), old_rows(std::move(old_rows_input)),
        new_rows(std::move(new_rows_input)) {}

  void undo() override {
    rows_model.set_cells(
        QItemSelectionRange(
            rows_model.index(first_row_number, left_column),
            rows_model.index(first_row_number + number_of_rows - 1,
                             right_column)),
        old_rows);
  }

  void redo() override {
    rows_model.set_cells(
        QItemSelectionRange(
            rows_model.index(first_row_number, left_column),
            rows_model.index(first_row_number + number_of_rows - 1,
                             right_column)),
        new_rows);
  }
};

[[nodiscard]] static auto get_mime_description(const QString &mime_type) {
  Q_ASSERT(mime_type.isValidUtf16());
  if (mime_type == Chord::get_cells_mime()) {
    return RowsModel<Chord>::tr("chords cells");
  }
  if (mime_type == PitchedNote::get_cells_mime()) {
    return RowsModel<PitchedNote>::tr("pitched notes cells");
  }
  if (mime_type == UnpitchedNote::get_cells_mime()) {
    return RowsModel<UnpitchedNote>::tr("unpitched notes cells");
  }
  return mime_type;
}

template <RowInterface SubRow> struct Cells {
  const int left_column;
  const int right_column;
  const QList<SubRow> rows;
  Cells(const int left_column_input, const int right_column_input,
        QList<SubRow> rows_input)
      : left_column(left_column_input), right_column(right_column_input),
        rows(std::move(rows_input)) {}
};

template <RowInterface SubRow>
[[nodiscard]] static auto
parse_clipboard(QWidget &parent,
                const int max_rows = std::numeric_limits<int>::max())
    -> std::optional<Cells<SubRow>> {
  const auto &mime_data =
      get_reference(get_reference(QGuiApplication::clipboard()).mimeData());
  const auto *mime_type = SubRow::get_cells_mime();
  if (!mime_data.hasFormat(mime_type)) {
    const auto formats = mime_data.formats();
    if (formats.empty()) {
      QMessageBox::warning(&parent, QObject::tr("Empty paste error"),
                           QObject::tr("Nothing to paste!"));
      return {};
    };
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Cannot paste ") << get_mime_description(formats[0])
           << QObject::tr(" as ") << get_mime_description(mime_type);
    QMessageBox::warning(&parent, QObject::tr("MIME type error"), message);
    return {};
  }
  const auto &copied_text = mime_data.data(mime_type).toStdString();
  nlohmann::json copied;
  try {
    copied = nlohmann::json::parse(copied_text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    QMessageBox::warning(&parent, QObject::tr("Parsing error"),
                         parse_error.what());
    return {};
  }
  if (copied.empty()) {
    QMessageBox::warning(&parent, QObject::tr("Empty paste"),
                         QObject::tr("Nothing to paste!"));
    return {};
  }
  static const auto validator = []() {
    const auto last_column = SubRow::get_number_of_columns() - 1;
    return make_validator(
        nlohmann::json({"left_column", "right_column", "rows"}),
        nlohmann::json(
            {{"left_column", get_number_schema("integer", 0, last_column)},
             {"right_column", get_number_schema("integer", 0, last_column)},
             {"rows", get_array_schema(SubRow::get_schema())}}));
  }();
  try {
    validator.validate(copied);
  } catch (const std::exception &error) {
    QMessageBox::warning(&parent, QObject::tr("Schema error"), error.what());
    return {};
  }
  const auto &json_rows = get_json_value(copied, "rows");
  QList<SubRow> new_rows;
  partial_json_to_rows(
      new_rows, json_rows,
      std::min({static_cast<int>(json_rows.size()), max_rows}));
  return Cells(get_json_int(copied, "left_column"),
               get_json_int(copied, "right_column"), std::move(new_rows));
}

[[nodiscard]] static auto get_selection_model(
    const QAbstractItemView &item_view) -> QItemSelectionModel & {
  return get_reference(item_view.selectionModel());
}

template <RowInterface SubRow>
[[nodiscard]] static auto
make_set_cells_command(RowsModel<SubRow> &rows_model,
                       const int first_row_number, const int number_of_rows,
                       const int left_column, const int right_column,
                       QList<SubRow> new_rows) -> QUndoCommand * {
  auto &rows = rows_model.get_rows();
  return new SetCells( // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, first_row_number, number_of_rows, left_column, right_column,
      copy_items(rows, first_row_number, number_of_rows), std::move(new_rows));
}

template <RowInterface SubRow>
[[nodiscard]] static auto
make_paste_cells_command(QWidget &parent, const int first_row_number,
                         RowsModel<SubRow> &rows_model) -> QUndoCommand * {
  auto &rows = rows_model.get_rows();
  auto maybe_cells = parse_clipboard<SubRow>(
      parent, static_cast<int>(rows.size()) - first_row_number);
  if (!maybe_cells.has_value()) {
    return nullptr;
  }
  auto &cells = maybe_cells.value();
  auto &copy_rows = cells.rows;
  const auto number_copied = static_cast<int>(copy_rows.size());
  return make_set_cells_command(rows_model, first_row_number, number_copied,
                                cells.left_column, cells.right_column,
                                std::move(copy_rows));
}

template <RowInterface SubRow> struct InsertRow : public QUndoCommand {
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

template <RowInterface SubRow>
static void
insert_or_remove(RowsModel<SubRow> &rows_model, const int first_row_number,
                 const QList<SubRow> &new_rows, const int left_column,
                 const int right_column, const bool should_insert) {
  if (should_insert) {
    rows_model.insert_rows(first_row_number, new_rows, left_column,
                           right_column);
  } else {
    rows_model.remove_rows(first_row_number, new_rows.size());
  }
}

template <RowInterface SubRow> struct InsertRemoveRows : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int first_row_number;
  const QList<SubRow> new_rows;
  const int left_column;
  const int right_column;
  const bool backwards;
  InsertRemoveRows(RowsModel<SubRow> &rows_model_input,
                   const int first_row_number_input,
                   QList<SubRow> new_rows_input, const int left_column_input,
                   const int right_column_input, const bool backwards_input)
      : rows_model(rows_model_input), first_row_number(first_row_number_input),
        new_rows(std::move(new_rows_input)), left_column(left_column_input),
        right_column(right_column_input), backwards(backwards_input) {}

  void undo() override {
    insert_or_remove(rows_model, first_row_number, new_rows, left_column,
                     right_column, backwards);
  }

  void redo() override {
    insert_or_remove(rows_model, first_row_number, new_rows, left_column,
                     right_column, !backwards);
  }
};

template <RowInterface SubRow>
[[nodiscard]] static auto
make_paste_insert_command(QWidget &parent, RowsModel<SubRow> &rows_model,
                          const int row_number) -> QUndoCommand * {
  const auto maybe_cells = parse_clipboard<SubRow>(parent);
  if (!maybe_cells.has_value()) {
    return nullptr;
  }
  auto &cells = maybe_cells.value();
  return new InsertRemoveRows( // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, row_number, std::move(cells.rows), cells.left_column,
      cells.right_column, false);
}

template <RowInterface SubRow>
[[nodiscard]] static auto
make_remove_command(RowsModel<SubRow> &rows_model, const int first_row_number,
                    const int number_of_rows) -> QUndoCommand * {
  return new InsertRemoveRows( // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, first_row_number,
      copy_items(rows_model.get_rows(), first_row_number, number_of_rows), 0,
      SubRow::get_number_of_columns(), true);
}

template <RowInterface SubRow> struct UndoRowsModel : public RowsModel<SubRow> {
  QUndoStack &undo_stack;

  explicit UndoRowsModel(QUndoStack &undo_stack_input)
      : undo_stack(undo_stack_input){};

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
};

static void rekey_selection(QList<Chord> &copied_chords,
                            const int number_of_chords,
                            const Interval &interval_ratio) {
  auto &first_chord = copied_chords[0];
  first_chord.interval = first_chord.interval * interval_ratio;
  for (auto index = 0; index < number_of_chords; index++) {
    for (auto &pitched_note : copied_chords[index].pitched_notes) {
      pitched_note.interval = pitched_note.interval / interval_ratio;
    }
  }
}

[[nodiscard]] static auto
make_rekey_command(UndoRowsModel<Chord> &chords_model,
                   const Interval &interval_ratio, const int first_chord_number,
                   const int number_of_chords) -> QUndoCommand * {
  const auto &chords = chords_model.get_rows();

  if (static_cast<int>(chords.size()) > first_chord_number + number_of_chords) {
    const auto plus_one = number_of_chords + 1;
    auto copied_chords = copy_items(chords, first_chord_number, plus_one);
    rekey_selection(copied_chords, number_of_chords, interval_ratio);
    auto &last_chord = copied_chords[number_of_chords];
    last_chord.interval = last_chord.interval / interval_ratio;
    return make_set_cells_command(
        chords_model, first_chord_number, plus_one, chord_pitched_notes_column,
        chord_interval_column, std::move(copied_chords));
  }
  auto copied_chords = copy_items(chords, first_chord_number, number_of_chords);
  rekey_selection(copied_chords, number_of_chords, interval_ratio);
  return make_set_cells_command(chords_model, first_chord_number,
                                number_of_chords, chord_pitched_notes_column,
                                chord_interval_column,
                                std::move(copied_chords));
}

struct ChordsModel : public UndoRowsModel<Chord> {
  Song &song;
  bool rekey_mode = false;

  explicit ChordsModel(QUndoStack &undo_stack, Song &song_input)
      : UndoRowsModel(undo_stack), song(song_input) {}

  [[nodiscard]] auto get_rows() const -> QList<Chord> & override {
    return song.chords;
  };

  [[nodiscard]] auto
  get_status(const int row_number) const -> QString override {
    return get_key_text(song, row_number);
  }

  [[nodiscard]] auto setData(const QModelIndex &new_index,
                             const QVariant &new_value,
                             const int role) -> bool override {
    if (role != Qt::EditRole) {
      return false;
    }
    if (rekey_mode && new_index.column() == chord_interval_column) {
      const auto chord_number = new_index.row();
      undo_stack.push(make_rekey_command(*this,
                                         variant_to<Interval>(new_value) /
                                             get_rows()[chord_number].interval,
                                         chord_number, 1));
      return true;
    }
    return UndoRowsModel::setData(new_index, new_value, role);
  }
};

template <NoteInterface SubNote>
struct NotesModel : public UndoRowsModel<SubNote> {
  QList<SubNote> *rows_pointer = nullptr;
  int parent_chord_number = -1;

  explicit NotesModel(QUndoStack &undo_stack)
      : UndoRowsModel<SubNote>(undo_stack) {}

  [[nodiscard]] auto is_valid() const -> bool override {
    return rows_pointer != nullptr;
  };

  [[nodiscard]] auto get_rows() const -> QList<SubNote> & override {
    return get_reference(rows_pointer);
  };

  void set_rows_pointer(QList<SubNote> *const new_rows_pointer = nullptr,
                        const int new_parent_chord_number = -1) {
    NotesModel::beginResetModel();
    rows_pointer = new_rows_pointer;
    parent_chord_number = new_parent_chord_number;
    NotesModel::endResetModel();
  }
};

template <NoteInterface SubNote>
static auto make_insert_note(NotesModel<SubNote> &notes_model,
                             const QList<Chord> &chords,
                             const int row_number) -> QUndoCommand * {
  SubNote sub_note;
  sub_note.beats = chords[notes_model.parent_chord_number].beats;
  return new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
      notes_model, row_number, std::move(sub_note));
}

struct PitchedNotesModel : public NotesModel<PitchedNote> {
  Song &song;

  explicit PitchedNotesModel(QUndoStack &undo_stack, Song &song_input)
      : NotesModel<PitchedNote>(undo_stack), song(song_input) {}

  [[nodiscard]] auto
  get_status(const int row_number) const -> QString override {
    return get_key_text(
        song, parent_chord_number,
        get_reference(rows_pointer).at(row_number).interval.to_double());
  }
};

struct UnpitchedNotesModel : public NotesModel<UnpitchedNote> {
  explicit UnpitchedNotesModel(QUndoStack &undo_stack)
      : NotesModel<UnpitchedNote>(undo_stack) {}
};

struct MyTable : public QTableView {
  MyTable() {
    auto &horizontal_header = get_reference(horizontalHeader());
    auto &vertical_header = get_reference(verticalHeader());

    setSelectionMode(QAbstractItemView::ContiguousSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSizeAdjustPolicy(SizeAdjustPolicy::AdjustToContents);

    horizontal_header.setSectionResizeMode(QHeaderView::Fixed);
    vertical_header.setSectionResizeMode(QHeaderView::Fixed);

    horizontal_header.setStretchLastSection(true);

    setMouseTracking(true);
  }

  [[nodiscard]] auto sizeHint() const -> QSize override {
    const QScrollBar &vertical_scroll_bar = get_reference(verticalScrollBar());
    const QScrollBar &horizontal_scroll_bar =
        get_reference(horizontalScrollBar());
    const auto &viewport_size = viewportSizeHint();
    const auto double_frame_width = 2 * frameWidth();
    return {
        double_frame_width + viewport_size.width() +
            (vertical_scroll_bar.isVisible() ? vertical_scroll_bar.width() : 0),
        double_frame_width + viewport_size.height() +
            (horizontal_scroll_bar.isVisible() ? horizontal_scroll_bar.height()
                                               : 0)};
  }

  [[nodiscard]] auto minimumSizeHint() const -> QSize override {
    return {0, 0};
  }
};

template <RowInterface SubRow>
static void set_model(QAbstractItemView &item_view,
                      RowsModel<SubRow> &rows_model) {
  item_view.setModel(&rows_model);
  rows_model.selection_model_pointer = item_view.selectionModel();
}

struct ChordsTable : public MyTable {
  ChordsModel model;
  ChordsTable(QUndoStack &undo_stack, Song &song)
      : model(ChordsModel(undo_stack, song)) {
    const auto &interval_size = get_minimum_size<IntervalEditor>();
    const auto &rational_size = get_minimum_size<RationalEditor>();
    const auto &instrument_size = get_minimum_size<InstrumentEditor>();
    const auto &percussion_instrument_size =
        get_minimum_size<PercussionInstrumentEditor>();

    const auto rational_width = rational_size.width();

    set_model(*this, model);

    setColumnWidth(chord_instrument_column, instrument_size.width());
    setColumnWidth(chord_percussion_instrument_column,
                   percussion_instrument_size.width());
    setColumnWidth(chord_interval_column, interval_size.width());
    setColumnWidth(chord_beats_column, rational_width);
    setColumnWidth(chord_velocity_ratio_column, rational_width);
    setColumnWidth(chord_tempo_ratio_column, rational_width);
    resizeColumnToContents(chord_pitched_notes_column);
    resizeColumnToContents(chord_unpitched_notes_column);
    setColumnWidth(chord_words_column, WORDS_WIDTH);

    get_reference(verticalHeader())
        .setDefaultSectionSize(std::max(
            {instrument_size.height(), percussion_instrument_size.height(),
             rational_size.height(), interval_size.height()}));
  }
};

struct UnpitchedNotesTable : public MyTable {
  UnpitchedNotesModel model;
  explicit UnpitchedNotesTable(QUndoStack &undo_stack)
      : model(UnpitchedNotesModel(undo_stack)) {
    const auto &rational_size = get_minimum_size<RationalEditor>();
    const auto &percussion_instrument_size =
        get_minimum_size<PercussionInstrumentEditor>();

    const auto rational_width = rational_size.width();

    set_model(*this, model);

    setColumnWidth(unpitched_note_percussion_instrument_column,
                   percussion_instrument_size.width());
    setColumnWidth(unpitched_note_beats_column, rational_width);
    setColumnWidth(unpitched_note_velocity_ratio_column, rational_width);
    setColumnWidth(unpitched_note_words_column, WORDS_WIDTH);

    get_reference(verticalHeader())
        .setDefaultSectionSize(std::max(
            {rational_size.height(), percussion_instrument_size.height()}));
  }
};

struct PitchedNotesTable : public MyTable {
  PitchedNotesModel model;
  PitchedNotesTable(QUndoStack &undo_stack, Song &song)
      : model(PitchedNotesModel(undo_stack, song)) {
    const auto &interval_size = get_minimum_size<IntervalEditor>();
    const auto &rational_size = get_minimum_size<RationalEditor>();
    const auto &instrument_size = get_minimum_size<InstrumentEditor>();

    const auto rational_width = rational_size.width();

    set_model(*this, model);

    setColumnWidth(pitched_note_instrument_column, instrument_size.width());
    setColumnWidth(pitched_note_interval_column, interval_size.width());
    setColumnWidth(pitched_note_beats_column, rational_width);
    setColumnWidth(pitched_note_velocity_ratio_column, rational_width);
    setColumnWidth(pitched_note_words_column, WORDS_WIDTH);

    get_reference(verticalHeader())
        .setDefaultSectionSize(
            std::max({rational_size.height(), instrument_size.height(),
                      interval_size.height()}));
  }
};

struct SwitchColumn : public QWidget {
  RowType current_row_type = chord_type;

  QLabel &editing_text = *(new QLabel(SwitchColumn::tr("Chords")));
  ChordsTable &chords_table;
  PitchedNotesTable &pitched_notes_table;
  UnpitchedNotesTable &unpitched_notes_table;

  QBoxLayout &column_layout = *(new QVBoxLayout(this));

  SwitchColumn(QUndoStack &undo_stack, Song &song)
      : chords_table(*new ChordsTable(undo_stack, song)),
        pitched_notes_table(*new PitchedNotesTable(undo_stack, song)),
        unpitched_notes_table(*new UnpitchedNotesTable(undo_stack)) {
    column_layout.addWidget(&editing_text);
    column_layout.addWidget(&chords_table);
    column_layout.addWidget(&pitched_notes_table);
    column_layout.addWidget(&unpitched_notes_table);
  }
};

[[nodiscard]] static auto
get_table(const SwitchColumn &switch_column) -> QTableView & {
  auto &chords_table = switch_column.chords_table;
  switch (switch_column.current_row_type) {
  case chord_type:
    return chords_table;
  case pitched_note_type:
    return switch_column.pitched_notes_table;
  case unpitched_note_type:
    return switch_column.unpitched_notes_table;
  default:
    Q_ASSERT(false);
    return chords_table;
  }
}

[[nodiscard]] static auto
get_parent_chord_number(const SwitchColumn &switch_column) -> int {
  switch (switch_column.current_row_type) {
  case chord_type:
    return -1;
  case pitched_note_type:
    return switch_column.pitched_notes_table.model.parent_chord_number;
  case unpitched_note_type:
    return switch_column.unpitched_notes_table.model.parent_chord_number;
  default:
    Q_ASSERT(false);
    return -1;
  }
}

[[nodiscard]] static auto get_only_range(const SwitchColumn &switch_column) {
  return get_only(get_selection_model(get_table(switch_column)).selection());
}

static void copy_selection(const SwitchColumn &switch_column) {
  const auto &range = get_only_range(switch_column);
  const auto first_row_number = range.top();
  const auto number_of_rows = get_number_of_rows(range);
  const auto left_column = range.left();
  const auto right_column = range.right();
  auto &mime_data = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QMimeData);

  switch (switch_column.current_row_type) {
  case chord_type:
    copy_from_model(mime_data, switch_column.chords_table.model,
                    first_row_number, number_of_rows, left_column,
                    right_column);
    break;
  case pitched_note_type:
    copy_from_model(mime_data, switch_column.pitched_notes_table.model,
                    first_row_number, number_of_rows, left_column,
                    right_column);
    break;
  case unpitched_note_type:
    copy_from_model(mime_data, switch_column.unpitched_notes_table.model,
                    first_row_number, number_of_rows, left_column,
                    right_column);
    break;
  default:
    Q_ASSERT(false);
    return;
  }
  get_reference(QGuiApplication::clipboard()).setMimeData(&mime_data);
}

struct SetDouble : public QUndoCommand {
  Song &song;
  fluid_synth_t &synth;
  QDoubleSpinBox &spin_box;
  const ChangeId control_id;
  const double old_value;
  double new_value;

  explicit SetDouble(Song &song_input, fluid_synth_t &synth_input,
                     QDoubleSpinBox &spin_box_input,
                     const ChangeId command_id_input,
                     const double old_value_input, const double new_value_input)
      : song(song_input), synth(synth_input), spin_box(spin_box_input),
        control_id(command_id_input), old_value(old_value_input),
        new_value(new_value_input) {}

  [[nodiscard]] auto id() const -> int override { return control_id; }

  [[nodiscard]] auto
  mergeWith(const QUndoCommand *const next_command_pointer) -> bool override {
    Q_ASSERT(next_command_pointer != nullptr);
    new_value =
        get_reference(dynamic_cast<const SetDouble *>(next_command_pointer))
            .new_value;
    return true;
  }

  void undo() override {
    set_double(song, synth, control_id, spin_box, old_value);
  }

  void redo() override {
    set_double(song, synth, control_id, spin_box, new_value);
  }
};

static void add_set_double(QUndoStack &undo_stack, Song &song,
                           fluid_synth_t &synth, QDoubleSpinBox &spin_box,
                           const ChangeId control_id, const double old_value,
                           const double new_value) {
  undo_stack.push(new SetDouble( // NOLINT(cppcoreguidelines-owning-memory)
      song, synth, spin_box, control_id, old_value, new_value));
}

struct SpinBoxes : public QWidget {
  QDoubleSpinBox &gain_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_key_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_velocity_editor = *(new QDoubleSpinBox);
  QDoubleSpinBox &starting_tempo_editor = *(new QDoubleSpinBox);
  QFormLayout &spin_boxes_form = *(new QFormLayout(this));

  explicit SpinBoxes(Song &song, fluid_synth_t &synth, QUndoStack &undo_stack) {
    auto &gain_editor = this->gain_editor;
    auto &starting_key_editor = this->starting_key_editor;
    auto &starting_velocity_editor = this->starting_velocity_editor;
    auto &starting_tempo_editor = this->starting_tempo_editor;

    add_control(spin_boxes_form, SpinBoxes::tr("&Gain:"), gain_editor, 0,
                MAX_GAIN, SpinBoxes::tr("/10"), GAIN_STEP, 1);
    add_control(spin_boxes_form, SpinBoxes::tr("Starting &key:"),
                starting_key_editor, 1, MAX_STARTING_KEY, SpinBoxes::tr(" hz"));
    add_control(spin_boxes_form, SpinBoxes::tr("Starting &velocity:"),
                starting_velocity_editor, 1, MAX_VELOCITY,
                SpinBoxes::tr("/127"));
    add_control(spin_boxes_form, SpinBoxes::tr("Starting &tempo:"),
                starting_tempo_editor, 1, MAX_STARTING_TEMPO,
                SpinBoxes::tr(" bpm"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QObject::connect(
        &gain_editor, &QDoubleSpinBox::valueChanged, this,
        [&undo_stack, &song, &synth, &gain_editor](double new_value) {
          add_set_double(undo_stack, song, synth, gain_editor, gain_id,
                         fluid_synth_get_gain(&synth), new_value);
        });
    QObject::connect(
        &starting_key_editor, &QDoubleSpinBox::valueChanged, this,
        [&undo_stack, &song, &synth, &starting_key_editor](double new_value) {
          add_set_double(undo_stack, song, synth, starting_key_editor,
                         starting_key_id, song.starting_key, new_value);
        });
    QObject::connect(
        &starting_velocity_editor, &QDoubleSpinBox::valueChanged, this,
        [&undo_stack, &song, &synth,
         &starting_velocity_editor](double new_value) {
          add_set_double(undo_stack, song, synth, starting_velocity_editor,
                         starting_velocity_id, song.starting_velocity,
                         new_value);
        });
    QObject::connect(
        &starting_tempo_editor, &QDoubleSpinBox::valueChanged, this,
        [&undo_stack, &song, &synth, &starting_tempo_editor](double new_value) {
          add_set_double(undo_stack, song, synth, starting_tempo_editor,
                         starting_tempo_id, song.starting_tempo, new_value);
        });

    gain_editor.setValue(DEFAULT_GAIN);
    starting_key_editor.setValue(song.starting_key);
    starting_velocity_editor.setValue(song.starting_velocity);
    starting_tempo_editor.setValue(song.starting_tempo);

    clear_and_clean(undo_stack);
  }
};

static void update_interval(QUndoStack &undo_stack, SwitchColumn &switch_column,
                            const Interval &interval) {
  const auto &range = get_only_range(switch_column);
  const auto first_row_number = range.top();
  const auto number_of_rows = get_number_of_rows(range);

  const auto current_row_type = switch_column.current_row_type;
  QUndoCommand *undo_command = nullptr;
  if (current_row_type == chord_type) {
    auto &chords_model = switch_column.chords_table.model;
    if (chords_model.rekey_mode) {
      undo_command = make_rekey_command(chords_model, interval,
                                        first_row_number, number_of_rows);
    } else {
      auto new_chords =
          copy_items(chords_model.get_rows(), first_row_number, number_of_rows);
      for (auto &chord : new_chords) {
        chord.interval = chord.interval * interval;
      }
      undo_command = make_set_cells_command(
          chords_model, first_row_number, number_of_rows, chord_interval_column,
          chord_interval_column, std::move(new_chords));
    }
  } else if (current_row_type == pitched_note_type) {
    auto &pitched_notes_model = switch_column.pitched_notes_table.model;
    auto new_pitched_notes = copy_items(pitched_notes_model.get_rows(),
                                        first_row_number, number_of_rows);
    for (auto &pitched_note : new_pitched_notes) {
      pitched_note.interval = pitched_note.interval * interval;
    }
    undo_command = make_set_cells_command(
        pitched_notes_model, first_row_number, number_of_rows,
        pitched_note_interval_column, pitched_note_interval_column,
        std::move(new_pitched_notes));
  } else {
    Q_ASSERT(false);
  }
  undo_stack.push(undo_command);
}

static void make_square(QPushButton &button) {
  button.setFixedWidth(button.sizeHint().height());
}

struct IntervalRow : public QWidget {
  QUndoStack &undo_stack;
  SwitchColumn &switch_column;
  QBoxLayout &row_layout = *(new QHBoxLayout(this));
  QPushButton &minus_button = *(new QPushButton("−", this));
  QLabel &text;
  QPushButton &plus_button = *(new QPushButton("+", this));
  const Interval interval;

  IntervalRow(QUndoStack &undo_stack_input, SwitchColumn &switch_column_input,
              const char *const interval_name, Interval interval_input)
      : undo_stack(undo_stack_input), switch_column(switch_column_input),
        text(*(new QLabel(interval_name, this))),
        interval(std::move(interval_input)) {
    make_square(minus_button);
    make_square(plus_button);
    row_layout.addWidget(&minus_button);
    row_layout.addWidget(&text);
    row_layout.addWidget(&plus_button);

    auto &switch_column = this->switch_column;
    auto &undo_stack = this->undo_stack;
    const auto &interval = this->interval;

    QObject::connect(&minus_button, &QPushButton::released, this,
                     [&undo_stack, &switch_column, &interval]() {
                       update_interval(undo_stack, switch_column,
                                       Interval(1, 1, 0) / interval);
                     });

    QObject::connect(&plus_button, &QPushButton::released, this,
                     [&undo_stack, &switch_column, &interval]() {
                       update_interval(undo_stack, switch_column, interval);
                     });
  }
};

static void set_enabled(IntervalRow &interval_row, bool enabled) {
  interval_row.minus_button.setEnabled(enabled);
  interval_row.plus_button.setEnabled(enabled);
}

static void set_rows_enabled(IntervalRow &third_row, IntervalRow &fifth_row,
                             IntervalRow &seventh_row, IntervalRow &octave_row,
                             bool enabled) {
  set_enabled(third_row, enabled);
  set_enabled(fifth_row, enabled);
  set_enabled(seventh_row, enabled);
  set_enabled(octave_row, enabled);
}

struct ControlsColumn : public QWidget {
  SpinBoxes &spin_boxes;
  IntervalRow &third_row;
  IntervalRow &fifth_row;
  IntervalRow &seventh_row;
  IntervalRow &octave_row;
  QBoxLayout &column_layout = *(new QVBoxLayout(this));

  ControlsColumn(Song &song, fluid_synth_t &synth, QUndoStack &undo_stack,
                 SwitchColumn &switch_column)
      : spin_boxes(*new SpinBoxes(song, synth, undo_stack)),
        third_row(*new IntervalRow(undo_stack, switch_column, "Major third",
                                   Interval(FIVE, 1, -2))),
        fifth_row(*new IntervalRow(undo_stack, switch_column, "Perfect fifth",
                                   Interval(3, 1, -1))),
        seventh_row(*new IntervalRow(undo_stack, switch_column,
                                     "Harmonic seventh",
                                     Interval(SEVEN, 1, -2))),
        octave_row(*new IntervalRow(undo_stack, switch_column, "Octave",
                                    Interval(1, 1, 1))) {
    column_layout.addWidget(&spin_boxes);
    column_layout.addWidget(&third_row);
    column_layout.addWidget(&fifth_row);
    column_layout.addWidget(&seventh_row);
    column_layout.addWidget(&octave_row);
    set_rows_enabled(third_row, fifth_row, seventh_row, octave_row, false);
  }
};

struct SongWidget : public QWidget {
  Song song;
  Player player = Player(*this);
  QUndoStack undo_stack = QUndoStack(nullptr);
  QString current_file;
  QString current_folder =
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

  SwitchColumn &switch_column = *(new SwitchColumn(undo_stack, song));
  ControlsColumn &controls_column =
      *(new ControlsColumn(song, player.synth, undo_stack, switch_column));
  QBoxLayout &row_layout = *(new QHBoxLayout(this));

  explicit SongWidget() {
    row_layout.addWidget(&controls_column, 0, Qt::AlignTop);
    row_layout.addWidget(&switch_column, 0, Qt::AlignTop);
  }

  ~SongWidget() override { undo_stack.disconnect(); }

  // prevent moving and copying
  SongWidget(const SongWidget &) = delete;
  auto operator=(const SongWidget &) -> SongWidget = delete;
  SongWidget(SongWidget &&) = delete;
  auto operator=(SongWidget &&) -> SongWidget = delete;
};

auto get_gain(const SongWidget &song_widget) -> double {
  return fluid_synth_get_gain(&song_widget.player.synth);
}

[[nodiscard]] static auto get_next_row(const SongWidget &song_widget) {
  return get_only_range(song_widget.switch_column).bottom() + 1;
}

static void initialize_play(SongWidget &song_widget) {
  auto &player = song_widget.player;
  const auto &song = song_widget.song;

  player.current_instrument_pointer = nullptr;
  player.current_percussion_instrument = PercussionInstrument(nullptr, 0);
  player.current_key = song.starting_key;
  player.current_velocity = song.starting_velocity;
  player.current_tempo = song.starting_tempo;
  player.current_time = fluid_sequencer_get_tick(&player.sequencer);

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
    const auto pitched_result =
        play_all_notes(player, chord_number, chord.pitched_notes);
    if (!pitched_result) {
      return;
    }
    const auto unpitched_result =
        play_all_notes(player, chord_number, chord.unpitched_notes);
    if (!unpitched_result) {
      return;
    }
    const auto new_current_time =
        player.current_time + get_milliseconds(player.current_tempo, chord);
    player.current_time = new_current_time;
    update_final_time(player, new_current_time);
  }
}

void export_to_file(SongWidget &song_widget, const QString &output_file) {
  Q_ASSERT(output_file.isValidUtf16());
  auto &player = song_widget.player;
  const auto &song = song_widget.song;

  auto &settings = player.settings;
  auto &event = player.event;
  auto &sequencer = player.sequencer;

  stop_playing(sequencer, event);
  delete_audio_driver(player.audio_driver_pointer);
  check_fluid_ok(fluid_settings_setstr(&settings, "audio.file.name",
                                       output_file.toStdString().c_str()));

  set_fluid_int(settings, "synth.lock-memory", 0);

  auto finished = false;
  const auto finished_timer_id = fluid_sequencer_register_client(
      &player.sequencer, "finished timer",
      [](unsigned int /*time*/, fluid_event_t * /*event*/,
         fluid_sequencer_t * /*seq*/, void *data_pointer) {
        get_reference(static_cast<bool *>(data_pointer)) = true;
      },
      &finished);
  Q_ASSERT(finished_timer_id >= 0);

  initialize_play(song_widget);
  play_chords(song_widget, 0, static_cast<int>(song.chords.size()),
              START_END_MILLISECONDS);

  fluid_event_set_dest(&event, finished_timer_id);
  fluid_event_timer(&event, nullptr);
  send_event_at(sequencer, event, player.final_time + START_END_MILLISECONDS);

  auto &renderer = get_reference(new_fluid_file_renderer(&player.synth));
  while (!finished) {
    check_fluid_ok(fluid_file_renderer_process_block(&renderer));
  }
  delete_fluid_file_renderer(&renderer);

  fluid_event_set_dest(&event, player.sequencer_id);
  set_fluid_int(settings, "synth.lock-memory", 1);
  player.audio_driver_pointer =
      make_audio_driver(player.parent, player.settings, player.synth);
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

static void add_insert_row(SongWidget &song_widget, const int row_number) {
  auto &switch_column = song_widget.switch_column;
  const auto current_row_type = switch_column.current_row_type;
  QUndoCommand *undo_command = nullptr;
  const auto &chords = song_widget.song.chords;
  switch (current_row_type) {
  case chord_type:
    undo_command = new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
        switch_column.chords_table.model, row_number);
    break;
  case pitched_note_type:
    undo_command = make_insert_note(switch_column.pitched_notes_table.model,
                                    chords, row_number);
    break;
  case unpitched_note_type:
    undo_command = make_insert_note(switch_column.unpitched_notes_table.model,
                                    chords, row_number);
    break;
  default:
    Q_ASSERT(false);
  }
  song_widget.undo_stack.push(undo_command);
}

static void add_paste_insert(SongWidget &song_widget, const int row_number) {
  auto &switch_column = song_widget.switch_column;
  auto &chords_table = switch_column.chords_table;
  auto &pitched_notes_table = switch_column.pitched_notes_table;
  auto &unpitched_notes_table = switch_column.unpitched_notes_table;

  QUndoCommand *undo_command = nullptr;
  switch (switch_column.current_row_type) {
  case chord_type:
    undo_command =
        make_paste_insert_command(chords_table, chords_table.model, row_number);
    break;
  case pitched_note_type:
    undo_command = make_paste_insert_command(
        pitched_notes_table, pitched_notes_table.model, row_number);
    break;
  case unpitched_note_type:
    undo_command = make_paste_insert_command(
        unpitched_notes_table, unpitched_notes_table.model, row_number);
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

static void add_delete_cells(SongWidget &song_widget) {
  auto &undo_stack = song_widget.undo_stack;
  auto &switch_column = song_widget.switch_column;

  const auto &range = get_only_range(switch_column);

  QUndoCommand *undo_command = nullptr;
  switch (switch_column.current_row_type) {
  case chord_type:
    undo_command = new DeleteCells( // NOLINT(cppcoreguidelines-owning-memory)
        switch_column.chords_table.model, range);
    break;
  case pitched_note_type:
    undo_command = new DeleteCells( // NOLINT(cppcoreguidelines-owning-memory)
        switch_column.pitched_notes_table.model, range);
    break;
  case unpitched_note_type:
    undo_command = new DeleteCells( // NOLINT(cppcoreguidelines-owning-memory)
        switch_column.unpitched_notes_table.model, range);
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
  Q_ASSERT(filter.isValidUtf16());
  Q_ASSERT(suffix.isValidUtf16());
  auto &dialog = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QFileDialog(&song_widget, SongWidget::tr(caption),
                        song_widget.current_folder, filter));

  dialog.setAcceptMode(accept_mode);
  dialog.setDefaultSuffix(suffix);
  dialog.setFileMode(file_mode);

  return dialog;
}

[[nodiscard]] static auto can_discard_changes(SongWidget &song_widget) {
  return song_widget.undo_stack.isClean() ||
         QMessageBox::question(&song_widget, SongWidget::tr("Unsaved changes"),
                               SongWidget::tr("Discard unsaved changes?")) ==
             QMessageBox::Yes;
}

[[nodiscard]] static auto get_selected_file(SongWidget &song_widget,
                                            const QFileDialog &dialog) {
  song_widget.current_folder = dialog.directory().absolutePath();
  return get_only(dialog.selectedFiles());
}

void save_as_file(SongWidget &song_widget, const QString &filename) {
  Q_ASSERT(filename.isValidUtf16());
  const auto &song = song_widget.song;
  std::ofstream file_io(filename.toStdString().c_str());

  nlohmann::json json_song = nlohmann::json::object();
  json_song["gain"] = get_gain(song_widget);
  json_song["starting_key"] = song.starting_key;
  json_song["starting_tempo"] = song.starting_tempo;
  json_song["starting_velocity"] = song.starting_velocity;

  add_rows_to_json(json_song, "chords", song.chords);

  file_io << std::setw(4) << json_song;
  file_io.close();
  song_widget.current_file = filename;

  song_widget.undo_stack.setClean();
}

void open_file(SongWidget &song_widget, const QString &filename) {
  Q_ASSERT(filename.isValidUtf16());
  auto &undo_stack = song_widget.undo_stack;
  auto &spin_boxes = song_widget.controls_column.spin_boxes;
  auto &chords_model = song_widget.switch_column.chords_table.model;
  std::ifstream file_io(filename.toStdString().c_str());
  nlohmann::json json_song;
  try {
    json_song = nlohmann::json::parse(file_io);
  } catch (const nlohmann::json::parse_error &parse_error) {
    QMessageBox::warning(&song_widget, SongWidget::tr("Parsing error"),
                         parse_error.what());
    return;
  }
  file_io.close();

  static const auto song_validator = []() {
    nlohmann::json song_schema(
        {{"gain", get_number_schema("number", 0, MAX_GAIN)},
         {"starting_key", get_number_schema("number", 1, MAX_STARTING_KEY)},
         {"starting_tempo", get_number_schema("number", 1, MAX_STARTING_TEMPO)},
         {"starting_velocity", get_number_schema("number", 0, MAX_VELOCITY)}});
    song_schema["chords"] = get_array_schema(Chord::get_schema());
    return make_validator(nlohmann::json({
                              "gain",
                              "starting_key",
                              "starting_tempo",
                              "starting_velocity",
                          }),
                          std::move(song_schema));
  }();
  try {
    song_validator.validate(json_song);
  } catch (const std::exception &error) {
    QMessageBox::warning(&song_widget, SongWidget::tr("Schema error"),
                         error.what());
    return;
  }

  spin_boxes.gain_editor.setValue(get_json_double(json_song, "gain"));
  spin_boxes.starting_key_editor.setValue(
      get_json_double(json_song, "starting_key"));
  spin_boxes.starting_velocity_editor.setValue(
      get_json_double(json_song, "starting_velocity"));
  spin_boxes.starting_tempo_editor.setValue(
      get_json_double(json_song, "starting_tempo"));

  clear_rows(chords_model);

  if (json_song.contains("chords")) {
    chords_model.insert_json_rows(0, json_song["chords"]);
  }

  song_widget.current_file = filename;

  clear_and_clean(undo_stack);
}

auto get_chords_table(const SongWidget &song_widget) -> QAbstractItemView & {
  return song_widget.switch_column.chords_table;
}

auto get_pitched_notes_table(const SongWidget &song_widget)
    -> QAbstractItemView & {
  return song_widget.switch_column.pitched_notes_table;
}

auto get_unpitched_notes_table(const SongWidget &song_widget)
    -> QAbstractItemView & {
  return song_widget.switch_column.unpitched_notes_table;
}

auto get_unpitched_notes_model(SongWidget &song_widget)
    -> QAbstractItemModel & {
  return song_widget.switch_column.unpitched_notes_table.model;
}

auto get_starting_key(const SongWidget &song_widget) -> double {
  return song_widget.song.starting_key;
}

auto get_starting_velocity(const SongWidget &song_widget) -> double {
  return song_widget.song.starting_velocity;
}

auto get_starting_tempo(const SongWidget &song_widget) -> double {
  return song_widget.song.starting_tempo;
}

auto get_current_file(const SongWidget &song_widget) -> QString {
  return song_widget.current_file;
}

auto get_current_chord_number(const SongWidget &song_widget) -> int {
  return get_parent_chord_number(song_widget.switch_column);
}

void set_gain(const SongWidget &song_widget, double new_value) {
  song_widget.controls_column.spin_boxes.gain_editor.setValue(new_value);
}

void set_starting_key(const SongWidget &song_widget, double new_value) {
  song_widget.controls_column.spin_boxes.starting_key_editor.setValue(
      new_value);
}

void set_starting_velocity(const SongWidget &song_widget, double new_value) {
  song_widget.controls_column.spin_boxes.starting_velocity_editor.setValue(
      new_value);
}

void set_starting_tempo(const SongWidget &song_widget, double new_value) {
  song_widget.controls_column.spin_boxes.starting_tempo_editor.setValue(
      new_value);
}

void undo(SongWidget &song_widget) { song_widget.undo_stack.undo(); }

void trigger_third_down(SongWidget &song_widget) {
  song_widget.controls_column.third_row.minus_button.click();
};
void trigger_third_up(SongWidget &song_widget) {
  song_widget.controls_column.third_row.plus_button.click();
};
void trigger_fifth_down(SongWidget &song_widget) {
  song_widget.controls_column.fifth_row.minus_button.click();
};
void trigger_fifth_up(SongWidget &song_widget) {
  song_widget.controls_column.fifth_row.plus_button.click();
};
void trigger_seventh_down(SongWidget &song_widget) {
  song_widget.controls_column.seventh_row.minus_button.click();
};
void trigger_seventh_up(SongWidget &song_widget) {
  song_widget.controls_column.seventh_row.plus_button.click();
};
void trigger_octave_down(SongWidget &song_widget) {
  song_widget.controls_column.octave_row.minus_button.click();
};
void trigger_octave_up(SongWidget &song_widget) {
  song_widget.controls_column.octave_row.plus_button.click();
};

struct FileMenu : public QMenu {
  QAction save_action = QAction(FileMenu::tr("&Save"));
  QAction open_action = QAction(FileMenu::tr("&Open"));
  QAction save_as_action = QAction(FileMenu::tr("&Save As..."));
  QAction export_action = QAction(FileMenu::tr("&Export recording"));

  explicit FileMenu(SongWidget &song_widget) : QMenu(FileMenu::tr("&File")) {
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
          open_file(song_widget, get_selected_file(song_widget, dialog));
        }
      }
    });

    QObject::connect(&save_action, &QAction::triggered, this, [&song_widget]() {
      save_as_file(song_widget, song_widget.current_file);
    });

    QObject::connect(
        &save_as_action, &QAction::triggered, this, [&song_widget]() {
          auto &dialog = make_file_dialog(
              song_widget, "Save As — Justly", "JSON file (*.json)",
              QFileDialog::AcceptSave, ".json", QFileDialog::AnyFile);

          if (dialog.exec() != 0) {
            save_as_file(song_widget, get_selected_file(song_widget, dialog));
          }
        });

    QObject::connect(
        &export_action, &QAction::triggered, this, [&song_widget]() {
          auto &dialog = make_file_dialog(
              song_widget, "Export — Justly", "WAV file (*.wav)",
              QFileDialog::AcceptSave, ".wav", QFileDialog::AnyFile);
          dialog.setLabelText(QFileDialog::Accept, "Export");
          if (dialog.exec() != 0) {
            export_to_file(song_widget, get_selected_file(song_widget, dialog));
          }
        });
  }
};

struct PasteMenu : public QMenu {
  QAction paste_over_action = QAction(PasteMenu::tr("&Over"));
  QAction paste_into_action = QAction(PasteMenu::tr("&Into start"));
  QAction paste_after_action = QAction(PasteMenu::tr("&After"));

  explicit PasteMenu(SongWidget &song_widget) : QMenu(PasteMenu::tr("&Paste")) {
    add_menu_action(*this, paste_over_action, QKeySequence::Paste, false);
    add_menu_action(*this, paste_into_action);
    add_menu_action(*this, paste_after_action, QKeySequence::StandardKey(),
                    false);

    QObject::connect(
        &paste_over_action, &QAction::triggered, this, [&song_widget]() {
          auto &switch_column = song_widget.switch_column;
          auto &chords_table = switch_column.chords_table;
          auto &pitched_notes_table = switch_column.pitched_notes_table;
          auto &unpitched_notes_table = switch_column.unpitched_notes_table;

          const auto first_row_number = get_only_range(switch_column).top();

          QUndoCommand *undo_command = nullptr;
          switch (switch_column.current_row_type) {
          case chord_type:
            undo_command = make_paste_cells_command(
                chords_table, first_row_number, chords_table.model);
            break;
          case pitched_note_type:
            undo_command =
                make_paste_cells_command(pitched_notes_table, first_row_number,
                                         pitched_notes_table.model);
            break;
          case unpitched_note_type:
            undo_command = make_paste_cells_command(
                unpitched_notes_table, first_row_number,
                unpitched_notes_table.model);
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

    QObject::connect(&paste_into_action, &QAction::triggered, this,
                     [&song_widget]() { add_paste_insert(song_widget, 0); });

    QObject::connect(&paste_after_action, &QAction::triggered, this,
                     [&song_widget]() {
                       add_paste_insert(song_widget, get_next_row(song_widget));
                     });
  }
};

struct InsertMenu : public QMenu {
  QAction insert_after_action = QAction(InsertMenu::tr("&After"));
  QAction insert_into_action = QAction(InsertMenu::tr("&Into start"));

  explicit InsertMenu(SongWidget &song_widget)
      : QMenu(InsertMenu::tr("&Insert")) {
    add_menu_action(*this, insert_after_action,
                    QKeySequence::InsertLineSeparator, false);
    add_menu_action(*this, insert_into_action, QKeySequence::AddTab);

    QObject::connect(&insert_after_action, &QAction::triggered, this,
                     [&song_widget]() {
                       add_insert_row(song_widget, get_next_row(song_widget));
                     });

    QObject::connect(&insert_into_action, &QAction::triggered, this,
                     [&song_widget]() { add_insert_row(song_widget, 0); });
  }
};

struct ViewMenu : public QMenu {
  QAction back_to_chords_action = QAction(ViewMenu::tr("&Back to chords"));
  QAction previous_chord_action = QAction(ViewMenu::tr("&Previous chord"));
  QAction next_chord_action = QAction(ViewMenu::tr("&Next chord"));

  explicit ViewMenu() : QMenu(ViewMenu::tr("&View")) {
    add_menu_action(*this, back_to_chords_action, QKeySequence::Back, false);
    add_menu_action(*this, previous_chord_action, QKeySequence::PreviousChild,
                    false);
    add_menu_action(*this, next_chord_action, QKeySequence::NextChild, false);
  }
};

struct PlayMenu : public QMenu {
  QAction play_action = QAction(PlayMenu::tr("&Play selection"));
  QAction stop_playing_action = QAction(PlayMenu::tr("&Stop playing"));

  explicit PlayMenu(SongWidget &song_widget) : QMenu(PlayMenu::tr("&Play")) {
    add_menu_action(*this, play_action, QKeySequence::Print, false);
    add_menu_action(*this, stop_playing_action, QKeySequence::Cancel);

    const auto &player = song_widget.player;
    QObject::connect(&play_action, &QAction::triggered, this, [&song_widget]() {
      const auto &switch_column = song_widget.switch_column;
      const auto &song = song_widget.song;
      auto &player = song_widget.player;

      const auto current_row_type = switch_column.current_row_type;

      const auto &range = get_only_range(switch_column);
      const auto first_row_number = range.top();
      const auto number_of_rows = get_number_of_rows(range);

      stop_playing(player.sequencer, player.event);
      initialize_play(song_widget);

      if (current_row_type == chord_type) {
        modulate_before_chord(song_widget, first_row_number);
        play_chords(song_widget, first_row_number, number_of_rows);
      } else {
        const auto chord_number = get_parent_chord_number(switch_column);
        modulate_before_chord(song_widget, chord_number);
        const auto &chord = song.chords.at(chord_number);
        modulate(player, chord);
        if (current_row_type == pitched_note_type) {
          const auto pitched_result =
              play_notes(player, chord_number, chord.pitched_notes,
                         first_row_number, number_of_rows);
          if (!pitched_result) {
            return;
          }
        } else {
          const auto unpitched_result =
              play_notes(player, chord_number, chord.unpitched_notes,
                         first_row_number, number_of_rows);
          if (!unpitched_result) {
            return;
          }
        }
      }
    });

    QObject::connect(
        &stop_playing_action, &QAction::triggered, this,
        [&player]() { stop_playing(player.sequencer, player.event); });
  }
};

struct EditMenu : public QMenu {
  QAction cut_action = QAction(EditMenu::tr("&Cut"));
  QAction copy_action = QAction(EditMenu::tr("&Copy"));
  PasteMenu paste_menu;
  InsertMenu insert_menu;
  QAction delete_cells_action = QAction(EditMenu::tr("&Delete cells"));
  QAction remove_rows_action = QAction(EditMenu::tr("&Remove rows"));
  QAction rekey_action = QAction(EditMenu::tr("&Rekey mode"));

  explicit EditMenu(SongWidget &song_widget)
      : QMenu(EditMenu::tr("&Edit")), paste_menu(PasteMenu(song_widget)),
        insert_menu(InsertMenu(song_widget)) {
    auto &undo_stack = song_widget.undo_stack;
    auto &switch_column = song_widget.switch_column;
    auto &chords_model = song_widget.switch_column.chords_table.model;

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

    rekey_action.setCheckable(true);
    QObject::connect(
        &rekey_action, &QAction::toggled, &chords_model,
        [&chords_model](bool checked) { chords_model.rekey_mode = checked; });

    add_menu_action(*this, rekey_action);

    QObject::connect(&cut_action, &QAction::triggered, this, [&song_widget]() {
      copy_selection(song_widget.switch_column);
      add_delete_cells(song_widget);
    });

    QObject::connect(&copy_action, &QAction::triggered, &switch_column,
                     [&switch_column]() { copy_selection(switch_column); });

    QObject::connect(&delete_cells_action, &QAction::triggered, this,
                     [&song_widget]() { add_delete_cells(song_widget); });

    QObject::connect(
        &remove_rows_action, &QAction::triggered, this, [&song_widget]() {
          auto &switch_column = song_widget.switch_column;
          auto &undo_stack = song_widget.undo_stack;

          const auto &range = get_only_range(switch_column);
          const auto first_row_number = range.top();
          const auto number_of_rows = get_number_of_rows(range);

          QUndoCommand *undo_command = nullptr;
          switch (switch_column.current_row_type) {
          case chord_type:
            undo_command =
                make_remove_command(switch_column.chords_table.model,
                                    first_row_number, number_of_rows);
            break;
          case pitched_note_type:
            undo_command =
                make_remove_command(switch_column.pitched_notes_table.model,
                                    first_row_number, number_of_rows);
            break;
          case unpitched_note_type:
            undo_command =
                make_remove_command(switch_column.unpitched_notes_table.model,
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
  ViewMenu view_menu;
  PlayMenu play_menu;

  explicit SongMenuBar(SongWidget &song_widget)
      : file_menu(FileMenu(song_widget)), edit_menu(EditMenu(song_widget)),
        play_menu(PlayMenu(song_widget)) {
    addMenu(&file_menu);
    addMenu(&edit_menu);
    addMenu(&view_menu);
    addMenu(&play_menu);
  }
};

static void update_actions(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                           const QItemSelectionModel &selector) {
  auto &edit_menu = song_menu_bar.edit_menu;
  auto &controls_column = song_widget.controls_column;

  const auto anything_selected = !selector.selection().empty();
  const auto any_pitched_selected =
      anything_selected &&
      song_widget.switch_column.current_row_type != unpitched_note_type;

  set_rows_enabled(controls_column.third_row, controls_column.fifth_row,
                   controls_column.seventh_row, controls_column.octave_row,
                   any_pitched_selected);

  edit_menu.cut_action.setEnabled(anything_selected);
  edit_menu.copy_action.setEnabled(anything_selected);
  edit_menu.paste_menu.paste_over_action.setEnabled(anything_selected);
  edit_menu.paste_menu.paste_after_action.setEnabled(anything_selected);
  edit_menu.insert_menu.insert_after_action.setEnabled(anything_selected);
  edit_menu.delete_cells_action.setEnabled(anything_selected);
  edit_menu.remove_rows_action.setEnabled(anything_selected);
  song_menu_bar.play_menu.play_action.setEnabled(anything_selected);
}

static void set_model(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                      const RowType row_type) {
  auto &switch_column = song_widget.switch_column;
  switch_column.current_row_type = row_type;
  switch_column.chords_table.setVisible(row_type == chord_type);
  switch_column.pitched_notes_table.setVisible(row_type == pitched_note_type);
  switch_column.unpitched_notes_table.setVisible(row_type ==
                                                 unpitched_note_type);
  update_actions(song_menu_bar, song_widget,
                 get_selection_model(get_table(switch_column)));
}

void trigger_back_to_chords(SongMenuBar &song_menu_bar) {
  song_menu_bar.view_menu.back_to_chords_action.trigger();
}

void trigger_insert_after(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.insert_menu.insert_after_action.trigger();
}

void trigger_insert_into(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.insert_menu.insert_into_action.trigger();
}

void trigger_delete_cells(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.delete_cells_action.trigger();
}

void trigger_remove_rows(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.remove_rows_action.trigger();
}

void trigger_cut(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.cut_action.trigger();
}

void trigger_copy(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.copy_action.trigger();
}

void trigger_paste_over(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.paste_menu.paste_over_action.trigger();
}

void trigger_paste_into(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.paste_menu.paste_into_action.trigger();
}

void trigger_paste_after(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.paste_menu.paste_after_action.trigger();
}

void trigger_previous_chord(SongMenuBar &song_menu_bar) {
  song_menu_bar.view_menu.previous_chord_action.trigger();
}

void trigger_next_chord(SongMenuBar &song_menu_bar) {
  song_menu_bar.view_menu.next_chord_action.trigger();
}

void trigger_save(SongMenuBar &song_menu_bar) {
  song_menu_bar.file_menu.save_action.trigger();
}

void trigger_play(SongMenuBar &song_menu_bar) {
  song_menu_bar.play_menu.play_action.trigger();
}

void trigger_stop_playing(SongMenuBar &song_menu_bar) {
  song_menu_bar.play_menu.stop_playing_action.trigger();
}

void toggle_rekey_mode(SongMenuBar &song_menu_bar) {
  song_menu_bar.edit_menu.rekey_action.toggle();
}

static void replace_table(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                          const RowType new_row_type,
                          const int new_chord_number) {
  auto &switch_column = song_widget.switch_column;
  auto &view_menu = song_menu_bar.view_menu;

  auto &previous_chord_action = view_menu.previous_chord_action;
  auto &next_chord_action = view_menu.next_chord_action;

  auto &chords = song_widget.song.chords;
  auto to_chords = new_row_type == chord_type;

  const auto old_row_type = switch_column.current_row_type;
  const auto row_type_changed = old_row_type != new_row_type;

  QString label_text;
  QTextStream stream(&label_text);
  switch (new_row_type) {
  case chord_type:
    stream << SongMenuBar::tr("Chords");
    break;
  case pitched_note_type:
    stream << SongMenuBar::tr("Pitched notes for chord ")
           << new_chord_number + 1;
    break;
  case unpitched_note_type:
    stream << SongMenuBar::tr("Unpitched notes for chord ")
           << new_chord_number + 1;
  }
  switch_column.editing_text.setText(label_text);

  if (row_type_changed) {
    song_menu_bar.view_menu.back_to_chords_action.setEnabled(!to_chords);
    song_menu_bar.file_menu.open_action.setEnabled(to_chords);
    song_menu_bar.edit_menu.rekey_action.setEnabled(to_chords);
  }

  if (to_chords) {
    previous_chord_action.setEnabled(false);
    next_chord_action.setEnabled(false);

    auto &chords_table = switch_column.chords_table;
    const auto chord_index =
        chords_table.model.index(get_parent_chord_number(switch_column), 0);
    get_selection_model(chords_table)
        .select(chord_index, QItemSelectionModel::Select |
                                 QItemSelectionModel::Clear |
                                 QItemSelectionModel::Rows);
    chords_table.scrollTo(chord_index);

    set_model(song_menu_bar, song_widget, new_row_type);
    if (old_row_type == pitched_note_type) {
      switch_column.pitched_notes_table.model.set_rows_pointer();
    } else {
      switch_column.unpitched_notes_table.model.set_rows_pointer();
    }
  } else {
    auto &chord = chords[new_chord_number];
    previous_chord_action.setEnabled(new_chord_number > 0);
    next_chord_action.setEnabled(new_chord_number < chords.size() - 1);
    if (new_row_type == pitched_note_type) {
      switch_column.pitched_notes_table.model.set_rows_pointer(
          &chord.pitched_notes, new_chord_number);
    } else {
      switch_column.unpitched_notes_table.model.set_rows_pointer(
          &chord.unpitched_notes, new_chord_number);
    }
    set_model(song_menu_bar, song_widget, new_row_type);
  }
}

struct ReplaceTable : public QUndoCommand {
  SongMenuBar &song_menu_bar;
  SongWidget &song_widget;
  const RowType old_row_type;
  const int old_chord_number;
  RowType new_row_type;
  int new_chord_number;

  explicit ReplaceTable(SongMenuBar &song_menu_bar_input,
                        SongWidget &song_widget_input,
                        const RowType new_row_type_input,
                        const int new_chord_number_input)
      : song_menu_bar(song_menu_bar_input), song_widget(song_widget_input),
        old_row_type(song_widget.switch_column.current_row_type),
        old_chord_number(get_parent_chord_number(song_widget.switch_column)),
        new_row_type(new_row_type_input),
        new_chord_number(new_chord_number_input){};

  [[nodiscard]] auto id() const -> int override { return replace_table_id; }

  [[nodiscard]] auto
  mergeWith(const QUndoCommand *const next_command_pointer) -> bool override {
    Q_ASSERT(next_command_pointer != nullptr);
    const auto &next_command =
        get_reference(dynamic_cast<const ReplaceTable *>(next_command_pointer));
    const auto next_row_type = next_command.new_row_type;
    const auto next_chord_number = next_command.new_chord_number;
    if (old_row_type == next_row_type &&
        old_chord_number == next_chord_number) {
      setObsolete(true);
    }
    new_row_type = next_row_type;
    new_chord_number = next_chord_number;
    return true;
  }

  void undo() override {
    replace_table(song_menu_bar, song_widget, old_row_type, old_chord_number);
  }

  void redo() override {
    replace_table(song_menu_bar, song_widget, new_row_type, new_chord_number);
  }
};

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

SongEditor::SongEditor()
    : song_widget(*(new SongWidget)),
      song_menu_bar(*(new SongMenuBar(song_widget))) {
  auto &song_menu_bar = this->song_menu_bar;
  auto &song_widget = this->song_widget;

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

  QObject::connect(&song_menu_bar.view_menu.back_to_chords_action,
                   &QAction::triggered, this, [&song_menu_bar, &song_widget]() {
                     add_replace_table(song_menu_bar, song_widget, chord_type,
                                       -1);
                   });

  QObject::connect(
      &switch_column.chords_table, &QAbstractItemView::doubleClicked, this,
      [&song_menu_bar, &song_widget](const QModelIndex &index) {
        const auto column = index.column();
        const auto is_pitched = column == chord_pitched_notes_column;
        if (is_pitched || (column == chord_unpitched_notes_column)) {
          add_replace_table(
              song_menu_bar, song_widget,
              (is_pitched ? pitched_note_type : unpitched_note_type),
              index.row());
        }
      });

  QObject::connect(&song_menu_bar.view_menu.previous_chord_action,
                   &QAction::triggered, this, [&song_menu_bar, &song_widget]() {
                     const auto &switch_column = song_widget.switch_column;
                     add_replace_table(song_menu_bar, song_widget,
                                       switch_column.current_row_type,
                                       get_parent_chord_number(switch_column) -
                                           1);
                   });
  QObject::connect(&song_menu_bar.view_menu.next_chord_action,
                   &QAction::triggered, this, [&song_menu_bar, &song_widget]() {
                     const auto &switch_column = song_widget.switch_column;
                     add_replace_table(song_menu_bar, song_widget,
                                       switch_column.current_row_type,
                                       get_parent_chord_number(switch_column) +
                                           1);
                   });

  add_replace_table(song_menu_bar, song_widget, chord_type, -1);
  clear_and_clean(undo_stack);
}

void SongEditor::closeEvent(QCloseEvent *const close_event_pointer) {
  if (!can_discard_changes(song_widget)) {
    get_reference(close_event_pointer).ignore();
    return;
  }
  QMainWindow::closeEvent(close_event_pointer);
};

struct MusicXMLNote {
  int start_time = 0;
  int duration = 0;
  int midi_number = 0;
  QString words;
};

struct MusicXMLChord {
  int midi_key = 0;
  int divisions = 0;
  int measure_number = 0;
  QList<MusicXMLNote> pitched_notes;
  QList<MusicXMLNote> unpitched_notes;
};

struct PartInfo {
  QString part_name;
  QMap<QString, QString> instrument_map;
};

[[nodiscard]] static auto
parse_int(const tinyxml2::XMLElement &element) -> int {
  auto is_int = false;
  const int result = QString(element.GetText()).toInt(&is_int);
  Q_ASSERT(is_int);
  return result;
}

[[nodiscard]] static auto
get_duration(const tinyxml2::XMLElement &measure_element) -> int {
  const auto *duration_pointer = measure_element.FirstChildElement("duration");
  if (duration_pointer == nullptr) {
    return 0;
  }
  return parse_int(get_reference(duration_pointer));
}

[[nodiscard]] static auto get_interval(const int midi_interval) -> Interval {
  const auto [octave, degree] =
      get_quotient_remainder(midi_interval, HALFSTEPS_PER_OCTAVE);
  static const QList<Rational> scale = {
      Rational(1, 1), Rational(16, 15), Rational(9, 8),   Rational(6, 5),
      Rational(5, 4), Rational(4, 3),   Rational(45, 32), Rational(3, 2),
      Rational(8, 5), Rational(5, 3),   Rational(9, 5),   Rational(15, 8)};
  const auto &pitch = scale[degree];
  return {pitch.numerator, pitch.denominator, octave};
}

static void add_chord(ChordsModel &chords_model,
                      const MusicXMLChord &parse_chord, Interval interval,
                      Rational tempo_ratio, const int time_delta) {
  const auto midi_key = parse_chord.midi_key;
  const auto divisions = parse_chord.divisions;
  Chord new_chord;
  new_chord.beats = Rational(time_delta, divisions);
  new_chord.interval = std::move(interval);
  new_chord.words = QString::number(parse_chord.measure_number);
  new_chord.tempo_ratio = std::move(tempo_ratio);
  auto &unpitched_notes = new_chord.unpitched_notes;
  for (const auto &parse_unpitched_note : parse_chord.unpitched_notes) {
    UnpitchedNote new_note;
    new_note.beats = Rational(parse_unpitched_note.duration / divisions);
    new_note.words = parse_unpitched_note.words;
    unpitched_notes.push_back(std::move(new_note));
  }
  auto &pitched_notes = new_chord.pitched_notes;
  for (const auto &parse_pitched_note : parse_chord.pitched_notes) {
    PitchedNote new_note;
    new_note.beats = Rational(parse_pitched_note.duration / divisions);
    new_note.words = parse_pitched_note.words;
    new_note.interval = get_interval(parse_pitched_note.midi_number - midi_key);
    pitched_notes.push_back(std::move(new_note));
  }
  chords_model.insert_row(chords_model.rowCount(QModelIndex()),
                          std::move(new_chord));
}

static void add_note(MusicXMLChord &chord, MusicXMLNote note, bool is_pitched) {
  if (is_pitched) {
    chord.pitched_notes.push_back(std::move(note));
  } else {
    chord.unpitched_notes.push_back(std::move(note));
  }
}

static void add_note_and_maybe_chord(QMap<int, MusicXMLChord> &part_chord_dict,
                                     MusicXMLNote note, bool is_pitched) {
  const auto start_time = note.start_time;
  if (part_chord_dict.contains(start_time)) {
    add_note(part_chord_dict[start_time], std::move(note), is_pitched);
  } else {
    MusicXMLChord new_chord;
    add_note(part_chord_dict[start_time], std::move(note), is_pitched);
    part_chord_dict[start_time] = std::move(new_chord);
  }
}

template <typename NoteType>
[[nodiscard]] static auto get_max_duration(QList<NoteType> &notes) -> int {
  if (notes.empty()) {
    return 0;
  }
  return std::max_element(
             notes.begin(), notes.end(),
             [](const NoteType &first_note, const NoteType &second_note) {
               return first_note.duration < second_note.duration;
             })
      ->duration;
}

[[nodiscard]] static auto should_continue(const auto &dict,
                                          const auto &iterator,
                                          const int time) -> bool {
  return iterator != dict.end() && iterator.key() > time;
}

[[nodiscard]] static auto get_midi_number(const tinyxml2::XMLElement &pitch,
                                          const char *step_name,
                                          const char *octave_name) {
  static const QMap<QString, int> note_to_midi = {
      {"C", 0},  {"C#", 1}, {"Db", 1},  {"D", 2},   {"D#", 3}, {"Eb", 3},
      {"E", 4},  {"F", 5},  {"F#", 6},  {"Gb", 6},  {"G", 7},  {"G#", 8},
      {"Ab", 8}, {"A", 9},  {"A#", 10}, {"Bb", 10}, {"B", 11}};
  return note_to_midi[get_reference(pitch.FirstChildElement(step_name))
                          .GetText()] +
         parse_int(get_reference(pitch.FirstChildElement(octave_name))) *
             HALFSTEPS_PER_OCTAVE +
         C_0_MIDI;
}

static auto add_tied_note(PartInfo &part_info,
                          QMap<int, MusicXMLChord> &part_chord_dict,
                          QMap<int, MusicXMLNote> &tied_notes,
                          const tinyxml2::XMLElement &note_element,
                          int note_midi_number, int chord_start_time,
                          int note_duration, bool tie_start, bool tie_end,
                          bool is_pitched) {
  if (tie_end) {
    const auto tied_notes_iterator = tied_notes.find(note_midi_number);
    Q_ASSERT(tied_notes_iterator != tied_notes.end());
    auto &previous_note = tied_notes_iterator.value();
    previous_note.duration = previous_note.duration + note_duration;
    if (!tie_start) {
      add_note_and_maybe_chord(part_chord_dict, previous_note, is_pitched);
      tied_notes.erase(tied_notes_iterator);
    }
  } else {
    MusicXMLNote new_note;
    new_note.duration = note_duration;
    QTextStream stream(&new_note.words);
    stream << QObject::tr("Part ") << part_info.part_name;
    const auto *instrument_pointer =
        note_element.FirstChildElement("instrument");
    if (instrument_pointer != nullptr) {
      stream << QObject::tr(" instrument ")
             << part_info.instrument_map[get_reference(instrument_pointer)
                                             .Attribute("id")];
    }
    new_note.midi_number = note_midi_number;
    new_note.start_time = chord_start_time;
    if (tie_start) { // also not tie end
      tied_notes[note_midi_number] = std::move(new_note);
    } else { // not tie start or end
      add_note_and_maybe_chord(part_chord_dict, std::move(new_note),
                               is_pitched);
    }
  }
}
// TODO(brandon): validate musicxml
// TODO(brandon): transposing instruments
static void parse_file(SongWidget &song_widget, const QString &filename) {
  auto &undo_stack = song_widget.undo_stack;
  auto &spin_boxes = song_widget.controls_column.spin_boxes;
  auto &chords_model = song_widget.switch_column.chords_table.model;

  tinyxml2::XMLDocument document;
  const auto result = document.LoadFile(filename.toStdString().c_str());
  Q_ASSERT(result == tinyxml2::XML_SUCCESS);

  clear_rows(chords_model);

  // Get root measure_element
  const auto &root = get_reference(document.RootElement());
  if (QString(root.Name()) != "score-partwise") {
    QMessageBox::warning(
        &song_widget, QObject::tr("Partwise error"),
        QObject::tr("Justly only supports partwise musicxml scores"));
  };
  const auto &part_list = get_reference(root.FirstChildElement("part-list"));

  QMap<QString, PartInfo> part_info_dict;
  const auto *score_part_pointer = part_list.FirstChildElement("score-part");
  while (score_part_pointer != nullptr) {
    const auto &score_part = get_reference(score_part_pointer);

    PartInfo part_info;
    part_info.part_name =
        get_reference(score_part.FirstChildElement("part-name")).GetText();
    auto &instrument_map = part_info.instrument_map;
    const auto *score_instrument_pointer =
        score_part.FirstChildElement("score-instrument");
    while (score_instrument_pointer != nullptr) {
      const auto &score_instrument = get_reference(score_instrument_pointer);
      instrument_map[score_instrument.Attribute("id")] =
          get_reference(score_instrument.FirstChildElement("instrument-name"))
              .GetText();
      score_instrument_pointer =
          score_instrument.NextSiblingElement("score-instrument");
    }
    part_info_dict[score_part.Attribute("id")] = std::move(part_info);
    score_part_pointer = score_part.NextSiblingElement("score-part");
  }

  QMap<int, MusicXMLChord> chords_dict;
  QMap<int, MusicXMLNote> tied_notes;
  QMap<int, MusicXMLChord> part_chord_dict;
  QMap<int, int> keys_dict;
  QMap<int, int> divisions_dict;
  QMap<int, int> measure_number_dict;

  const auto *part_pointer = root.FirstChildElement("part");
  while (part_pointer != nullptr) {
    auto current_time = 0;
    auto chord_start_time = current_time;
    auto measure_number = 1;

    const auto &part = get_reference(part_pointer);
    const QString part_id = part.Attribute("id");
    auto &part_info = part_info_dict[part_id];

    const auto *measure_pointer = part.FirstChildElement("measure");

    while (measure_pointer != nullptr) {
      const auto &measure = get_reference(measure_pointer);
      measure_number_dict[current_time] = measure_number;
      const auto *measure_element_pointer = measure.FirstChildElement();
      while (measure_element_pointer != nullptr) {
        const auto &measure_element = get_reference(measure_element_pointer);
        const std::string measure_element_name = measure_element.Name();
        if (measure_element_name == "attributes") {
          const auto *key_pointer = measure_element.FirstChildElement("key");
          if (key_pointer != nullptr) {
            const auto [quotient, remainder] = get_quotient_remainder(
                FIFTH_HALFSTEPS *
                    parse_int(get_reference(get_reference(key_pointer)
                                                .FirstChildElement("fifths"))),
                HALFSTEPS_PER_OCTAVE);
            keys_dict[current_time] = MIDDLE_C_MIDI + remainder;
          }
          const auto *divisions_pointer =
              measure_element.FirstChildElement("divisions");
          if (divisions_pointer != nullptr) {
            divisions_dict[current_time] =
                parse_int(get_reference(divisions_pointer));
          }
        } else if (measure_element_name == "note") {
          const auto note_duration = get_duration(measure_element);
          if (note_duration == 0) {
            // TODO(brandon): support or better error
            QMessageBox::warning(
                &song_widget, QObject::tr("Note duration error"),
                QObject::tr("Zero duration notes not supported"));
          } else {
            const auto *pitch_pointer =
                measure_element.FirstChildElement("pitch");
            const auto *unpitched_pointer =
                measure_element.FirstChildElement("unpitched");
            if (measure_element_pointer->FirstChildElement("chord") ==
                nullptr) {
              chord_start_time = current_time;
              current_time = current_time + note_duration;
            }

            bool tie_start = false;
            bool tie_end = false;

            const auto *tie_pointer = measure_element.FirstChildElement("tie");
            while (tie_pointer != nullptr) {
              const auto &tie_element = get_reference(tie_pointer);
              const std::string tie_type = tie_element.Attribute("type");
              if (tie_type == "stop") {
                tie_end = true;
              } else if (tie_type == "start") {
                tie_start = true;
              };
              tie_pointer = tie_element.NextSiblingElement("tie");
            }

            if (unpitched_pointer != nullptr) {
              const auto &unpitched = get_reference(unpitched_pointer);
              add_tied_note(
                  part_info, part_chord_dict, tied_notes, measure_element,
                  get_midi_number(unpitched, "display-step", "display-octave"),
                  chord_start_time, note_duration, tie_start, tie_end, false);
            } else if (pitch_pointer != nullptr) {
              const auto &pitch = get_reference(pitch_pointer);

              const auto *alteration_pointer = pitch.FirstChildElement("alter");
              auto alteration = 0;
              if (alteration_pointer != nullptr) {
                alteration = parse_int(get_reference(alteration_pointer));
              }
              add_tied_note(
                  part_info, part_chord_dict, tied_notes, measure_element,
                  get_midi_number(pitch, "step", "octave") + alteration,
                  chord_start_time, note_duration, tie_start, tie_end, true);
            }
          }
        } else if (measure_element_name == "backup") {
          current_time = current_time - get_duration(measure_element);
          chord_start_time = current_time;
        } else if (measure_element_name == "forward") {
          current_time = current_time + get_duration(measure_element);
          chord_start_time = current_time;
        }
        measure_element_pointer = measure_element.NextSiblingElement();
      }
      measure_number = measure_number + 1;
      measure_pointer = measure.NextSiblingElement("measure");
    }
    Q_ASSERT(tied_notes.empty());

    auto measure_number_iterator = measure_number_dict.begin();
    auto keys_iterator = keys_dict.begin();
    auto divisions_iterator = divisions_dict.begin();

    int last_measure_number = -1;
    int last_key = 0;
    int last_divisions = 1;

    auto part_chord_iterator = part_chord_dict.begin();
    const auto part_chord_dict_end = part_chord_dict.end();
    while (part_chord_iterator != part_chord_dict_end) {
      const auto chord_time = part_chord_iterator.key();

      while (should_continue(measure_number_dict, measure_number_iterator,
                             chord_time)) {
        last_measure_number = measure_number_iterator.value();
        ++measure_number_iterator;
      }

      while (should_continue(keys_dict, keys_iterator, chord_time)) {
        last_key = keys_iterator.value();
        ++keys_iterator;
      }

      while (should_continue(divisions_dict, divisions_iterator, chord_time)) {
        last_divisions = divisions_iterator.value();
        ++divisions_iterator;
      }

      auto &part_chord = part_chord_iterator.value();
      part_chord.measure_number = last_measure_number;
      part_chord.midi_key = last_key;
      part_chord.divisions = last_divisions;

      auto maybe_old_chord_iterator = chords_dict.find(chord_time);
      if (maybe_old_chord_iterator == chords_dict.end()) {
        chords_dict[chord_time] = std::move(part_chord);
      } else {
        auto &old_chord = maybe_old_chord_iterator.value();
        Q_ASSERT(old_chord.measure_number == last_measure_number);
        Q_ASSERT(old_chord.midi_key == last_key);
        Q_ASSERT(old_chord.divisions == last_divisions);
        old_chord.pitched_notes.append(std::move(part_chord.pitched_notes));
        old_chord.pitched_notes.append(std::move(part_chord.pitched_notes));
      }

      ++part_chord_iterator;
    }

    part_chord_dict.clear();
    keys_dict.clear();
    divisions_dict.clear();
    measure_number_dict.clear();

    part_pointer = part.NextSiblingElement("part");
  }

  auto chord_iterator = chords_dict.begin();
  const auto chord_dict_end = chords_dict.end();
  if (chord_iterator == chord_dict_end) {
    // TODO(brandon): support or better error
    QMessageBox::warning(&song_widget, QObject::tr("Empty MusicXML error"),
                         QObject::tr("No chords"));
  }

  int last_time = chord_iterator.key();
  auto last_parse_chord = std::move(chord_iterator.value());
  auto last_midi_key = last_parse_chord.midi_key;
  auto last_divisions = last_parse_chord.divisions;
  auto last_interval = get_interval(0);
  auto last_tempo_ratio = Rational(1, 1);

  spin_boxes.starting_key_editor.setValue(get_frequency(last_midi_key));

  ++chord_iterator;
  while (chord_iterator != chord_dict_end) {
    const int time = chord_iterator.key();
    add_chord(chords_model, last_parse_chord, std::move(last_interval),
              std::move(last_tempo_ratio), time - last_time);
    last_parse_chord = std::move(chord_iterator.value());
    const auto current_midi_key = last_parse_chord.midi_key;
    const auto divisions = last_parse_chord.divisions;
    last_interval = get_interval(current_midi_key - last_midi_key);
    last_tempo_ratio = Rational(last_divisions, divisions);
    last_time = time;
    last_midi_key = current_midi_key;
    last_divisions = divisions;
    ++chord_iterator;
  }
  add_chord(chords_model, last_parse_chord, std::move(last_interval),
            std::move(last_tempo_ratio),
            std::max(get_max_duration(last_parse_chord.pitched_notes),
                     get_max_duration(last_parse_chord.unpitched_notes)));

  clear_and_clean(undo_stack);
}

// TODO(brandon): add tests for button rekey
// TODO(brandon): add docs for buttons
// TODO(brandon): update docs for percussion instruments
// https://www.voidaudio.net/percussion.html
// TODO(brandon): test for more crash bugs

#include "justly.moc"
#include <QAbstractItemDelegate>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QByteArray>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGuiApplication>
#include <QHeaderView>
#include <QIcon>
#include <QItemEditorFactory>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
#include <QMetaType>
#include <QMimeData>
#include <QObject>
#include <QRect>
#include <QScreen>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QString>
#include <QStringListModel>
#include <QStyleOption>
#include <QTableView>
#include <QTextStream>
#include <QUndoStack>
#include <QVariant>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
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
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "justly/justly.hpp"

struct Player;

enum ControlId {
  gain_id,
  starting_key_id,
  starting_velocity_id,
  starting_tempo_id
};

enum ModelType { chords_type, pitched_notes_type, unpitched_notes_type };

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
static const auto MAX_RATIONAL_DENOMINATOR = 199;
static const auto MAX_RATIONAL_NUMERATOR = 199;
static const auto MAX_STARTING_KEY = 440;
static const auto MAX_STARTING_TEMPO = 200;
static const auto MAX_VELOCITY = 127;
static const auto MILLISECONDS_PER_SECOND = 1000;
static const auto MIN_STARTING_KEY = 60;
static const auto MIN_STARTING_TEMPO = 25;
static const auto NUMBER_OF_MIDI_CHANNELS = 16;
static const auto OCTAVE_RATIO = 2.0;
static const auto SECONDS_PER_MINUTE = 60;
static const auto START_END_MILLISECONDS = 500;
static const auto VERBOSE_FLUIDSYNTH = false;
static const auto ZERO_BEND_HALFSTEPS = 2;

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

[[nodiscard]] static auto to_int(double value) {
  return static_cast<int>(std::round(value));
};

[[nodiscard]] static auto get_number_schema(const char *type, int minimum,
                                            int maximum) {
  return nlohmann::json(
      {{"type", type}, {"minimum", minimum}, {"maximum", maximum}});
};

// a subnamed should have the following methods:
// static auto SubNamed::get_all_nameds() -> const QList<SubNamed>&;
// static auto get_field_name() -> const char*;
struct Named {
  QString name;
  explicit Named(const char *name_input) : name(name_input){};
};

template <std::derived_from<Named> SubNamed>
[[nodiscard]] static auto get_by_name(const QString &name) -> const SubNamed & {
  const auto &all_nameds = SubNamed::get_all_nameds();
  const auto named_pointer =
      std::find_if(all_nameds.cbegin(), all_nameds.cend(),
                   [name](const SubNamed &item) { return item.name == name; });
  Q_ASSERT(named_pointer != nullptr);
  return *named_pointer;
}

struct PercussionInstrument : public Named {
  short midi_number;
  PercussionInstrument(const char *name, short midi_number_input)
      : Named(name), midi_number(midi_number_input){};
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
  };

  [[nodiscard]] static auto get_field_name() {
    return "percussion_instrument";
  };
};

Q_DECLARE_METATYPE(const PercussionInstrument *);

[[nodiscard]] auto get_soundfont_id(fluid_synth_t *synth_pointer) {
  auto soundfont_file = QDir(QCoreApplication::applicationDirPath())
                            .filePath("../share/MuseScore_General.sf2")
                            .toStdString();
  Q_ASSERT(std::filesystem::exists(soundfont_file));

  auto soundfont_id =
      fluid_synth_sfload(synth_pointer, soundfont_file.c_str(), 1);
  Q_ASSERT(soundfont_id >= 0);
  return soundfont_id;
};

struct Program : public Named {
  short bank_number;
  short preset_number;
  Program(const char *name, short bank_number_input, short preset_number_input)
      : Named(name), bank_number(bank_number_input),
        preset_number(preset_number_input){};
};

template <std::derived_from<Program> SubProgram>
[[nodiscard]] static auto get_programs(bool is_percussion) {
  QList<SubProgram> programs;
  auto *settings_pointer = new_fluid_settings();
  auto *synth_pointer = new_fluid_synth(settings_pointer);

  fluid_sfont_t *soundfont_pointer = fluid_synth_get_sfont_by_id(
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

  while (preset_pointer != nullptr) {
    const auto *name = fluid_preset_get_name(preset_pointer);
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
  PercussionSet(const char *name, short bank_number, short preset_number)
      : Program(name, bank_number, preset_number){};
  [[nodiscard]] static auto get_all_nameds() -> const QList<PercussionSet> & {
    static const auto all_percussion_sets = get_programs<PercussionSet>(true);
    return all_percussion_sets;
  };
  [[nodiscard]] static auto get_field_name() { return "percussion_set"; };
};

Q_DECLARE_METATYPE(const PercussionSet *);

struct Instrument : public Program {
  Instrument(const char *name, short bank_number, short preset_number)
      : Program(name, bank_number, preset_number){};
  [[nodiscard]] static auto get_all_nameds() -> const QList<Instrument> & {
    static const auto all_instruments = get_programs<Instrument>(false);
    return all_instruments;
  };

  [[nodiscard]] static auto get_field_name() { return "instrument"; };
};

Q_DECLARE_METATYPE(const Instrument *);

static void add_int_to_json(nlohmann::json &json_object, const char *field_name,
                            int value, int default_value) {
  if (value != default_value) {
    json_object[field_name] = value;
  }
};

// a sub rational should have the following method:
// SubRational(const nlohmann::json &json_rational);
struct AbstractRational {
  int numerator = 1;
  int denominator = 1;

  AbstractRational() = default;
  AbstractRational(int numerator_input, int denominator_input)
      : numerator(numerator_input), denominator(denominator_input){};
  explicit AbstractRational(const nlohmann::json &json_rational)
      : numerator(json_rational.value("numerator", 1)),
        denominator(json_rational.value("denominator", 1)){};
  virtual ~AbstractRational() = default;

  [[nodiscard]] auto operator==(const AbstractRational &other_rational) const {
    return numerator == other_rational.numerator &&
           denominator == other_rational.denominator;
  };

  [[nodiscard]] virtual auto is_default() const -> bool {
    return numerator == 1 && denominator == 1;
  };
  [[nodiscard]] virtual auto to_double() const -> double {
    Q_ASSERT(denominator != 0);
    return 1.0 * numerator / denominator;
  };
  virtual void to_json(nlohmann::json &json_rational) const {
    add_int_to_json(json_rational, "numerator", numerator, 1);
    add_int_to_json(json_rational, "denominator", denominator, 1);
  };
};

template <std::derived_from<AbstractRational> SubRational>
[[nodiscard]] static auto
json_field_to_abstract_rational(const nlohmann::json &json_row,
                                const char *field_name) {
  if (json_row.contains(field_name)) {
    return SubRational(json_row[field_name]);
  }
  return SubRational();
}

struct Rational : public AbstractRational {
  Rational() = default;
  Rational(int numerator, int denominator)
      : AbstractRational(numerator, denominator){};
  explicit Rational(const nlohmann::json &json_rational)
      : AbstractRational(json_rational){};
  [[nodiscard]] auto operator==(const Rational &other_rational) const {
    return AbstractRational::operator==(other_rational);
  };
};

Q_DECLARE_METATYPE(Rational);

struct Interval : public AbstractRational {
  int octave = 0;

  Interval() = default;
  Interval(int numerator, int denominator, int octave_input)
      : AbstractRational(numerator, denominator), octave(octave_input){};
  explicit Interval(const nlohmann::json &json_rational)
      : AbstractRational(json_rational),
        octave(json_rational.value("octave", 0)){};

  [[nodiscard]] auto operator==(const Interval &other_interval) const {
    return AbstractRational::operator==(other_interval) &&
           octave == other_interval.octave;
  };
  [[nodiscard]] auto is_default() const -> bool override {
    return AbstractRational::is_default() && octave == 0;
  };
  [[nodiscard]] auto to_double() const -> double override {
    return AbstractRational::to_double() * pow(OCTAVE_RATIO, octave);
  };
  void to_json(nlohmann::json &json_interval) const override {
    AbstractRational::to_json(json_interval);
    add_int_to_json(json_interval, "octave", octave, 0);
  };
};

Q_DECLARE_METATYPE(Interval);

static void add_abstract_rational_to_json(nlohmann::json &json_row,
                                          const AbstractRational &rational,
                                          const char *column_name) {
  if (!rational.is_default()) {
    nlohmann::json json_rational;
    rational.to_json(json_rational);
    json_row[column_name] = std::move(json_rational);
  }
}

static void add_words_to_json(nlohmann::json &json_row, const QString &words) {
  if (!words.isEmpty()) {
    json_row["words"] = words.toStdString().c_str();
  }
};

[[nodiscard]] static auto get_rational_fields_schema() {
  return nlohmann::json(
      {{"numerator", get_number_schema("integer", 1, MAX_RATIONAL_NUMERATOR)},
       {"denominator",
        get_number_schema("integer", 1, MAX_RATIONAL_DENOMINATOR)}});
};

[[nodiscard]] static auto
get_object_schema(const nlohmann::json &properties_json) {
  return nlohmann::json({{"type", "object"}, {"properties", properties_json}});
};

// In addition to the following, a sub-row should have the following methods:
// SubRow(const nlohmann::json& json_row);
// static auto is_column_editable(int column_number) -> bool;
// (optional)
// static auto get_column_name(int column_number) -> QString;
// static auto get_number_of_columns() -> int;
// static auto get_fields_schema() -> nlohmann::json;
// static auto get_plural_field_for() -> const char *;
// void copy_columns_from(const SubRow &template_row, int left_column,
//                        int right_column);
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
          if (json_row.contains("words")) {
            return QString::fromStdString(json_row["words"]);
          }
          return QString("");
        }(json_chord)){};

  virtual ~Row() = default;
  [[nodiscard]] static auto is_column_editable(int /*column_number*/) {
    return true;
  };
  [[nodiscard]] virtual auto get_data(int column_number) const -> QVariant = 0;
  virtual void set_data(int column, const QVariant &new_value) = 0;
  [[nodiscard]] virtual auto
  columns_to_json(int left_column,
                  int right_column) const -> nlohmann::json = 0;
  [[nodiscard]] virtual auto to_json() const -> nlohmann::json {
    auto json_row = nlohmann::json::object();
    add_abstract_rational_to_json(json_row, beats, "beats");
    add_abstract_rational_to_json(json_row, velocity_ratio, "velocity_ratio");
    add_words_to_json(json_row, words);
    return json_row;
  };
  [[nodiscard]] static auto get_fields_schema() {
    return nlohmann::json(
        {{"beats", get_object_schema(get_rational_fields_schema())},
         {"velocity_ratio", get_object_schema(get_rational_fields_schema())},
         {"words", nlohmann::json({{"type", "string"}})}});
  };
};

template <std::derived_from<Row> SubRow> struct RowsModel;

template <std::derived_from<Row> SubRow>
static void partial_json_to_rows(QList<SubRow> &new_rows,
                                 const nlohmann::json &json_rows,
                                 int number_of_rows) {
  std::transform(
      json_rows.cbegin(), json_rows.cbegin() + static_cast<int>(number_of_rows),
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
  if (!rows.empty()) {
    nlohmann::json json_rows;
    std::transform(
        rows.cbegin(), rows.cend(), std::back_inserter(json_rows),
        [](const SubRow &row) -> nlohmann::json { return row.to_json(); });
    json_chord[SubRow::get_plural_field_for()] = std::move(json_rows);
  }
}

template <std::derived_from<Named> SubNamed>
static void add_named_to_json(nlohmann::json &json_row,
                              const SubNamed *named_pointer) {
  if (named_pointer != nullptr) {
    json_row[SubNamed::get_field_name()] = named_pointer->name.toStdString();
  }
}

template <std::derived_from<Named> SubNamed>
[[nodiscard]] static auto json_field_to_named_pointer(
    const nlohmann::json &json_row) -> const SubNamed * {
  const char *field_name = SubNamed::get_field_name();
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
  std::vector<std::string> names;
  const auto &all_nameds = SubNamed::get_all_nameds();
  std::transform(all_nameds.cbegin(), all_nameds.cend(),
                 std::back_inserter(names),
                 [](const SubNamed &item) { return item.name.toStdString(); });
  json_row[SubNamed::get_field_name()] =
      nlohmann::json({{"type", "string"}, {"enum", std::move(names)}});
};

static void add_pitched_fields_to_schema(nlohmann::json &schema) {
  add_named_schema<Instrument>(schema);
  auto interval_fields_schema = get_rational_fields_schema();
  interval_fields_schema["octave"] =
      get_number_schema("integer", -MAX_OCTAVE, MAX_OCTAVE);
  schema["interval"] = get_object_schema(interval_fields_schema);
};

static void add_unpitched_fields_to_schema(nlohmann::json &schema) {
  add_named_schema<PercussionSet>(schema);
  add_named_schema<PercussionInstrument>(schema);
};

template <std::derived_from<Row> SubRow>
static void add_row_array_schema(nlohmann::json &schema) {
  schema[SubRow::get_plural_field_for()] = nlohmann::json(
      {{"type", "array"},
       {"items", get_object_schema(SubRow::get_fields_schema())}});
}

template <typename SubType>
[[nodiscard]] static auto variant_to(const QVariant &variant) {
  Q_ASSERT(variant.canConvert<SubType>());
  return variant.value<SubType>();
}

// A subnote should have the following method:
// static auto get_note_type() -> const char*;
struct Note : Row {
  Note() = default;
  explicit Note(const nlohmann::json &json_note) : Row(json_note){};

  [[nodiscard]] virtual auto
  get_closest_midi(Player &player, int channel_number, int chord_number,
                   int note_number) const -> short = 0;

  [[nodiscard]] virtual auto
  get_program(const Player &player, int chord_number,
              int note_number) const -> const Program & = 0;
};

template <std::derived_from<Note> SubNote>
static void add_note_location(QTextStream &stream, int chord_number,
                              int note_number) {
  stream << QObject::tr(" for chord ") << chord_number + 1
         << QObject::tr(SubNote::get_note_type()) << note_number + 1;
};

template <std::derived_from<Note> SubNote, std::derived_from<Named> SubNamed>
[[nodiscard]] static auto
substitute_named_for(QWidget &parent, const SubNamed *sub_named_pointer,
                     const SubNamed *current_sub_named_pointer,
                     const char *default_one, int chord_number, int note_number,
                     const char *missing_title, const char *missing_message,
                     const char *default_message) -> const SubNamed & {
  if (sub_named_pointer == nullptr) {
    sub_named_pointer = current_sub_named_pointer;
  };
  if (sub_named_pointer == nullptr) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr(missing_message);
    add_note_location<SubNote>(stream, chord_number, note_number);
    stream << QObject::tr(default_message);
    QMessageBox::warning(&parent, QObject::tr(missing_title), message);
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
            json_field_to_named_pointer<PercussionInstrument>(json_note)){};

  [[nodiscard]] auto get_closest_midi(Player &player, int channel_number,
                                      int chord_number,
                                      int note_number) const -> short override;

  [[nodiscard]] auto
  get_program(const Player &player, int chord_number,
              int note_number) const -> const Program & override;

  [[nodiscard]] static auto get_fields_schema() {
    auto schema = Row::get_fields_schema();
    add_unpitched_fields_to_schema(schema);
    return schema;
  };
  [[nodiscard]] static auto get_note_type() { return ", unpitched note "; };

  [[nodiscard]] static auto get_plural_field_for() {
    return "unpitched_notes";
  };

  [[nodiscard]] static auto get_column_name(int column_number) {
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
  };
  [[nodiscard]] static auto get_number_of_columns() {
    return number_of_unpitched_note_columns;
  };

  [[nodiscard]] auto get_data(int column_number) const -> QVariant override {
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
  };
  void set_data(int column, const QVariant &new_value) override {
    switch (column) {
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
  };

  void copy_columns_from(const UnpitchedNote &template_row, int left_column,
                         int right_column) {
    for (auto percussion_column = left_column;
         percussion_column <= right_column; percussion_column++) {
      switch (percussion_column) {
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
  };

  [[nodiscard]] auto to_json() const -> nlohmann::json override {
    auto json_percussion = Row::to_json();
    add_named_to_json(json_percussion, percussion_set_pointer);
    add_named_to_json(json_percussion, percussion_instrument_pointer);
    return json_percussion;
  };
  [[nodiscard]] auto columns_to_json(int left_column, int right_column) const
      -> nlohmann::json override {
    auto json_percussion = nlohmann::json::object();

    for (auto percussion_column = left_column;
         percussion_column <= right_column; percussion_column++) {
      switch (percussion_column) {
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
    return json_percussion;
  };
};

struct PitchedNote : Note {
  const Instrument *instrument_pointer = nullptr;
  Interval interval;

  PitchedNote() = default;
  explicit PitchedNote(const nlohmann::json &json_note)
      : Note(json_note),
        instrument_pointer(json_field_to_named_pointer<Instrument>(json_note)),
        interval(
            json_field_to_abstract_rational<Interval>(json_note, "interval")){};

  [[nodiscard]] auto get_closest_midi(Player &player, int channel_number,
                                      int chord_number,
                                      int note_number) const -> short override;

  [[nodiscard]] auto
  get_program(const Player &player, int chord_number,
              int note_number) const -> const Program & override;

  [[nodiscard]] static auto get_fields_schema() {
    auto schema = Row::get_fields_schema();
    add_pitched_fields_to_schema(schema);
    return schema;
  };
  [[nodiscard]] static auto get_plural_field_for() { return "pitched_notes"; };
  [[nodiscard]] static auto get_note_type() { return ", pitched note "; };
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
    }
  };
  [[nodiscard]] static auto get_number_of_columns() {
    return number_of_pitched_note_columns;
  };

  [[nodiscard]] auto get_data(int column_number) const -> QVariant override {
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
  };
  void set_data(int column, const QVariant &new_value) override {
    switch (column) {
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
  };
  ;

  void copy_columns_from(const PitchedNote &template_row, int left_column,
                         int right_column) {
    for (auto note_column = left_column; note_column <= right_column;
         note_column++) {
      switch (note_column) {
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
  };

  [[nodiscard]] auto to_json() const -> nlohmann::json override {
    auto json_note = Row::to_json();
    add_named_to_json(json_note, instrument_pointer);
    add_abstract_rational_to_json(json_note, interval, "interval");
    return json_note;
  };
  [[nodiscard]] auto columns_to_json(int left_column, int right_column) const
      -> nlohmann::json override {
    auto json_note = nlohmann::json::object();
    for (auto note_column = left_column; note_column <= right_column;
         note_column++) {
      switch (note_column) {
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
    return json_note;
  };
};

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto json_field_to_rows(nlohmann::json json_object) {
  const char *field = SubRow::get_plural_field_for();
  if (json_object.contains(field)) {
    QList<SubRow> rows;
    const auto &json_rows = json_object[field];
    json_to_rows(rows, json_rows);
    return rows;
  }
  return QList<SubRow>();
}

static void set_fluid_int(fluid_settings_t *settings_pointer, const char *field,
                          int value) {
  auto result = fluid_settings_setint(settings_pointer, field, value);
  Q_ASSERT(result == FLUID_OK);
}

[[nodiscard]] static auto get_settings_pointer() {
  fluid_settings_t *settings_pointer = new_fluid_settings();
  Q_ASSERT(settings_pointer != nullptr);
  auto cores = std::thread::hardware_concurrency();
  if (cores > 0) {
    set_fluid_int(settings_pointer, "synth.cpu-cores", static_cast<int>(cores));
  }
#ifdef __linux__
  fluid_settings_setstr(settings_pointer, "audio.driver", "pulseaudio");
#endif
  return settings_pointer;
}

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
        unpitched_notes(json_field_to_rows<UnpitchedNote>(json_chord)){};

  [[nodiscard]] static auto get_fields_schema() {
    auto schema = Row::get_fields_schema();
    add_pitched_fields_to_schema(schema);
    add_unpitched_fields_to_schema(schema);
    schema["tempo_ratio"] = get_object_schema(get_rational_fields_schema());
    add_row_array_schema<PitchedNote>(schema);
    add_row_array_schema<UnpitchedNote>(schema);
    return schema;
  };

  [[nodiscard]] static auto get_plural_field_for() { return "chords"; };
  [[nodiscard]] static auto get_column_name(int column_number) {
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
  };
  [[nodiscard]] static auto get_number_of_columns() {
    return number_of_chord_columns;
  };
  [[nodiscard]] static auto is_column_editable(int column_number) {
    return column_number != chord_pitched_notes_column &&
           column_number != chord_unpitched_notes_column;
  };

  [[nodiscard]] auto get_data(int column_number) const -> QVariant override {
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
  };
  void set_data(int column, const QVariant &new_value) override {
    switch (column) {
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
  };

  void copy_columns_from(const Chord &template_row, int left_column,
                         int right_column) {
    for (auto chord_column = left_column; chord_column <= right_column;
         chord_column++) {
      switch (chord_column) {
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
  };

  [[nodiscard]] auto to_json() const -> nlohmann::json override {
    auto json_chord = Row::to_json();

    add_named_to_json(json_chord, instrument_pointer);
    add_abstract_rational_to_json(json_chord, interval, "interval");

    add_named_to_json(json_chord, percussion_set_pointer);
    add_named_to_json(json_chord, percussion_instrument_pointer);

    add_abstract_rational_to_json(json_chord, tempo_ratio, "tempo_ratio");
    add_rows_to_json(json_chord, pitched_notes);
    add_rows_to_json(json_chord, unpitched_notes);
    return json_chord;
  };
  [[nodiscard]] auto columns_to_json(int left_column, int right_column) const
      -> nlohmann::json override {
    auto json_chord = nlohmann::json::object();

    for (auto chord_column = left_column; chord_column <= right_column;
         chord_column = chord_column + 1) {
      switch (chord_column) {
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
    return json_chord;
  };
};

struct Song {
  double starting_key = DEFAULT_STARTING_KEY;
  double starting_velocity = DEFAULT_STARTING_VELOCITY;
  double starting_tempo = DEFAULT_STARTING_TEMPO;
  QList<Chord> chords;
};

[[nodiscard]] static auto get_midi(double key) {
  return HALFSTEPS_PER_OCTAVE * log2(key / CONCERT_A_FREQUENCY) +
         CONCERT_A_MIDI;
};

[[nodiscard]] static auto get_note_text(int degree) {
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
}

[[nodiscard]] static auto get_key_text(const Song &song, int chord_number,
                                       double ratio = 1) {
  const auto &chords = song.chords;
  auto key = song.starting_key;
  for (auto previous_chord_number = 0; previous_chord_number <= chord_number;
       previous_chord_number++) {
    key = key * chords.at(previous_chord_number).interval.to_double();
  }
  key = key * ratio;
  auto midi_float = get_midi(key);
  auto closest_midi = to_int(midi_float);
  auto difference_from_c = closest_midi - C_0_MIDI;
  auto octave =
      difference_from_c / HALFSTEPS_PER_OCTAVE; // floor integer division
  auto degree = difference_from_c - octave * HALFSTEPS_PER_OCTAVE;
  auto cents = to_int((midi_float - closest_midi) * CENTS_PER_HALFSTEP);

  QString result;
  QTextStream stream(&result);
  stream << key << QObject::tr(" Hz; ") << QObject::tr(get_note_text(degree))
         << octave;
  if (cents != 0) {
    stream << QObject::tr(cents >= 0 ? " + " : " − ") << abs(cents)
           << QObject::tr(" cents");
  }
  return result;
};

struct Player {
  // data
  QWidget &parent;

  // play state fields
  QList<double> channel_schedules;

  const Instrument *current_instrument_pointer = nullptr;
  const PercussionSet *current_percussion_set_pointer = nullptr;
  const PercussionInstrument *current_percussion_instrument_pointer = nullptr;

  double starting_time = 0;
  double current_time = 0;
  double final_time = 0;

  double current_key = 0;
  double current_velocity = 0;
  double current_tempo = 0;

  fluid_settings_t *const settings_pointer;
  fluid_event_t *const event_pointer;
  fluid_sequencer_t *const sequencer_pointer;
  fluid_synth_t *synth_pointer;
  const unsigned int soundfont_id;
  const fluid_seq_id_t sequencer_id;

  fluid_audio_driver_t *audio_driver_pointer = nullptr;

  // methods
  explicit Player(QWidget &parent);
  ~Player();

  // prevent moving and copying
  Player(const Player &) = delete;
  auto operator=(const Player &) -> Player = delete;
  Player(Player &&) = delete;
  auto operator=(Player &&) -> Player = delete;
};

static void delete_audio_driver(Player &player) {
  auto *audio_driver_pointer = player.audio_driver_pointer;
  if (audio_driver_pointer != nullptr) {
    delete_fluid_audio_driver(audio_driver_pointer);
    player.audio_driver_pointer = nullptr;
  }
}

static void start_real_time(Player &player) {
  delete_audio_driver(player);

#ifndef NO_REALTIME_AUDIO
  auto *new_audio_driver =
      new_fluid_audio_driver(player.settings_pointer, player.synth_pointer);
  if (new_audio_driver == nullptr) {
    QMessageBox::warning(&player.parent, QObject::tr("Audio driver error"),
                         QObject::tr("Cannot start audio driver"));
  } else {
    player.audio_driver_pointer = new_audio_driver;
  }
#endif
}

Player::Player(QWidget &parent_input)
    : parent(parent_input),
      channel_schedules(QList<double>(NUMBER_OF_MIDI_CHANNELS, 0)),
      settings_pointer(get_settings_pointer()),
      event_pointer(new_fluid_event()),
      sequencer_pointer(new_fluid_sequencer2(0)),
      synth_pointer(new_fluid_synth(settings_pointer)),
      soundfont_id(get_soundfont_id(synth_pointer)),
      sequencer_id(fluid_sequencer_register_fluidsynth(sequencer_pointer,
                                                       synth_pointer)) {
  fluid_event_set_dest(event_pointer, sequencer_id);
  start_real_time(*this);
}

Player::~Player() {
  delete_audio_driver(*this);
  delete_fluid_event(event_pointer);
  delete_fluid_sequencer(sequencer_pointer);
  delete_fluid_synth(synth_pointer);
  delete_fluid_settings(settings_pointer);
}

static void send_event_at(const Player &player, double time) {
  Q_ASSERT(time >= 0);
  auto result =
      fluid_sequencer_send_at(player.sequencer_pointer, player.event_pointer,
                              static_cast<unsigned int>(std::round(time)), 1);
  Q_ASSERT(result == FLUID_OK);
};

static void stop_playing(const Player &player) {
  auto *event_pointer = player.event_pointer;
  auto *sequencer_pointer = player.sequencer_pointer;

  fluid_sequencer_remove_events(sequencer_pointer, -1, -1, -1);

  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       channel_number = channel_number + 1) {
    fluid_event_all_sounds_off(event_pointer, channel_number);
    fluid_sequencer_send_now(sequencer_pointer, event_pointer);
  }
}

static void initialize_play(Player &player, const Song &song) {
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

static void modulate_before_chord(Player &player, const Song &song,
                                  int next_chord_number) {
  const auto &chords = song.chords;
  if (next_chord_number > 0) {
    for (auto chord_number = 0; chord_number < next_chord_number;
         chord_number = chord_number + 1) {
      modulate(player, chords.at(chord_number));
    }
  }
}

static void update_final_time(Player &player, double new_final_time) {
  if (new_final_time > player.final_time) {
    player.final_time = new_final_time;
  }
};

[[nodiscard]] static auto get_beat_time(double tempo) {
  return SECONDS_PER_MINUTE / tempo;
}

template <std::derived_from<Note> SubNote>
static void play_notes(Player &player, int chord_number,
                       const QList<SubNote> &sub_notes, int first_note_number,
                       int number_of_notes) {
  auto &parent = player.parent;
  auto *event_pointer = player.event_pointer;
  auto &channel_schedules = player.channel_schedules;
  const auto soundfont_id = player.soundfont_id;

  const auto current_time = player.current_time;
  const auto current_velocity = player.current_velocity;
  const auto current_tempo = player.current_tempo;

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
      QMessageBox::warning(&parent, QObject::tr("MIDI channel error"), message);
      return;
    }
    const auto &sub_note = sub_notes.at(note_number);

    const Program &program =
        sub_note.get_program(player, chord_number, note_number);

    fluid_event_program_select(event_pointer, channel_number, soundfont_id,
                               program.bank_number, program.preset_number);
    send_event_at(player, current_time);

    auto midi_number = sub_note.get_closest_midi(player, channel_number,
                                                 chord_number, note_number);

    auto velocity = current_velocity * sub_note.velocity_ratio.to_double();
    short new_velocity = 1;
    if (velocity > MAX_VELOCITY) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Velocity ") << velocity << QObject::tr(" exceeds ")
             << MAX_VELOCITY;
      add_note_location<SubNote>(stream, chord_number, note_number);
      stream << QObject::tr(". Playing with velocity ") << MAX_VELOCITY;
      QMessageBox::warning(&parent, QObject::tr("Velocity error"), message);
    } else {
      new_velocity = static_cast<short>(std::round(velocity));
    }
    fluid_event_noteon(event_pointer, channel_number, midi_number,
                       new_velocity);
    send_event_at(player, current_time);

    auto end_time = current_time + get_beat_time(current_tempo) *
                                       sub_note.beats.to_double() *
                                       MILLISECONDS_PER_SECOND;

    fluid_event_noteoff(event_pointer, channel_number, midi_number);
    send_event_at(player, end_time);

    channel_schedules[channel_number] = end_time;
  }
}

template <std::derived_from<Note> SubNote>
static void play_all_notes(Player &player, int chord_number,
                           const QList<SubNote> &sub_notes,
                           int first_note_number) {
  play_notes(player, chord_number, sub_notes, first_note_number,
             static_cast<int>(sub_notes.size()));
}

static void play_chords(Player &player, const Song &song,
                        int first_chord_number, int number_of_chords,
                        int wait_frames = 0) {
  auto start_time = player.current_time + wait_frames;
  player.current_time = start_time;
  update_final_time(player, start_time);
  const auto &chords = song.chords;
  for (auto chord_number = first_chord_number;
       chord_number < first_chord_number + number_of_chords;
       chord_number = chord_number + 1) {
    const auto &chord = chords.at(chord_number);

    modulate(player, chord);
    play_all_notes(player, chord_number, chord.pitched_notes, 0);
    play_all_notes(player, chord_number, chord.unpitched_notes, 0);
    auto new_current_time =
        player.current_time + get_beat_time(player.current_tempo) *
                                  chord.beats.to_double() *
                                  MILLISECONDS_PER_SECOND;
    player.current_time = new_current_time;
    update_final_time(player, new_current_time);
  }
}

static void set_fluid_string(fluid_settings_t *settings_pointer,
                             const char *field, const char *value) {
  auto result = fluid_settings_setstr(settings_pointer, field, value);
  Q_ASSERT(result == FLUID_OK);
}

static void export_song_to_file(Player &player, const Song &song,
                                const QString &output_file) {
  auto *settings_pointer = player.settings_pointer;
  auto *event_pointer = player.event_pointer;

  stop_playing(player);

  delete_audio_driver(player);
  set_fluid_string(settings_pointer, "audio.file.name",
                   output_file.toStdString().c_str());

  set_fluid_int(settings_pointer, "synth.lock-memory", 0);

  auto finished = false;
  auto finished_timer_id = fluid_sequencer_register_client(
      player.sequencer_pointer, "finished timer",
      [](unsigned int /*time*/, fluid_event_t * /*event*/,
         fluid_sequencer_t * /*seq*/, void *data_pointer) {
        auto *finished_pointer = static_cast<bool *>(data_pointer);
        Q_ASSERT(finished_pointer != nullptr);
        *finished_pointer = true;
      },
      &finished);
  Q_ASSERT(finished_timer_id >= 0);

  initialize_play(player, song);
  play_chords(player, song, 0, static_cast<int>(song.chords.size()),
              START_END_MILLISECONDS);

  fluid_event_set_dest(event_pointer, finished_timer_id);
  fluid_event_timer(event_pointer, nullptr);
  send_event_at(player, player.final_time + START_END_MILLISECONDS);

  auto *renderer_pointer = new_fluid_file_renderer(player.synth_pointer);
  Q_ASSERT(renderer_pointer != nullptr);
  while (!finished) {
    auto process_result = fluid_file_renderer_process_block(renderer_pointer);
    Q_ASSERT(process_result == FLUID_OK);
  }
  delete_fluid_file_renderer(renderer_pointer);

  fluid_event_set_dest(event_pointer, player.sequencer_id);
  set_fluid_int(settings_pointer, "synth.lock-memory", 1);
  start_real_time(player);
};

template <std::derived_from<Row> SubRow> struct SetCell : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  QModelIndex index;
  const QVariant old_value;
  const QVariant new_value;

  explicit SetCell(RowsModel<SubRow> &rows_model_input,
                   const QModelIndex &index_input, QVariant new_value_input)
      : rows_model(rows_model_input), index(index_input),
        old_value(rows_model.data(index, Qt::DisplayRole)),
        new_value(std::move(new_value_input)){};

  void undo() override { set_model_data_directly(*this, old_value); };

  void redo() override { set_model_data_directly(*this, new_value); };
};

template <std::derived_from<Row> SubRow>
static void set_model_data_directly(SetCell<SubRow> &change,
                                    const QVariant &set_value) {
  const auto &index = change.index;
  change.rows_model.set_cell(index.row(), index.column(), set_value);
}

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

  [[nodiscard]] auto headerData(int section, Qt::Orientation orientation,
                                int role) const -> QVariant override {
    if (role != Qt::DisplayRole) {
      return {};
    }
    switch (orientation) {
    case Qt::Horizontal:
      return RowsModel::tr(SubRow::get_column_name(section));
    case Qt::Vertical:
      return section + 1;
    default:
      Q_ASSERT(false);
      return {};
    }
  };

  [[nodiscard]] auto
  flags(const QModelIndex &index) const -> Qt::ItemFlags override {
    auto uneditable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (SubRow::is_column_editable(index.column())) {
      return uneditable | Qt::ItemIsEditable;
    }
    return uneditable;
  };

  [[nodiscard]] virtual auto
  is_column_editable(int /*column_number*/) const -> bool {
    return true;
  };

  [[nodiscard]] virtual auto get_status(int /*row_number*/) const -> QString {
    return "";
  };

  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override {
    Q_ASSERT(index.isValid());
    auto row_number = index.row();

    if (role == Qt::StatusTipRole) {
      return get_status(row_number);
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
      return {};
    }

    return get_const_reference(rows_pointer)
        .at(row_number)
        .get_data(index.column());
  }

  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             int role) -> bool override {
    // only set data for edit
    if (role != Qt::EditRole) {
      return false;
    };
    undo_stack.push(
        new SetCell<SubRow>( // NOLINT(cppcoreguidelines-owning-memory)
            *this, index, new_value));
    return true;
  };

  // don't inline these functions because they use protected methods
  void set_cell(int row_number, int column_number, const QVariant &new_value) {
    get_reference(rows_pointer)[row_number].set_data(column_number, new_value);
    dataChanged(index(row_number, column_number),
                index(row_number, column_number),
                {Qt::DisplayRole, Qt::EditRole});
  }

  void set_cells(int first_row_number, const QList<SubRow> &new_rows,
                 int left_column, int right_column) {
    auto &rows = get_reference(rows_pointer);
    auto number_of_new_rows = new_rows.size();
    for (auto replace_number = 0; replace_number < number_of_new_rows;
         replace_number++) {
      rows[first_row_number + replace_number].copy_columns_from(
          new_rows.at(replace_number), left_column, right_column);
    }
    dataChanged(index(first_row_number, left_column),
                index(first_row_number + number_of_new_rows - 1, right_column),
                {Qt::DisplayRole, Qt::EditRole});
  }

  void insert_json_rows(int first_row_number, const nlohmann::json &json_rows) {
    auto &rows = get_reference(rows_pointer);
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + static_cast<int>(json_rows.size()) - 1);
    json_to_rows(rows, json_rows);
    endInsertRows();
  }

  void insert_rows(int first_row_number, const QList<SubRow> &new_rows) {
    auto &rows = get_reference(rows_pointer);
    beginInsertRows(QModelIndex(), first_row_number,
                    first_row_number + new_rows.size() - 1);
    std::copy(new_rows.cbegin(), new_rows.cend(),
              std::inserter(rows, rows.begin() + first_row_number));
    endInsertRows();
  }

  void insert_row(int row_number, const SubRow &new_row) {
    beginInsertRows(QModelIndex(), row_number, row_number);
    auto &rows = get_reference(rows_pointer);
    rows.insert(rows.begin() + row_number, new_row);
    endInsertRows();
  }

  void remove_rows(int first_row_number, int number_of_rows) {
    auto &rows = get_reference(rows_pointer);
    beginRemoveRows(QModelIndex(), first_row_number,
                    first_row_number + number_of_rows - 1);
    rows.erase(rows.begin() + first_row_number,
               rows.begin() + first_row_number + number_of_rows);
    endRemoveRows();
  }

  void set_rows_pointer(QList<SubRow> *new_rows = nullptr) {
    beginResetModel();
    rows_pointer = new_rows;
    endResetModel();
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
                    int first_row_number_input, int left_column_input,
                    int right_column_input, QList<SubRow> old_rows_input,
                    QList<SubRow> new_rows_input)
      : rows_model(rows_model_input), first_row_number(first_row_number_input),
        left_column(left_column_input), right_column(right_column_input),
        old_rows(std::move(old_rows_input)),
        new_rows(std::move(new_rows_input)){};

  void undo() override { set_cells(*this, old_rows); };

  void redo() override { set_cells(*this, new_rows); }
};

template <std::derived_from<Row> SubRow>
static void set_cells(SetCells<SubRow> &change, const QList<SubRow> &set_rows) {
  change.rows_model.set_cells(change.first_row_number, set_rows,
                              change.left_column, change.right_column);
}

template <std::derived_from<Row> SubRow>
struct InsertRow : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int row_number;
  const SubRow new_row;

  InsertRow(RowsModel<SubRow> &rows_model_input, int row_number_input,
            SubRow new_row_input)
      : rows_model(rows_model_input), row_number(row_number_input),
        new_row(std::move(new_row_input)){};

  void undo() override { rows_model.remove_rows(row_number, 1); };
  void redo() override { rows_model.insert_row(row_number, new_row); };
};

template <std::derived_from<Row> SubRow>
struct InsertRemoveRows : public QUndoCommand {
  RowsModel<SubRow> &rows_model;
  const int first_row_number;
  const QList<SubRow> new_rows;
  const bool backwards;
  InsertRemoveRows(RowsModel<SubRow> &rows_model_input,
                   int first_row_number_input, QList<SubRow> new_rows_input,
                   bool backwards_input)
      : rows_model(rows_model_input), first_row_number(first_row_number_input),
        new_rows(std::move(new_rows_input)), backwards(backwards_input){};

  void undo() override { insert_or_remove(*this, backwards); };

  void redo() override { insert_or_remove(*this, !backwards); };
};

template <std::derived_from<Row> SubRow>
static void insert_or_remove(const InsertRemoveRows<SubRow> &change,
                             bool should_insert) {
  auto &rows_model = change.rows_model;
  const auto &new_rows = change.new_rows;
  const auto first_row_number = change.first_row_number;

  if (should_insert) {
    rows_model.insert_rows(first_row_number, new_rows);
  } else {
    rows_model.remove_rows(first_row_number, new_rows.size());
  }
}

struct ChordsModel : public RowsModel<Chord> {
  Song &song;

  explicit ChordsModel(QUndoStack &undo_stack, Song &song_input)
      : RowsModel(undo_stack), song(song_input) {
    rows_pointer = &song.chords;
  };
  [[nodiscard]] auto get_status(int row_number) const -> QString override {
    return get_key_text(song, row_number);
  };
};

struct PitchedNotesModel : public RowsModel<PitchedNote> {
  Song &song;
  int parent_chord_number = -1;

  explicit PitchedNotesModel(QUndoStack &undo_stack, Song &song_input)
      : RowsModel<PitchedNote>(undo_stack), song(song_input){};

  [[nodiscard]] auto get_status(int row_number) const -> QString override {
    return get_key_text(
        song, parent_chord_number,
        get_const_reference(rows_pointer).at(row_number).interval.to_double());
  };
};

struct SongEditor : public QMainWindow {
  Q_OBJECT;

public:
  // data
  Song song;
  Player player;

  // mode fields
  ModelType current_model_type = chords_type;
  int current_chord_number = -1;

  // folder/file fields
  QString current_folder;
  QString current_file;

  // const fields
  QUndoStack &undo_stack;

  // starting controls
  QDoubleSpinBox &gain_editor;
  QDoubleSpinBox &starting_key_editor;
  QDoubleSpinBox &starting_velocity_editor;
  QDoubleSpinBox &starting_tempo_editor;

  // views and models
  QLabel &editing_text;
  QTableView &table_view;
  ChordsModel chords_model;
  PitchedNotesModel pitched_notes_model;
  RowsModel<UnpitchedNote> unpitched_notes_model;

  // mode actions
  QAction back_to_chords_action;

  // insert remove actions
  QAction insert_after_action;
  QAction insert_into_action;
  QAction delete_action;
  QAction remove_rows_action;

  // copy paste actions
  QAction cut_action;
  QAction copy_action;
  QAction paste_over_action;
  QAction paste_into_action;
  QAction paste_after_action;

  // play actions
  QAction play_action;
  QAction stop_playing_action;

  // io actions
  QAction save_action;
  QAction open_action;

  // methods
  explicit SongEditor();
  ~SongEditor() override { undo_stack.disconnect(); };

  // prevent moving and copying
  SongEditor(const SongEditor &) = delete;
  auto operator=(const SongEditor &) -> SongEditor = delete;
  SongEditor(SongEditor &&) = delete;
  auto operator=(SongEditor &&) -> SongEditor = delete;

  void closeEvent(QCloseEvent *close_event_pointer) override;
};

[[nodiscard]] static auto reference_get_gain(const SongEditor &song_editor) {
  return fluid_synth_get_gain(song_editor.player.synth_pointer);
};

static void show_warning(QWidget &parent, const char *title,
                         const QString &message) {
  QMessageBox::warning(&parent, SongEditor::tr(title), message);
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

// get json functions
[[nodiscard]] static auto get_json_value(const nlohmann::json &json_data,
                                         const char *field) {
  Q_ASSERT(json_data.is_object());
  Q_ASSERT(json_data.contains(field));
  return json_data[field];
}

static void set_double_from_json(const nlohmann::json &json_song,
                                 QDoubleSpinBox &double_editor,
                                 const char *field_name) {
  if (json_song.contains(field_name)) {
    const auto &json_value = get_json_value(json_song, field_name);
    Q_ASSERT(json_value.is_number());
    double_editor.setValue(json_value.get<double>());
  }
}

static void reference_open_file(SongEditor &song_editor,
                                const QString &filename) {
  auto &chords = song_editor.song.chords;
  auto &chords_model = song_editor.chords_model;
  auto &undo_stack = song_editor.undo_stack;

  std::ifstream file_io(filename.toStdString().c_str());
  nlohmann::json json_song;
  try {
    json_song = nlohmann::json::parse(file_io);
  } catch (const nlohmann::json::parse_error &parse_error) {
    show_warning(song_editor, "Parsing error", parse_error.what());
    return;
  }
  file_io.close();

  static auto song_validator = []() {
    nlohmann::json song_schema(
        {{"gain", get_number_schema("number", 0, MAX_GAIN)},
         {"starting_key",
          get_number_schema("number", MIN_STARTING_KEY, MAX_STARTING_KEY)},
         {"starting_tempo",
          get_number_schema("number", MIN_STARTING_TEMPO, MAX_STARTING_TEMPO)},
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
    show_warning(song_editor, "Schema error", error.what());
    return;
  }

  set_double_from_json(json_song, song_editor.gain_editor, "gain");
  set_double_from_json(json_song, song_editor.starting_key_editor,
                       "starting_key");
  set_double_from_json(json_song, song_editor.starting_velocity_editor,
                       "starting_velocity");
  set_double_from_json(json_song, song_editor.starting_tempo_editor,
                       "starting_tempo");

  if (!chords.empty()) {
    chords_model.remove_rows(0, static_cast<int>(chords.size()));
  }

  if (json_song.contains("chords")) {
    chords_model.insert_json_rows(0, json_song["chords"]);
  }

  song_editor.current_file = filename;

  undo_stack.clear();
  undo_stack.setClean();
}

static void reference_safe_as_file(SongEditor &song_editor,
                                   const QString &filename) {
  const auto &song = song_editor.song;

  std::ofstream file_io(filename.toStdString().c_str());

  nlohmann::json json_song;
  json_song["gain"] = reference_get_gain(song_editor);
  json_song["starting_key"] = song.starting_key;
  json_song["starting_tempo"] = song.starting_tempo;
  json_song["starting_velocity"] = song.starting_velocity;

  add_rows_to_json(json_song, song.chords);

  file_io << std::setw(4) << json_song;
  file_io.close();
  song_editor.current_file = filename;

  song_editor.undo_stack.setClean();
}

auto UnpitchedNote::get_closest_midi(Player &player, int /*channel_number*/,
                                     int chord_number,
                                     int note_number) const -> short {
  return substitute_named_for<UnpitchedNote>(
             player.parent, percussion_instrument_pointer,
             player.current_percussion_instrument_pointer, "Tambourine",
             chord_number, note_number, "Percussion instrument error",
             "No percussion instrument", ". Using Tambourine.")
      .midi_number;
}

auto UnpitchedNote::get_program(const Player &player, int chord_number,
                                int note_number) const -> const Program & {
  return substitute_named_for<UnpitchedNote>(
      player.parent, percussion_set_pointer,
      player.current_percussion_set_pointer, "Marimba", chord_number,
      note_number, "Percussion set error", "No percussion set",
      ". Using Standard.");
}

auto PitchedNote::get_closest_midi(Player &player, int channel_number,
                                   int /*chord_number*/,
                                   int /*note_number*/) const -> short {
  auto midi_float = get_midi(player.current_key * interval.to_double());
  auto closest_midi = static_cast<short>(round(midi_float));

  fluid_event_pitch_bend(
      player.event_pointer, channel_number,
      to_int((midi_float - closest_midi + ZERO_BEND_HALFSTEPS) *
             BEND_PER_HALFSTEP));
  send_event_at(player, player.current_time);
  return closest_midi;
}

auto PitchedNote::get_program(const Player &player, int chord_number,
                              int note_number) const -> const Program & {
  return substitute_named_for<PitchedNote>(
      player.parent, instrument_pointer, player.current_instrument_pointer,
      "Marimba", chord_number, note_number, "Instrument error", "No instrument",
      ". Using Marimba.");
}

static void player_play_pitched_notes(Player &player, const Song &song,
                                      int chord_number, int first_note_number,
                                      int number_of_notes) {
  modulate_before_chord(player, song, chord_number);
  const auto &chord = song.chords.at(chord_number);
  modulate(player, chord);
  play_notes(player, chord_number, chord.pitched_notes, first_note_number,
             number_of_notes);
};

[[nodiscard]] static auto
get_selection_model(const QTableView &table_view) -> QItemSelectionModel & {
  return get_reference(table_view.selectionModel());
}

[[nodiscard]] static auto get_selection(const QTableView &table_view) {
  return get_selection_model(table_view).selection();
}

static void update_actions(SongEditor &song_editor) {
  auto anything_selected = !get_selection(song_editor.table_view).empty();

  song_editor.cut_action.setEnabled(anything_selected);
  song_editor.copy_action.setEnabled(anything_selected);
  song_editor.paste_over_action.setEnabled(anything_selected);
  song_editor.paste_after_action.setEnabled(anything_selected);
  song_editor.insert_after_action.setEnabled(anything_selected);
  song_editor.delete_action.setEnabled(anything_selected);
  song_editor.remove_rows_action.setEnabled(anything_selected);
  song_editor.play_action.setEnabled(anything_selected);
}

static void set_model(SongEditor &song_editor, QAbstractItemModel &model) {
  song_editor.table_view.setModel(&model);
  update_actions(song_editor);

  SongEditor::connect(&get_selection_model(song_editor.table_view),
                      &QItemSelectionModel::selectionChanged, &song_editor,
                      [&song_editor]() { update_actions(song_editor); });
};

static void edit_children_or_back(SongEditor &song_editor, int chord_number,
                                  bool is_pitched, bool should_edit_children) {
  auto &editing_text = song_editor.editing_text;

  song_editor.back_to_chords_action.setEnabled(should_edit_children);
  song_editor.open_action.setEnabled(!should_edit_children);

  if (should_edit_children) {
    QString label_text;
    QTextStream stream(&label_text);
    stream << SongEditor::tr(is_pitched ? "Editing pitched notes for chord "
                                        : "Editing unpitched notes for chord ")
           << chord_number + 1;
    editing_text.setText(label_text);
    Q_ASSERT(song_editor.current_model_type == chords_type);
    song_editor.current_model_type =
        is_pitched ? pitched_notes_type : unpitched_notes_type;
    song_editor.current_chord_number = chord_number;

    auto &chord = song_editor.song.chords[chord_number];
    if (is_pitched) {
      auto &pitched_notes_model = song_editor.pitched_notes_model;
      pitched_notes_model.parent_chord_number = chord_number;
      pitched_notes_model.set_rows_pointer(&chord.pitched_notes);
      set_model(song_editor, pitched_notes_model);
    } else {
      auto &unpitched_notes_model = song_editor.unpitched_notes_model;
      unpitched_notes_model.set_rows_pointer(&chord.unpitched_notes);
      set_model(song_editor, unpitched_notes_model);
    }
  } else {
    set_model(song_editor, song_editor.chords_model);

    editing_text.setText(SongEditor::tr("Editing chords"));
    song_editor.current_model_type = chords_type;
    song_editor.current_chord_number = -1;

    if (is_pitched) {
      auto &pitched_notes_model = song_editor.pitched_notes_model;
      pitched_notes_model.set_rows_pointer();
      pitched_notes_model.parent_chord_number = -1;
    } else {
      song_editor.unpitched_notes_model.set_rows_pointer();
    }
  }
};

struct EditChildrenOrBack : public QUndoCommand {
  SongEditor &song_editor;
  int chord_number;
  bool is_pitched;
  bool backwards;

  explicit EditChildrenOrBack(SongEditor &song_editor_input,
                              int chord_number_input, bool is_notes_input,
                              bool backwards_input)
      : song_editor(song_editor_input), chord_number(chord_number_input),
        is_pitched(is_notes_input), backwards(backwards_input){};
  void undo() override {
    edit_children_or_back(song_editor, chord_number, is_pitched, backwards);
  };
  void redo() override {
    edit_children_or_back(song_editor, chord_number, is_pitched, !backwards);
  };
};

static void set_starting_double(SongEditor &song_editor, ControlId control_id,
                                QDoubleSpinBox &spin_box, double set_value) {
  auto &song = song_editor.song;
  switch (control_id) {
  case gain_id:
    fluid_synth_set_gain(song_editor.player.synth_pointer,
                         static_cast<float>(set_value));
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
  SongEditor &song_editor;
  QDoubleSpinBox &spin_box;
  const ControlId control_id;
  const double old_value;
  double new_value;

  explicit SetStartingDouble(SongEditor &song_editor_input,
                             QDoubleSpinBox &spin_box_input,
                             ControlId command_id_input, double old_value_input,
                             double new_value_input)
      : song_editor(song_editor_input), spin_box(spin_box_input),
        control_id(command_id_input), old_value(old_value_input),
        new_value(new_value_input){};

  [[nodiscard]] auto id() const -> int override { return control_id; };
  [[nodiscard]] auto
  mergeWith(const QUndoCommand *next_command_pointer) -> bool override {
    Q_ASSERT(next_command_pointer != nullptr);

    const auto *next_velocity_change_pointer =
        dynamic_cast<const SetStartingDouble *>(next_command_pointer);

    new_value = get_const_reference(next_velocity_change_pointer).new_value;
    return true;
  };

  void undo() override {
    set_starting_double(song_editor, control_id, spin_box, old_value);
  };
  void redo() override {
    set_starting_double(song_editor, control_id, spin_box, new_value);
  };
};

[[nodiscard]] static auto get_json_int(const nlohmann::json &json_data,
                                       const char *field) {
  const auto &json_value = get_json_value(json_data, field);
  Q_ASSERT(json_value.is_number());
  return json_value.get<int>();
}

template <typename Iterable>
[[nodiscard]] static auto get_only(const Iterable &iterable) {
  Q_ASSERT(iterable.size() == 1);
  return iterable.at(0);
}

[[nodiscard]] static auto get_only_range(const SongEditor &song_editor) {
  return get_only(get_selection(song_editor.table_view));
}

[[nodiscard]] static auto get_number_of_rows(const QItemSelectionRange &range) {
  Q_ASSERT(range.isValid());
  return range.bottom() - range.top() + 1;
}

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto get_cells_mime() -> const QString & {
  static auto cells_mime = [] {
    QString cells_mime;
    QTextStream stream(&cells_mime);
    stream << "application/prs." << SubRow::get_plural_field_for()
           << "_cells+json";
    return cells_mime;
  }();
  return cells_mime;
}

[[nodiscard]] static auto get_mime_description(const QString &mime_type) {
  if (mime_type == get_cells_mime<Chord>()) {
    return SongEditor::tr("chords cells");
  }
  if (mime_type == get_cells_mime<PitchedNote>()) {
    return SongEditor::tr("pitched notes cells");
  }
  if (mime_type == get_cells_mime<UnpitchedNote>()) {
    return SongEditor::tr("unpitched notes cells");
  }
  return mime_type;
}

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto
add_set_cells(QUndoStack &undo_stack, RowsModel<SubRow> &rows_model,
              int first_row_number, int left_column, int right_column,
              const QList<SubRow> &old_rows, const QList<SubRow> &new_rows) {
  undo_stack.push(
      new SetCells<SubRow>( // NOLINT(cppcoreguidelines-owning-memory)
          rows_model, first_row_number, left_column, right_column, old_rows,
          new_rows));
}

template <std::derived_from<Row> SubRow>
static void add_insert_row(QUndoStack &undo_stack,
                           RowsModel<SubRow> &rows_model, int row_number,
                           const SubRow &new_row) {
  undo_stack.push(
      new InsertRow<SubRow>( // NOLINT(cppcoreguidelines-owning-memory)
          rows_model, row_number, new_row));
}

template <std::derived_from<Row> SubRow>
static void
add_insert_remove_rows(QUndoStack &undo_stack, RowsModel<SubRow> &rows_model,
                       int row_number, const QList<SubRow> &new_rows,
                       bool backwards) {
  undo_stack.push(
      new InsertRemoveRows<SubRow>( // NOLINT(cppcoreguidelines-owning-memory)
          rows_model, row_number, new_rows, backwards));
}

template <typename Item>
[[nodiscard]] static auto copy_items(const QList<Item> &items,
                                     int first_row_number, int number_of_rows) {
  QList<Item> copied;
  std::copy(items.cbegin() + first_row_number,
            items.cbegin() + first_row_number + number_of_rows,
            std::back_inserter(copied));
  return copied;
}

[[nodiscard]] static auto can_discard_changes(SongEditor &song_editor) {
  return song_editor.undo_stack.isClean() ||
         QMessageBox::question(&song_editor, SongEditor::tr("Unsaved changes"),
                               SongEditor::tr("Discard unsaved changes?")) ==
             QMessageBox::Yes;
}

[[nodiscard]] static auto
make_file_dialog(SongEditor &song_editor, const char *caption,
                 const QString &filter, QFileDialog::AcceptMode accept_mode,
                 const QString &suffix,
                 QFileDialog::FileMode file_mode) -> QFileDialog & {
  auto &dialog = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QFileDialog(&song_editor, SongEditor::tr(caption),
                        song_editor.current_folder, filter));

  dialog.setAcceptMode(accept_mode);
  dialog.setDefaultSuffix(suffix);
  dialog.setFileMode(file_mode);

  return dialog;
}

[[nodiscard]] static auto get_selected_file(SongEditor &song_editor,
                                            QFileDialog &dialog) {
  song_editor.current_folder = dialog.directory().absolutePath();
  return get_only(dialog.selectedFiles());
}

[[nodiscard]] static auto get_clipboard() -> QClipboard & {
  return get_reference(QGuiApplication::clipboard());
}

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto
get_rows(RowsModel<SubRow> &rows_model) -> QList<SubRow> & {
  return get_reference(rows_model.rows_pointer);
};

template <std::derived_from<Row> SubRow>
static void copy_template(const QItemSelectionRange &range,
                          RowsModel<SubRow> &rows_model) {
  auto first_row_number = range.top();
  auto left_column = range.left();
  auto right_column = range.right();

  auto &rows = get_rows(rows_model);

  nlohmann::json copied_json = nlohmann::json::array();
  std::transform(
      rows.cbegin() + first_row_number,
      rows.cbegin() + first_row_number + get_number_of_rows(range),
      std::back_inserter(copied_json),
      [left_column, right_column](const SubRow &row) -> nlohmann::json {
        return row.columns_to_json(left_column, right_column);
      });

  const nlohmann::json copied(
      {{"left_column", left_column},
       {"right_column", right_column},
       {SubRow::get_plural_field_for(), std::move(copied_json)}});

  std::stringstream json_text;
  json_text << std::setw(4) << copied;

  auto &new_data = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QMimeData);

  new_data.setData(get_cells_mime<SubRow>(), json_text.str().c_str());

  get_clipboard().setMimeData(&new_data);
}

static void copy_selection(SongEditor &song_editor) {
  const auto &range = get_only_range(song_editor);
  switch (song_editor.current_model_type) {
  case chords_type:
    copy_template(range, song_editor.chords_model);
    return;
  case pitched_notes_type:
    copy_template(range, song_editor.pitched_notes_model);
    return;
  case unpitched_notes_type:
    copy_template(range, song_editor.unpitched_notes_model);
    return;
  default:
    Q_ASSERT(false);
    return;
  }
}

template <std::derived_from<Row> SubRow>
static void delete_cells_template(const QItemSelectionRange &range,
                                  QUndoStack &undo_stack,
                                  RowsModel<SubRow> &rows_model) {
  auto first_row_number = range.top();
  auto number_of_rows = get_number_of_rows(range);

  add_set_cells(
      undo_stack, rows_model, first_row_number, range.left(), range.right(),
      copy_items(get_rows(rows_model), first_row_number, number_of_rows),
      QList<SubRow>(number_of_rows));
}

static void delete_cells(SongEditor &song_editor) {
  const auto &range = get_only_range(song_editor);
  auto &undo_stack = song_editor.undo_stack;
  switch (song_editor.current_model_type) {
  case chords_type:
    delete_cells_template(range, undo_stack, song_editor.chords_model);
    return;
  case pitched_notes_type:
    delete_cells_template(range, undo_stack, song_editor.pitched_notes_model);
    return;
  case unpitched_notes_type:
    delete_cells_template(range, undo_stack, song_editor.unpitched_notes_model);
    return;
  default:
    Q_ASSERT(false);
    break;
  }
}

[[nodiscard]] static auto
get_required_object_schema(const nlohmann::json &required_json,
                           const nlohmann::json &properties_json) {
  return nlohmann::json({{"type", "object"},
                         {"required", required_json},
                         {"properties", properties_json}});
};

template <std::derived_from<Row> SubRow>
[[nodiscard]] static auto parse_clipboard(QWidget &parent) {
  const auto &mime_data = get_const_reference(get_clipboard().mimeData());
  const auto &mime_type = get_cells_mime<SubRow>();
  if (!mime_data.hasFormat(mime_type)) {
    auto formats = mime_data.formats();
    if (formats.empty()) {
      show_warning(parent, "Empty paste error",
                   SongEditor::tr("Nothing to paste!"));
      return nlohmann::json();
    };
    QString message;
    QTextStream stream(&message);
    stream << SongEditor::tr("Cannot paste ")
           << get_mime_description(formats[0])
           << SongEditor::tr(" into destination needing ")
           << get_mime_description(mime_type);
    show_warning(parent, "MIME type error", message);
    return nlohmann::json();
  }
  const auto &copied_text = mime_data.data(mime_type).toStdString();
  nlohmann::json copied;
  try {
    copied = nlohmann::json::parse(copied_text);
  } catch (const nlohmann::json::parse_error &parse_error) {
    show_warning(parent, "Parsing error", parse_error.what());
    return nlohmann::json();
  }
  if (copied.empty()) {
    show_warning(parent, "Empty paste", SongEditor::tr("Nothing to paste!"));
    return nlohmann::json();
  }
  static const auto cells_validator = []() {
    auto last_column = SubRow::get_number_of_columns() - 1;
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
    return nlohmann::json();
  }
  return copied;
}

template <std::derived_from<Row> SubRow>
static void paste_cells_template(SongEditor &song_editor,
                                 RowsModel<SubRow> &rows_model) {
  auto first_row_number = get_only_range(song_editor).top();

  const auto json_cells = parse_clipboard<SubRow>(song_editor);
  if (json_cells.empty()) {
    return;
  }
  const auto &json_rows =
      get_json_value(json_cells, SubRow::get_plural_field_for());

  const auto &rows = get_rows(rows_model);

  auto number_of_rows =
      std::min({static_cast<int>(json_rows.size()),
                static_cast<int>(rows.size()) - first_row_number});

  QList<SubRow> new_rows;
  partial_json_to_rows(new_rows, json_rows, number_of_rows);
  add_set_cells(song_editor.undo_stack, rows_model, first_row_number,
                get_json_int(json_cells, "left_column"),
                get_json_int(json_cells, "right_column"),
                copy_items(rows, first_row_number, number_of_rows),
                std::move(new_rows));
}

template <std::derived_from<Row> SubRow>
static void paste_insert_template(SongEditor &song_editor,
                                  RowsModel<SubRow> &rows_model,
                                  int row_number) {
  const auto json_cells = parse_clipboard<SubRow>(song_editor);
  if (json_cells.empty()) {
    return;
  }
  const auto &json_rows =
      get_json_value(json_cells, SubRow::get_plural_field_for());

  QList<SubRow> new_rows;
  json_to_rows(new_rows, json_rows);
  add_insert_remove_rows(song_editor.undo_stack, rows_model, row_number,
                         new_rows, false);
}

static void paste_insert(SongEditor &song_editor, int row_number) {
  switch (song_editor.current_model_type) {
  case chords_type:
    paste_insert_template(song_editor, song_editor.chords_model, row_number);
    return;
  case pitched_notes_type:
    paste_insert_template(song_editor, song_editor.pitched_notes_model,
                          row_number);
    return;
  case unpitched_notes_type:
    paste_insert_template(song_editor, song_editor.unpitched_notes_model,
                          row_number);
    return;
  default:
    Q_ASSERT(false);
    return;
  }
}

static void insert_model_row(SongEditor &song_editor, int row_number) {
  const auto current_model_type = song_editor.current_model_type;
  auto &undo_stack = song_editor.undo_stack;
  if (current_model_type == chords_type) {
    add_insert_row(undo_stack, song_editor.chords_model, row_number, Chord());
  } else {
    const auto &chord_beats =
        song_editor.song.chords[song_editor.current_chord_number].beats;
    if (current_model_type == pitched_notes_type) {
      PitchedNote new_pitched_note;
      new_pitched_note.beats = chord_beats;
      add_insert_row(undo_stack, song_editor.pitched_notes_model, row_number,
                     new_pitched_note);
    } else {
      UnpitchedNote new_unpitched_note;
      new_unpitched_note.beats = chord_beats;
      add_insert_row(undo_stack, song_editor.unpitched_notes_model, row_number,
                     new_unpitched_note);
    }
  }
}

template <std::derived_from<Row> SubRow>
static void remove_rows_template(const QItemSelectionRange &range,
                                 QUndoStack &undo_stack,
                                 RowsModel<SubRow> &rows_model) {
  auto first_row_number = range.top();
  auto number_of_rows = get_number_of_rows(range);

  add_insert_remove_rows(
      undo_stack, rows_model, first_row_number,
      copy_items(get_rows(rows_model), first_row_number, number_of_rows), true);
}

static void add_edit_children_or_back(SongEditor &song_editor, int chord_number,
                                      bool is_pitched, bool backwards) {
  song_editor.undo_stack.push(
      new EditChildrenOrBack( // NOLINT(cppcoreguidelines-owning-memory)
          song_editor, chord_number, is_pitched, backwards));
}

static void set_double(SongEditor &song_editor, QDoubleSpinBox &spin_box,
                       ControlId control_id, double old_value,
                       double new_value) {
  song_editor.undo_stack.push(
      new SetStartingDouble( // NOLINT(cppcoreguidelines-owning-memory)
          song_editor, spin_box, control_id, old_value, new_value));
}

static void connect_model(SongEditor &song_editor,
                          const QAbstractItemModel &model) {
  SongEditor::connect(&model, &QAbstractItemModel::rowsInserted, &song_editor,
                      [&song_editor]() { update_actions(song_editor); });
  SongEditor::connect(&model, &QAbstractItemModel::rowsRemoved, &song_editor,
                      [&song_editor]() { update_actions(song_editor); });
}

template <typename Functor>
static void add_menu_action(SongEditor &song_editor, QMenu &menu,
                            QAction &action, Functor &&trigger_action,
                            QKeySequence::StandardKey key_sequence,
                            bool enabled = true) {
  action.setShortcuts(key_sequence);
  action.setEnabled(enabled);
  menu.addAction(&action);
  QObject::connect(&action, &QAction::triggered, &song_editor, trigger_action);
}

template <typename Functor>
static void add_control(SongEditor &song_editor, QFormLayout &controls_form,
                        const char *label, QDoubleSpinBox &spin_box,
                        double starting_value, int minimum, int maximum,
                        double single_step, int decimals, const char *suffix,
                        Functor &&value_action) {
  spin_box.setMinimum(minimum);
  spin_box.setMaximum(maximum);
  spin_box.setSingleStep(single_step);
  spin_box.setDecimals(decimals);
  spin_box.setSuffix(SongEditor::tr(suffix));
  QObject::connect(&spin_box, &QDoubleSpinBox::valueChanged, &song_editor,
                   value_action);
  spin_box.setValue(starting_value);
  controls_form.addRow(SongEditor::tr(label), &spin_box);
}

[[nodiscard]] static auto make_action(const char *name,
                                      QWidget &parent) -> QAction & {
  return *(new QAction(SongEditor::tr(name), &parent));
}

[[nodiscard]] static auto move_action(const char *name) {
  return QAction(SongEditor::tr(name));
}

[[nodiscard]] static auto make_menu(const char *name,
                                    QWidget &parent) -> QMenu & {
  return *(new QMenu(SongEditor::tr(name), &parent));
}

SongEditor::SongEditor()
    : player(Player(*this)), current_folder(QStandardPaths::writableLocation(
                                 QStandardPaths::DocumentsLocation)),
      undo_stack(*(new QUndoStack(this))), gain_editor(*(new QDoubleSpinBox)),
      starting_key_editor(*(new QDoubleSpinBox)),
      starting_velocity_editor(*(new QDoubleSpinBox)),
      starting_tempo_editor(*(new QDoubleSpinBox)),
      editing_text(*(new QLabel(SongEditor::tr("Editing chords")))),
      table_view(*(new QTableView)),
      chords_model(ChordsModel(undo_stack, song)),
      pitched_notes_model(PitchedNotesModel(undo_stack, song)),
      unpitched_notes_model(RowsModel<UnpitchedNote>(undo_stack)),
      back_to_chords_action(move_action("&Back to chords")),
      insert_after_action(move_action("&After")),
      insert_into_action(move_action("&Into start")),
      delete_action(move_action("&Delete")),
      remove_rows_action(move_action("&Remove rows")),
      cut_action(move_action("&Cut")), copy_action(move_action("&Copy")),
      paste_over_action(move_action("&Over")),
      paste_into_action(move_action("&Into start")),
      paste_after_action(move_action("&After")),
      play_action(move_action("&Play selection")),
      stop_playing_action(move_action("&Stop playing")),
      save_action(move_action("&Save")), open_action(move_action("&Open")) {
  statusBar()->showMessage("");

  auto &controls = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QWidget);
  controls.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  auto &dock_widget = *(new QDockWidget(SongEditor::tr("Controls")));

  auto &menu_bar = get_reference(menuBar());

  auto &file_menu = make_menu("&File", *this);

  add_menu_action(
      *this, file_menu, open_action,
      [this]() {
        if (can_discard_changes(*this)) {
          auto &dialog = make_file_dialog(
              *this, "Open — Justly", "JSON file (*.json)",
              QFileDialog::AcceptOpen, ".json", QFileDialog::ExistingFile);
          if (dialog.exec() != 0) {
            reference_open_file(*this, get_selected_file(*this, dialog));
          }
        }
      },
      QKeySequence::Open);

  file_menu.addSeparator();

  add_menu_action(
      *this, file_menu, save_action,
      [this]() { reference_safe_as_file(*this, current_file); },
      QKeySequence::Save, false);

  add_menu_action(
      *this, file_menu, make_action("&Save As...", file_menu),
      [this]() {
        auto &dialog = make_file_dialog(
            *this, "Save As — Justly", "JSON file (*.json)",
            QFileDialog::AcceptSave, ".json", QFileDialog::AnyFile);

        if (dialog.exec() != 0) {
          reference_safe_as_file(*this, get_selected_file(*this, dialog));
        }
      },
      QKeySequence::SaveAs);

  add_menu_action(
      *this, file_menu, make_action("&Export recording", file_menu),
      [this]() {
        auto &dialog = make_file_dialog(
            *this, "Export — Justly", "WAV file (*.wav)",
            QFileDialog::AcceptSave, ".wav", QFileDialog::AnyFile);
        dialog.setLabelText(QFileDialog::Accept, "Export");
        if (dialog.exec() != 0) {
          export_song_to_file(player, song, get_selected_file(*this, dialog));
        }
      },
      QKeySequence::StandardKey());

  menu_bar.addMenu(&file_menu);

  auto &edit_menu = make_menu("&Edit", *this);
  auto &undo_action = get_reference(undo_stack.createUndoAction(&edit_menu));
  undo_action.setShortcuts(QKeySequence::Undo);
  edit_menu.addAction(&undo_action);

  auto &redo_action = get_reference(undo_stack.createRedoAction(&edit_menu));
  redo_action.setShortcuts(QKeySequence::Redo);
  edit_menu.addAction(&redo_action);

  edit_menu.addSeparator();

  add_menu_action(
      *this, edit_menu, cut_action,
      [this]() {
        copy_selection(*this);
        delete_cells(*this);
      },
      QKeySequence::Cut);

  add_menu_action(
      *this, edit_menu, copy_action, [this]() { copy_selection(*this); },
      QKeySequence::Copy);

  auto &paste_menu = make_menu("&Paste", edit_menu);
  edit_menu.addMenu(&paste_menu);

  add_menu_action(
      *this, paste_menu, paste_over_action,
      [this]() {
        switch (current_model_type) {
        case chords_type:
          paste_cells_template(*this, chords_model);
          return;
        case pitched_notes_type:
          paste_cells_template(*this, pitched_notes_model);
          return;
        case unpitched_notes_type:
          paste_cells_template(*this, unpitched_notes_model);
          return;
        default:
          Q_ASSERT(false);
          return;
        }
      },
      QKeySequence::Paste, false);

  add_menu_action(
      *this, paste_menu, paste_into_action,
      [this]() { paste_insert(*this, 0); }, QKeySequence::StandardKey());

  add_menu_action(
      *this, paste_menu, paste_after_action,
      [this]() { paste_insert(*this, get_only_range(*this).bottom() + 1); },
      QKeySequence::StandardKey(), false);

  edit_menu.addSeparator();

  auto &insert_menu = make_menu("&Insert", edit_menu);

  edit_menu.addMenu(&insert_menu);

  add_menu_action(
      *this, insert_menu, insert_after_action,
      [this]() { insert_model_row(*this, get_only_range(*this).bottom() + 1); },
      QKeySequence::InsertLineSeparator, false);

  add_menu_action(
      *this, insert_menu, insert_into_action,
      [this]() { insert_model_row(*this, 0); }, QKeySequence::AddTab);

  add_menu_action(
      *this, edit_menu, delete_action, [this]() { delete_cells(*this); },
      QKeySequence::Delete, false);

  add_menu_action(
      *this, edit_menu, remove_rows_action,
      [this]() {
        const auto &range = get_only_range(*this);
        switch (current_model_type) {
        case chords_type:
          remove_rows_template(range, undo_stack, chords_model);
          return;
        case pitched_notes_type:
          remove_rows_template(range, undo_stack, pitched_notes_model);
          return;
        case unpitched_notes_type:
          remove_rows_template(range, undo_stack, unpitched_notes_model);
          return;
        default:
          Q_ASSERT(false);
          break;
        }
      },
      QKeySequence::DeleteStartOfWord, false);

  edit_menu.addSeparator();

  add_menu_action(
      *this, edit_menu, back_to_chords_action,
      [this]() {
        add_edit_children_or_back(*this, current_chord_number,
                                  current_model_type == pitched_notes_type,
                                  true);
      },
      QKeySequence::Back, false);

  menu_bar.addMenu(&edit_menu);

  auto &view_menu = make_menu("&View", *this);

  auto &view_controls_checkbox = make_action("&Controls", view_menu);

  view_controls_checkbox.setCheckable(true);
  view_controls_checkbox.setChecked(true);
  connect(&view_controls_checkbox, &QAction::toggled, &dock_widget,
          &QWidget::setVisible);
  view_menu.addAction(&view_controls_checkbox);

  menu_bar.addMenu(&view_menu);

  auto &play_menu = make_menu("&Play", *this);

  add_menu_action(
      *this, play_menu, play_action,
      [this]() {
        const auto &range = get_only_range(*this);
        auto first_row_number = range.top();
        auto number_of_rows = get_number_of_rows(range);

        stop_playing(player);
        initialize_play(player, song);

        if (current_model_type == chords_type) {
          modulate_before_chord(player, song, first_row_number);
          play_chords(player, song, first_row_number, number_of_rows);
        } else {
          modulate_before_chord(player, song, current_chord_number);
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
      },
      QKeySequence::Print, false);

  add_menu_action(
      *this, play_menu, stop_playing_action, [this]() { stop_playing(player); },
      QKeySequence::Cancel);

  menu_bar.addMenu(&play_menu);

  auto &controls_form = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QFormLayout(&controls));

  add_control(*this, controls_form, "&Gain:", gain_editor, DEFAULT_GAIN, 0,
              MAX_GAIN, GAIN_STEP, 1, "/10", [this](double new_value) {
                set_double(*this, gain_editor, gain_id,
                           reference_get_gain(*this), new_value);
              });
  add_control(*this, controls_form, "Starting &key:", starting_key_editor,
              song.starting_key, MIN_STARTING_KEY, MAX_STARTING_KEY, 1, 0,
              " hz", [this](double new_value) {
                set_double(*this, starting_key_editor, starting_key_id,
                           song.starting_key, new_value);
              });
  add_control(
      *this, controls_form, "Starting &velocity:", starting_velocity_editor,
      song.starting_velocity, 0, MAX_VELOCITY, 1, 0, "/127",
      [this](double new_value) {
        set_double(*this, starting_velocity_editor, starting_velocity_id,
                   song.starting_velocity, new_value);
      });

  add_control(*this, controls_form, "Starting &tempo:", starting_tempo_editor,
              song.starting_tempo, MIN_STARTING_TEMPO, MAX_STARTING_TEMPO, 1, 0,
              " bpm", [this](double new_value) {
                set_double(*this, starting_tempo_editor, starting_tempo_id,
                           song.starting_tempo, new_value);
              });

  set_model(*this, chords_model);

  connect_model(*this, chords_model);
  connect_model(*this, pitched_notes_model);
  connect_model(*this, unpitched_notes_model);

  dock_widget.setWidget(&controls);
  dock_widget.setFeatures(QDockWidget::NoDockWidgetFeatures);
  addDockWidget(Qt::LeftDockWidgetArea, &dock_widget);

  table_view.setSelectionMode(QAbstractItemView::ContiguousSelection);
  table_view.setSelectionBehavior(QAbstractItemView::SelectItems);
  table_view.setSizeAdjustPolicy(
      QAbstractScrollArea::AdjustToContentsOnFirstShow);

  get_reference(table_view.horizontalHeader())
      .setSectionResizeMode(QHeaderView::ResizeToContents);

  table_view.setMouseTracking(true);

  connect(&table_view, &QAbstractItemView::doubleClicked, this,
          [this](const QModelIndex &index) {
            if (current_model_type == chords_type) {
              auto column = index.column();
              auto is_pitched = column == chord_pitched_notes_column;
              if (is_pitched || (column == chord_unpitched_notes_column)) {
                add_edit_children_or_back(*this, index.row(), is_pitched,
                                          false);
              }
            }
          });

  setWindowTitle("Justly");

  auto &table_column = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QWidget);
  auto &table_column_layout = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QVBoxLayout(&table_column));
  table_column_layout.addWidget(&editing_text);
  table_column_layout.addWidget(&table_view);
  setCentralWidget(&table_column);

  connect(&undo_stack, &QUndoStack::cleanChanged, this, [this]() {
    save_action.setEnabled(!undo_stack.isClean() && !current_file.isEmpty());
  });

  resize(get_reference(QGuiApplication::primaryScreen())
             .availableGeometry()
             .size());

  undo_stack.clear();
  undo_stack.setClean();
}

void SongEditor::closeEvent(QCloseEvent *close_event_pointer) {
  if (!can_discard_changes(*this)) {
    get_reference(close_event_pointer).ignore();
    return;
  }
  QMainWindow::closeEvent(close_event_pointer);
}

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

[[nodiscard]] static auto get_name_or_empty(const Named *named_pointer) {
  if (named_pointer == nullptr) {
    return QString("");
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
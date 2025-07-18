#pragma once

#include <QtCore/QStringListModel>
#include <set>

#include "other/helpers.hpp"
#include "sound/FluidSettings.hpp"
#include "sound/FluidSynth.hpp"

static const auto GENERAL_BANK_NUMBER = 0;
static const auto GENERAL_EXPRESSIVE_BANK_NUMBER = 17;
static const auto EXTRA_BANK_NUMBER = 8;
static const auto EXTRA_EXPRESSIVE_BANK_NUMBER = 18;
static const auto MAX_PITCHED_BANK_NUMBER =
    18; // banks numbers above 18 are duplicates except for detuned saw, special
        // cased below
static const auto TEMPLE_BLOCKS_BANK_NUMBER = 1;
static const auto UNPITCHED_BANK_NUMBER = 128;

[[nodiscard]] static inline auto get_soundfont_id(FluidSynth &synth) {
  const auto soundfont_id =
      fluid_synth_sfload(synth.internal_pointer,
                         get_share_file("MuseScore_General.sf2").c_str(), 1);
  Q_ASSERT(soundfont_id >= 0);
  return soundfont_id;
}

struct Program {
  std::string original_name;
  QString translated_name;
  short bank_number;
  short preset_number;

  Program(const char *const original_name_input, const short bank_number_input,
          const short preset_number_input)
      : original_name(original_name_input),
        translated_name(QObject::tr(original_name_input)),
        bank_number(bank_number_input), preset_number(preset_number_input){};
};

Q_DECLARE_METATYPE(const Program *);

static inline void maybe_add_program_to_xml(xmlNode &node,
                                            const char *const field_name,
                                            const Program *program_pointer) {
  Q_ASSERT(field_name != nullptr);
  if (program_pointer != nullptr) {
    set_xml_string(node, field_name,
                   get_reference(program_pointer).original_name);
  }
}

[[nodiscard]] static inline auto
set_program_from_xml(const QList<Program> &all_programs,
                     xmlNode &node) -> auto & {
  const auto original_name = get_content(node);
  const auto program_pointer =
      std::find_if(all_programs.cbegin(), all_programs.cend(),
                   [original_name](const Program &item) {
                     return item.original_name == original_name;
                   });
  Q_ASSERT(program_pointer != all_programs.end());
  return *program_pointer;
}

[[nodiscard]] static auto filter_programs(const QList<Program> &all_programs,
                                          const bool is_pitched) {
  QList<Program> result;
  std::copy_if(
      all_programs.begin(), all_programs.end(), std::back_inserter(result),
      [is_pitched](const Program &program) {
        const auto bank_number = program.bank_number;
        return ((bank_number != UNPITCHED_BANK_NUMBER &&
                 bank_number != TEMPLE_BLOCKS_BANK_NUMBER) == is_pitched);
      });
  return result;
}

[[nodiscard]] static inline auto
get_some_programs(const bool is_pitched) -> auto & {
  static const auto all_programs = []() {
    FluidSettings settings;
    FluidSynth synth(settings);

    fluid_sfont_t *const soundfont_pointer = fluid_synth_get_sfont_by_id(
        synth.internal_pointer, get_soundfont_id(synth));
    Q_ASSERT(soundfont_pointer != nullptr);

    fluid_sfont_iteration_start(soundfont_pointer);
    auto *preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);

    QList<Program> programs;
    std::set<int> expressive_preset_numbers;
    std::set<int> extra_expressive_preset_numbers;
    while (preset_pointer != nullptr) {
      const auto *const name = fluid_preset_get_name(preset_pointer);
      const auto bank_number =
          static_cast<short>(fluid_preset_get_banknum(preset_pointer));
      const auto preset_number =
          static_cast<short>(fluid_preset_get_num(preset_pointer));
      if (bank_number == GENERAL_EXPRESSIVE_BANK_NUMBER) {
        expressive_preset_numbers.insert(preset_number);
      }
      if (bank_number == EXTRA_EXPRESSIVE_BANK_NUMBER) {
        extra_expressive_preset_numbers.insert(preset_number);
      }
      // detuned saw expr. is the only non-duplicate instrument on a bank above
      // the max pitched bank number
      if (bank_number <= MAX_PITCHED_BANK_NUMBER ||
          bank_number == UNPITCHED_BANK_NUMBER ||
          std::string(name) == "Detuned Saw Expr.") {
        programs.push_back(Program(name, bank_number, preset_number));
      }
      preset_pointer = fluid_sfont_iteration_next(soundfont_pointer);
    }

    programs.erase(
        std::remove_if(
            programs.begin(), programs.end(),
            [&expressive_preset_numbers,
             &extra_expressive_preset_numbers](const auto &program) {
              const auto bank_number = program.bank_number;
              const auto preset_number = program.preset_number;
              return (bank_number == GENERAL_BANK_NUMBER &&
                      expressive_preset_numbers.find(preset_number) !=
                          expressive_preset_numbers.end()) ||
                     (bank_number == EXTRA_BANK_NUMBER &&
                      extra_expressive_preset_numbers.find(preset_number) !=
                          extra_expressive_preset_numbers.end());
            }),
        programs.end());

    std::sort(programs.begin(), programs.end(),
              [](const Program &instrument_1, const Program &instrument_2) {
                return instrument_1.translated_name <
                       instrument_2.translated_name;
              });
    return programs;
  }();
  static const auto pitched_programs = filter_programs(all_programs, true);
  static const auto unpitched_programs = filter_programs(all_programs, false);
  return is_pitched ? pitched_programs : unpitched_programs;
}

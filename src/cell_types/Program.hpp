#pragma once

#include <QtCore/QStringListModel>
#include <fluidsynth.h>
#include <set>

#include "other/helpers.hpp"

static const auto EXPRESSIVE_BANK_NUMBER = 17;
static const auto EXTRA_BANK_NUMBER = 8;
static const auto EXTRA_EXPRESSIVE_BANK_NUMBER = 18;
static const auto MAX_PITCHED_BANK_NUMBER =
    18; // banks numbers above 18 are duplicates except for detuned saw, special
        // cased below
static const auto TEMPLE_BLOCKS_BANK_NUMBER = 1;
static const auto UNPITCHED_BANK_NUMBER = 128;

[[nodiscard]] static inline auto get_soundfont_id(fluid_synth_t &synth) {
  const auto soundfont_id = fluid_synth_sfload(
      &synth, get_share_file("MuseScore_General.sf2").c_str(), 1);
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

static inline void maybe_set_xml_program(xmlNode &node, const char *const field_name,
                                  const Program *program_pointer) {
  Q_ASSERT(field_name != nullptr);
  if (program_pointer != nullptr) {
    set_xml_string(node, field_name,
                   get_reference(program_pointer).original_name);
  }
}

[[nodiscard]] static inline auto
xml_to_program(const QList<Program> &all_programs, xmlNode &node) -> auto& {
  const auto original_name = get_content(node);
  const auto program_pointer =
      std::find_if(all_programs.cbegin(), all_programs.cend(),
                   [original_name](const Program &item) {
                     return item.original_name == original_name;
                   });
  Q_ASSERT(program_pointer != all_programs.end());
  return *program_pointer;
}

[[nodiscard]] static auto get_some_programs(const bool is_pitched) {
  static const auto all_programs = []() {
    auto &settings = get_reference(new_fluid_settings());
    auto &synth = get_reference(new_fluid_synth(&settings));

    fluid_sfont_t *const soundfont_pointer =
        fluid_synth_get_sfont_by_id(&synth, get_soundfont_id(synth));
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
      if (bank_number == EXPRESSIVE_BANK_NUMBER) {
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
              return (bank_number == 0 &&
                      expressive_preset_numbers.find(preset_number) !=
                          expressive_preset_numbers.end()) ||
                     (bank_number == EXTRA_BANK_NUMBER &&
                      extra_expressive_preset_numbers.find(preset_number) !=
                          extra_expressive_preset_numbers.end());
            }),
        programs.end());

    delete_fluid_synth(&synth);
    delete_fluid_settings(&settings);

    std::sort(programs.begin(), programs.end(),
              [](const Program &instrument_1, const Program &instrument_2) {
                return instrument_1.translated_name <
                       instrument_2.translated_name;
              });
    return programs;
  }();
  QList<Program> some_programs;
  for (const auto &program : all_programs) {
    const auto bank_number = program.bank_number;
    if ((bank_number != UNPITCHED_BANK_NUMBER &&
         bank_number != TEMPLE_BLOCKS_BANK_NUMBER) == is_pitched) {
      some_programs.push_back(program);
    }
  }
  return some_programs;
}

[[nodiscard]] static inline auto get_pitched_instruments() -> auto & {
  static const auto pitched_programs = get_some_programs(true);
  return pitched_programs;
}

[[nodiscard]] static inline auto get_percussion_sets() -> auto & {
  static const auto percussion_sets = get_some_programs(false);
  return percussion_sets;
}

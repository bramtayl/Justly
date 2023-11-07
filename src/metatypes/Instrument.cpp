#include "justly/metatypes/Instrument.h"

#include <fast-cpp-csv-parser/csv.h>
#include <qcoreapplication.h>  // for QCoreApplication
#include <qdir.h>              // for QDir
#include <qstring.h>           // for QString

#include <algorithm>  // for any_of
#include <iterator>   // for back_insert_iterator, back_inse...
#include <string>
#include <utility>  // for move

const auto CSV_COLUMNS = 5;

Instrument::Instrument(std::string name_input, int bank_number_input,
                       int preset_number_input, int instrument_id_input)
    : instrument_name(std::move(name_input)),
      bank_number(bank_number_input),
      preset_number(preset_number_input),
      instrument_id(instrument_id_input) {}

auto Instrument::get_all_instruments() -> const std::vector<Instrument> & {
  static const std::vector<Instrument> all_instruments = []() {
    io::CSVReader<CSV_COLUMNS> input(
        QDir(QCoreApplication::applicationDirPath())
            .filePath(INSTRUMENTS_RELATIVE_PATH)
            .toStdString());
    input.read_header(io::ignore_extra_column, "exclude", "instrument_name",
                      "expressive", "bank_number", "preset_number");
    int exclude = 0;
    std::string instrument_name;
    int expressive = 0;
    int bank_number = 0;
    int preset_number = 0;
    int instrument_id = 0;
    std::vector<Instrument> temp_instruments;
    while (input.read_row(exclude, instrument_name, expressive, bank_number,
                          preset_number)) {
      if (exclude == 0 && expressive == 0) {
        temp_instruments.emplace_back(instrument_name, bank_number,
                                      preset_number, instrument_id);
        instrument_id = instrument_id + 1;
      }
    }
    return temp_instruments;
  }();
  return all_instruments;
}

auto Instrument::get_all_instrument_names()
    -> const std::vector<std::string> & {
  static const std::vector<std::string> all_instrument_names = []() {
    std::vector<std::string> temp_names;
    const auto &all_instrument_names = get_all_instruments();
    std::transform(all_instrument_names.cbegin(), all_instrument_names.cend(),
                   std::back_inserter(temp_names),
                   [](const Instrument &instrument) {
                     return instrument.instrument_name;
                   });
    return temp_names;
  }();
  return all_instrument_names;
}

auto Instrument::get_instrument_by_name(const std::string &instrument_name)
    -> const Instrument & {
  if (instrument_name.empty()) {
    return Instrument::get_empty_instrument();
  }
  const auto &instruments = get_all_instruments();
  return *std::find_if(instruments.cbegin(), instruments.cend(),
                       [instrument_name](const Instrument &instrument) {
                         return instrument.instrument_name == instrument_name;
                       });
}

auto Instrument::get_empty_instrument() -> const Instrument & {
  static const auto empty_instrument = Instrument("<default>");
  return empty_instrument;
}

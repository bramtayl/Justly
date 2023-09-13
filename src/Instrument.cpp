#include "Instrument.h"

#include <qcoreapplication.h>  // for QCoreApplication
#include <qdir.h>              // for QDir
#include <qstring.h>           // for QString

#include <algorithm>                  // for transform
#include <iterator>                   // for back_insert_iterator, back_inse...
#include <string>
#include <utility>  // for move

#include "fast-cpp-csv-parser/csv.h"

const auto CSV_COLUMNS = 5;

Instrument::Instrument(QString name_input, int bank_number_input,
                       int preset_number_input, int instrument_id_input)
    : instrument_name(std::move(name_input)),
      bank_number(bank_number_input),
      preset_number(preset_number_input),
      instument_id(instrument_id_input) {}

auto Instrument::get_all_instrument_pointers()
    -> const std::vector<std::unique_ptr<Instrument>> & {
  io::CSVReader<CSV_COLUMNS> input(QDir(QCoreApplication::applicationDirPath())
                                       .filePath("../share/instruments.csv")
                                       .toStdString());
  input.read_header(io::ignore_extra_column, "exclude", "instrument_name",
                    "expressive", "bank_number", "preset_number");
  int exclude = 0;
  std::string instrument_name;
  int expressive = 0;
  int bank_number = 0;
  int preset_number = 0;
  int instrument_id = 0;
  static std::vector<std::unique_ptr<Instrument>> all_instruments_temporary;
  while (input.read_row(exclude, instrument_name, expressive, bank_number,
                        preset_number)) {
    if (exclude == 0 && expressive == 0) {
      all_instruments_temporary.push_back(std::make_unique<Instrument>(
          QString::fromStdString(instrument_name), bank_number, preset_number,
          instrument_id));
      instrument_id = instrument_id + 1;
    }
  }
  static const std::vector<std::unique_ptr<Instrument>> all_instruments = std::move(all_instruments_temporary);
  return all_instruments;
}

auto Instrument::get_all_instrument_names() -> const std::vector<std::string>& {
  std::vector<std::string> all_instrument_names_temporary;
  std::transform(
    get_all_instrument_pointers().begin(),
    get_all_instrument_pointers().end(),
    std::back_inserter(all_instrument_names_temporary),
    [](const std::unique_ptr<Instrument>& instrument_pointer) {
      return (instrument_pointer -> instrument_name).toStdString();
    }
  );
  static const std::vector<std::string> all_instrument_names = 
    all_instrument_names_temporary;
  return all_instrument_names;
}

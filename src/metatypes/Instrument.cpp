#include "metatypes/Instrument.h"

#include <QtCore/qglobal.h>        // for qCritical
#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <qbytearray.h>            // for QByteArray
#include <qcoreapplication.h>      // for QCoreApplication
#include <qdir.h>                  // for QDir
#include <qstring.h>               // for QString

#include <algorithm>  // for any_of
#include <string>
#include <utility>  // for move

#include "fast-cpp-csv-parser/csv.h"

const auto CSV_COLUMNS = 5;

Instrument::Instrument(QString name_input, int bank_number_input,
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
        temp_instruments.emplace_back(QString::fromStdString(instrument_name),
                                      bank_number, preset_number,
                                      instrument_id);
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
    for (const Instrument &instrument : get_all_instruments()) {
      temp_names.push_back(instrument.instrument_name.toStdString());
    };
    return temp_names;
  }();
  return all_instrument_names;
}

auto Instrument::get_instrument_by_name(const QString &instrument_name) -> Instrument {
  if (instrument_name == "") {
    return Instrument();
  }
  const auto &instruments = get_all_instruments();
  for (const auto &instrument : instruments) {
    if (instrument.instrument_name == instrument_name) {
      return instrument;
    }
  }
  qCritical("Cannot find instrument \"%s\"!", qUtf8Printable(instrument_name));
  return Instrument();
}

auto Instrument::instrument_exists(const QString &maybe_instrument) -> bool {
  const auto &instruments = get_all_instruments();
  return std::any_of(instruments.cbegin(), instruments.cend(),
                     [&maybe_instrument](const auto &instrument) {
                       return instrument.instrument_name == maybe_instrument;
                     });
}

auto Instrument::operator==(const Instrument &other_interval) const -> bool {
  return instrument_name == other_interval.instrument_name &&
         bank_number == other_interval.bank_number &&
         preset_number == other_interval.preset_number &&
         instrument_id == other_interval.instrument_id;
}

auto Instrument::get_text() const -> QString {
  return instrument_name;
}


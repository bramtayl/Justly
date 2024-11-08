#include "named/program/instrument/Instrument.hpp"

Instrument::Instrument(const char* name, short bank_number, short preset_number)
    : Program(name, bank_number, preset_number) {}

auto Instrument::get_all_nameds() -> const QList<Instrument> & {
  static const auto all_instruments = get_programs<Instrument>(false);
  return all_instruments;
}

auto Instrument::get_field_name() -> const char* {
  return "instrument";
};
auto Instrument::get_type_name() -> const char* {
  return "instrument";
};
auto Instrument::get_missing_error() -> const char* {
  return "Instrument error";
};
auto Instrument::get_default() -> const char* {
  return "Marimba";
};

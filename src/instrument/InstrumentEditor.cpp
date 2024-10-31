#include "instrument/InstrumentEditor.hpp"

#include <QStringListModel>

#include "named/Named.hpp"

InstrumentEditor::InstrumentEditor(QWidget *parent_pointer)
    : NamedEditor<Instrument>(get_all_instruments(), parent_pointer) {
  static auto instrument_names_model = get_list_model(get_all_instruments());
  set_model(*this, instrument_names_model);
};
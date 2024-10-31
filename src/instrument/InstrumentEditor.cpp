#include "instrument/InstrumentEditor.hpp"

#include <QStringListModel>

#include "named/Named.hpp"

InstrumentEditor::InstrumentEditor(QWidget *parent_pointer_input)
    : NamedEditor<Instrument>(get_all_instruments(), parent_pointer_input) {
  static auto instrument_names_model = get_list_model(get_all_instruments());
  setModel(&instrument_names_model);
};
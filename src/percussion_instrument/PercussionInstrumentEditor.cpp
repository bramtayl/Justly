#include "percussion_instrument/PercussionInstrumentEditor.hpp"

#include <QStringListModel>

#include "named/Named.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"

PercussionInstrumentEditor::PercussionInstrumentEditor(
    QWidget *parent_pointer_input)
    : NamedEditor<PercussionInstrument>(get_all_percussion_instruments(),
                                        parent_pointer_input) {
  static auto percussion_instruments_model =
      get_list_model(get_all_percussion_instruments());
  setModel(&percussion_instruments_model);
};
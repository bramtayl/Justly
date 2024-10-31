#include "percussion_instrument/PercussionInstrumentEditor.hpp"

#include <QStringListModel>

#include "named/Named.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"

PercussionInstrumentEditor::PercussionInstrumentEditor(
    QWidget *parent_pointer)
    : NamedEditor<PercussionInstrument>(get_all_percussion_instruments(),
                                        parent_pointer) {
  static auto percussion_instruments_model =
      get_list_model(get_all_percussion_instruments());
  set_model(*this, percussion_instruments_model);
};
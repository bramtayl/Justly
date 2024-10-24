
#pragma once

#include <QComboBox>
#include <QObject>

#include "other/ComboboxEditor.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"

class QWidget;

struct PercussionInstrumentEditor
    : public ComboboxEditor<PercussionInstrument> {
  Q_OBJECT
  Q_PROPERTY(
      const PercussionInstrument *value READ value WRITE setValue USER true)
public:
  explicit PercussionInstrumentEditor(QWidget *parent_pointer_input = nullptr)
      : ComboboxEditor<PercussionInstrument>(get_all_percussion_instruments(),
                                             parent_pointer_input){};
};


#pragma once

#include <QComboBox>
#include <QObject>

#include "instrument/Instrument.hpp"
#include "named/NamedEditor.hpp"

class QWidget;

struct InstrumentEditor : public NamedEditor<Instrument> {
  Q_OBJECT
  Q_PROPERTY(const Instrument *value READ value WRITE setValue USER true)
public:
  explicit InstrumentEditor(QWidget *parent_pointer_input = nullptr)
      : NamedEditor<Instrument>(get_all_instruments(),
                                   parent_pointer_input){};
};

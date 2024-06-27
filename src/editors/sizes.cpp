#include "editors/sizes.hpp"

#include <qlineedit.h>  // for QLineEdit

#include <memory>  // for make_unique, unique_ptr

#include "editors/InstrumentEditor.hpp"  // for InstrumentEditor
#include "editors/IntervalEditor.hpp"    // for IntervalEditor
#include "editors/RationalEditor.hpp"    // for RationalEditor

auto get_interval_size() -> QSize {
  static auto interval_size = IntervalEditor().sizeHint();
  return interval_size;
};

auto get_rational_size() -> QSize {
  static auto rational_size = RationalEditor().sizeHint();
  return rational_size;
};

auto get_instrument_size() -> QSize {
  static auto instrument_size = InstrumentEditor().sizeHint();
  return instrument_size;
};

auto get_words_size() -> QSize {
  static auto words_size = QLineEdit().sizeHint();
  return words_size;
};
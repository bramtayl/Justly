#include "other/conversions.hpp"
#include <QtGlobal>

auto to_qsizetype(int input) -> qsizetype {
  Q_ASSERT(input >= 0);
  return static_cast<qsizetype>(input);
}

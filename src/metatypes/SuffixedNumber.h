#pragma once

#include <qmetatype.h>  // for qRegisterMetaType, qRegisterNormalizedMetaType
#include <qstring.h>  // for QString

class SuffixedNumber {
 public:
  double number;
  QString suffix;
  explicit SuffixedNumber(double number = 0.0, QString suffix_input = "");
  [[nodiscard]] auto get_text() const -> QString;
  auto operator==(const SuffixedNumber &other_suffixed_number) const -> bool;
};

Q_DECLARE_METATYPE(SuffixedNumber)

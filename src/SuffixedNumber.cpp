#include "SuffixedNumber.h"

#include <utility>  // for move

SuffixedNumber::SuffixedNumber(double number_input, QString suffix_input) : number(number_input), suffix(std::move(suffix_input)) {

}

auto SuffixedNumber::get_text() const  -> QString{
   return QString("%1%2").arg(number).arg(suffix);
}

auto SuffixedNumber::operator==(const SuffixedNumber& other_suffixed_number) const -> bool {
   return number == other_suffixed_number.number && suffix == other_suffixed_number.suffix;
}
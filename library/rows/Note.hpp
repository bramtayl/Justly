#pragma once

#include "cell_types/Rational.hpp"
#include "rows/Row.hpp"

static const auto MAX_VELOCITY = 127;

struct Note {
  QString voice_name;
  Rational beats;
  Rational velocity_ratio;
  QString words;

  // reads one of the fields every note has: beats, velocity_ratio, words, or
  // voice_name
  void note_field_from_xml(const std::string& name, xmlNode& field_node);
};

template <typename SubNote>  // type properties
concept NoteInterface = std::derived_from<SubNote, Note> && requires() {
  { SubNote::get_pitched() } -> std::same_as<const char*>;
};

template <NoteInterface SubNote>
static void add_note_location(QTextStream& stream, const int chord_number,
                              const int note_number) {
  stream << QObject::tr(" for chord ") << chord_number + 1 << QObject::tr(", ")
         << QObject::tr(SubNote::get_pitched()) << QObject::tr(" note ")
         << note_number + 1;
}

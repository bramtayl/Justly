#pragma once

#include <cstdint>

#include "cell_types/Rational.hpp"
#include "rows/Row.hpp"

static const auto MAX_VELOCITY = 127;

// the columns every note has; PitchedNote and UnpitchedNote map their own
// column numbers onto these, and only handle their other columns themselves
enum class NoteField : std::uint8_t { voice_name, beats, velocity_ratio, words };

struct Note {
  QString voice_name;
  Rational beats;
  Rational velocity_ratio;
  QString words;

  [[nodiscard]] static auto get_field_name(NoteField field) -> const char*;

  [[nodiscard]] auto get_field(NoteField field) const -> QVariant;

  void set_field(NoteField field, const QVariant& new_value);

  void field_to_xml(xmlNode& node, NoteField field) const;

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

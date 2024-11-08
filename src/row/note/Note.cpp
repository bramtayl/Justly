#include "row/note/Note.hpp"

#include <nlohmann/json.hpp>

Note::Note(const nlohmann::json &json_note) : Row(json_note) {}

void add_note_location(QTextStream &stream, int chord_number, int note_number,
                       const char *note_type) {
  stream << QObject::tr(" for chord ") << chord_number + 1 << QObject::tr(", ")
         << QObject::tr(note_type) << QObject::tr(" note ") << note_number + 1;
}

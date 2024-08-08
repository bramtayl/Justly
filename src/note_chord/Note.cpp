#include "note_chord/Note.hpp"

#include <nlohmann/json.hpp>

#include "note_chord/NoteChord.hpp"

Note::Note(const nlohmann::json &json_note) : NoteChord(json_note) {}


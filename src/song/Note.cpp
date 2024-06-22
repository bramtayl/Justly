#include "justly/Note.hpp"

#include <nlohmann/json_fwd.hpp>  // for json
#include <string>                 // for allocator, string

#include "justly/NoteChord.hpp"  // for NoteChord

Note::Note(const nlohmann::json& json_note) : NoteChord(json_note) {}

auto Note::symbol() const -> std::string { return "♪"; }
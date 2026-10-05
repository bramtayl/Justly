#pragma once

#include <libxml/tree.h>

#include <QtCore/QList>
#include <optional>

#include "musicxml/MusicXMLPart.hpp"
#include "rows/Chord.hpp"
#include "xml/XMLDocument.hpp"

class QWidget;

// a musicxml score converted to Justly's terms, ready to replace the song
struct ImportedScore {
  // unique and non-empty, with at least one unpitched voice
  VoiceNames voice_names;
  int starting_midi_key = 0;
  // notes name their voices from voice_names
  QList<Chord> chords;
};

// reads a .musicxml file, or the score inside a compressed .mxl file; a null
// document if it can't be read
[[nodiscard]] auto read_musicxml_document(const QString& filename)
    -> XMLDocument;

// warns and returns nullopt if the score has something that can't be imported
[[nodiscard]] auto import_score(QWidget& parent, xmlNode& score_partwise)
    -> std::optional<ImportedScore>;

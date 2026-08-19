#pragma once

#include <QtCore/QList>
#include <QtCore/QtSwap>
#include <utility>

#include "musicxml/MusicXMLNote.hpp"

struct MusicXMLChord {
  QList<MusicXMLNote> pitched_notes;
  QList<MusicXMLNote> unpitched_notes;
};

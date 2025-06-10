#pragma once

#include <QtCore/QList>

#include "MusicXMLNote.hpp"

struct MusicXMLChord {
  QList<MusicXMLNote> pitched_notes;
  QList<MusicXMLNote> unpitched_notes;
};

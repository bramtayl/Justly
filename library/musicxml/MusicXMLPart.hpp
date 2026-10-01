#pragma once

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <libxml/tree.h>
#include <optional>
#include <string>

#include "musicxml/MusicXMLChord.hpp"
#include "musicxml/MusicXMLMeasure.hpp"

class QWidget;

struct MusicXMLPart {
  QString id;
  QString name;
  QMap<std::string, QString> instrument_names;
  QList<MusicXMLMeasure> measures;
  // each keyed by the time of the change
  QMap<int, int> divisions_changes;
  // written, until untransposed
  QMap<int, int> fifths_changes;
  // in halfsteps
  QMap<int, int> transpose_changes;
};

struct VoiceNames {
  QList<QString> pitched;
  QList<QString> unpitched;
};

// the value of the last change at or before time
[[nodiscard]] auto get_most_recent(const QMap<int, int>& changes, int time,
                                   int default_value) -> int;

// the order the measures are played in, as indices, following repeats,
// endings, da capos, dal segnos, codas, and fines
[[nodiscard]] auto get_playback_order(const QList<MusicXMLMeasure>& measures)
    -> QList<int>;

// every part's repeats, endings, and jumps, measure by measure, since jumps
// are often only written in one part. The measures have no notes
[[nodiscard]] auto get_score_measures(const QList<MusicXMLPart>& parts)
    -> QList<MusicXMLMeasure>;

// reads each part as written, in its own divisions; warns and returns nullopt
// if the score has something that can't be imported
[[nodiscard]] auto parse_musicxml(QWidget& parent, xmlNode& score_partwise)
    -> std::optional<QList<MusicXMLPart>>;

// spells each pitched note from its accidental, an earlier accidental in the
// measure, or the key signature
void fill_in_accidentals(MusicXMLPart& part);

// moves the notes and keys of transposing instruments from written to
// sounding pitch
void untranspose(MusicXMLPart& part);

// extends each tie-start note through the notes tied to it, and drops those
void combine_ties(MusicXMLPart& part);

// divisions per beat that every part's divisions fit into
[[nodiscard]] auto get_song_divisions(const QList<MusicXMLPart>& parts) -> int;

// moves times from the part's own divisions, which can change partway
// through, to the song's divisions, which are the same throughout
void normalize_divisions(MusicXMLPart& part, int song_divisions);

// lays the measures out in the order they're played
void unroll_repeats(MusicXMLPart& part, const QList<int>& playback_order);

// each instrument in each part gets its own voice
[[nodiscard]] auto assign_voices(QList<MusicXMLPart>& parts) -> VoiceNames;

// notes that start at the same time, in any part, form a chord
[[nodiscard]] auto get_chords(const QList<MusicXMLPart>& parts)
    -> QMap<int, MusicXMLChord>;

[[nodiscard]] auto get_midi_keys(const QList<MusicXMLPart>& parts)
    -> QMap<int, int>;

[[nodiscard]] auto get_measure_numbers(const QList<MusicXMLPart>& parts)
    -> QMap<int, int>;

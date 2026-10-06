#pragma once

#include <libxml/tree.h>

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <optional>
#include <string>

class QWidget;

// how an accidental alters a step
struct Accidental {
  int chromatic = 0;
  // Johnston septimal quartertones (36/35): -1 for a 7, +1 for an el
  int septimal_quartertones = 0;
};

struct MusicXMLNote {
  // as written in the file
  int start_time = 0;
  int duration = 0;
  bool is_pitched = true;
  // the written step and octave, for pitched notes
  QString step;
  int octave = 0;
  std::optional<Accidental> accidental;
  QString staff = "1";
  std::string instrument_id;
  bool tie_start = false;
  bool tie_stop = false;

  // filled in after parsing
  int midi_number = 0;
  // Johnston septimal quartertones (36/35): -1 for a 7, +1 for an el
  int septimal_quartertones = 0;
  int voice_number = 0;
  QString words;
};

enum class JumpType { da_capo, dal_segno, to_coda, fine };

// a direction to continue somewhere else once a measure ends; a fine
// continues to the end of the score
struct MusicXMLJump {
  JumpType type = JumpType::da_capo;
  // the name of the segno or coda to jump to
  QString target;
  // the times through the measure the jump is taken on. If empty, a da capo
  // or dal segno is taken the first time, and a to coda or fine only after
  // a da capo or dal segno
  QList<int> times;
};

struct MusicXMLMeasure {
  int number = 1;
  int start_time = 0;
  int end_time = 0;
  bool has_forward_repeat = false;
  bool has_backward_repeat = false;
  int repeat_times = 2;
  // the passes through a repeat this measure plays on, if it's in an ending
  QList<int> ending_numbers;
  // the names of the segnos and codas that mark this measure as a target
  QList<QString> segnos;
  QList<QString> codas;
  QList<MusicXMLJump> jumps;
  // rests are left out, but the rest of the notes are in written order
  QList<MusicXMLNote> notes;
};

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

struct MusicXMLChord {
  QList<MusicXMLNote> pitched_notes;
  QList<MusicXMLNote> unpitched_notes;
};

struct VoiceNames {
  QList<QString> pitched;
  QList<QString> unpitched;
};

// every part's notes at sounding pitch, with repeats unrolled and ties
// combined, all timed in song_divisions per beat
struct ParsedScore {
  int song_divisions = 1;
  // notes that start at the same time, in any part, form a chord
  QMap<int, MusicXMLChord> chords;
  // notes name their voices by number from voice_names
  VoiceNames voice_names;
  // each keyed by the time of the change
  QMap<int, int> midi_keys;
  QMap<int, int> measure_numbers;
};

// the value of the last change at or before time
[[nodiscard]] auto get_most_recent(const QMap<int, int>& changes, int time,
                                   int default_value) -> int;

// the order the measures are played in, as indices, following repeats,
// endings, da capos, dal segnos, codas, and fines
[[nodiscard]] auto get_playback_order(const QList<MusicXMLMeasure>& measures)
    -> QList<int>;

// warns and returns nullopt if the score has something that can't be imported
[[nodiscard]] auto parse_score(QWidget& parent, xmlNode& score_partwise)
    -> std::optional<ParsedScore>;

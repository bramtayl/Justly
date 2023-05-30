#include "NoteChord.h"

NoteChord::NoteChord(
    const std::vector<std::unique_ptr<const QString>>& instruments,
    const QString& default_instrument)
    : instruments(instruments),
      default_instrument(default_instrument){

      };



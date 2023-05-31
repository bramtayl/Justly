#include "NoteChord.h"

NoteChord::NoteChord(
    const std::vector<std::unique_ptr<const QString>>& instrument_pointers,
    const QString& default_instrument)
    : instrument_pointers(instrument_pointers),
      default_instrument(default_instrument){

      };



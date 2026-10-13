"""Reads a score with music21 for Justly to import.

usage: python import_score.py SCORE_FILE

Prints a json object to stdout. On success, it has every part's notes at
sounding pitch, with repeats unrolled and ties combined, all timed in
"divisions" per quarter note:

    divisions: int
    voices: {"pitched": [voice], "unpitched": [voice]}, where a voice is
        {"part_name": str, "instrument_name": str, "midi_program": int | null}
    notes: [{"time": int, "duration": int, "is_pitched": bool, "voice": int,
             "midi_number": int, "septimal_quartertones": int}], where voice
        indexes the pitched or unpitched voices, and midi_number and
        septimal_quartertones are only for pitched notes
    midi_keys, measure_numbers, tempos: [[time, value]], each value
        starting at its time. Tempos are in quarter notes per minute

On failure, it has a single "error" with a message to show.
"""

import contextlib
import copy
import io
import json
import math
import sys
from fractions import Fraction

# keeps any warnings music21 prints at import out of the json
with contextlib.redirect_stdout(io.StringIO()):
    from music21 import bar, converter, note, percussion, repeat, stream, tempo
    from music21 import chord as music21_chord
    from music21 import key as music21_key
    from music21 import pitch as music21_pitch

MIDDLE_C_MIDI = 60
MAX_MIDI_NUMBER = 127
FIFTH_HALFSTEPS = 7
HALFSTEPS_PER_OCTAVE = 12
MAX_TIME = 2**31 - 1
# unrolling a repeat copies its measures this many times
MAX_REPEAT_TIMES = 100


class ImportError_(Exception):
    pass


def is_unpitched(element):
    return isinstance(element, (note.Unpitched, percussion.PercussionChord))


def get_sub_notes(element):
    if isinstance(element, music21_chord.ChordBase):
        return element.notes
    return [element]


# the chromatic alteration of each accidental an arrow can be added to
ARROW_ALTERS = {
    "flat-flat": -2,
    "flat": -1,
    "natural": 0,
    "sharp": 1,
    "sharp-sharp": 2,
    "double-sharp": 2,
}


def get_arrow_spelling(accidental_name):
    """The chromatic alteration and the septimal quartertones that an arrow
    accidental marks, or None if it isn't an arrow. Arrows mark Johnston's 7
    (down) and el (up)"""
    for suffix, septimal_quartertones in (("-down", -1), ("-up", 1)):
        if accidental_name.endswith(suffix):
            base = accidental_name.removesuffix(suffix)
            if base in ARROW_ALTERS:
                return ARROW_ALTERS[base], septimal_quartertones
    return None


def spell_arrows(score):
    """Spells each pitched note with an arrow from its accidental, and stores
    its 7 or el in its editorial, while the score is still written, not
    sounding, so they survive transposition. music21 takes pitches from the
    file's alters, which can't hold a 7 or el, so files leave them out or get
    them wrong. An accidental lasts until the end of its measure, for notes on
    the same staff, step, and octave, and a tied note keeps the spelling it's
    tied from. music21 reads a staff as its own part"""
    for part in score.parts:
        # the arrow spelling of each tie in progress
        tied_spellings = {}
        for measure in part.getElementsByClass(stream.Measure):
            in_effect = {}
            for element in measure.recurse().notes:
                if is_unpitched(element):
                    continue
                for sub_note in get_sub_notes(element):
                    pitch = sub_note.pitch
                    written = (pitch.step, pitch.octave)
                    accidental = pitch.accidental
                    tie_type = None if sub_note.tie is None else sub_note.tie.type
                    # only an accidental written in the file, not one music21
                    # infers from the note's alter
                    if accidental is not None and accidental.displayStatus is True:
                        spelling = get_arrow_spelling(accidental.name)
                        in_effect[written] = spelling
                        # only an arrow can make the pitch microtonal
                        if spelling is None and pitch.ps != round(pitch.ps):
                            raise ImportError_(
                                f"Accidental {accidental.name} in measure "
                                f"{measure.number} is not supported"
                            )
                    elif tie_type in ("stop", "continue") and written in tied_spellings:
                        in_effect[written] = tied_spellings[written]
                    spelling = in_effect.get(written)
                    if spelling is None:
                        sub_note.editorial.septimal_quartertones = 0
                    else:
                        alter, septimal_quartertones = spelling
                        pitch.accidental = music21_pitch.Accidental(alter)
                        pitch.accidental.displayStatus = False
                        sub_note.editorial.septimal_quartertones = (
                            septimal_quartertones
                        )
                    if tie_type in ("start", "continue"):
                        tied_spellings[written] = spelling
                    else:
                        tied_spellings.pop(written, None)


def to_fraction(quarter_length):
    return Fraction(quarter_length).limit_denominator(MAX_TIME)


def get_part_numbers(score):
    """Numbers the parts in order. music21 splits a part's staves into
    PartStaffs, with ids like P1-Staff2, which belong to the part before"""
    part_numbers = []
    for part in score.parts:
        is_later_staff = isinstance(part, stream.PartStaff) and not str(
            part.id
        ).endswith("-Staff1")
        if is_later_staff and part_numbers:
            part_numbers.append(part_numbers[-1])
        else:
            part_numbers.append(len(set(part_numbers)))
    return part_numbers


def share_jumps(score):
    """Jumps, and the segnos, codas, and fines they go to, are often only
    written in one part, but every part follows them"""
    measure_lists = [
        list(part.getElementsByClass(stream.Measure)) for part in score.parts
    ]
    for measures in measure_lists:
        for index, measure in enumerate(measures):
            for mark in list(measure.getElementsByClass(repeat.RepeatExpression)):
                for other_measures in measure_lists:
                    if other_measures is measures or index >= len(other_measures):
                        continue
                    other_measure = other_measures[index]
                    if not any(
                        type(other_mark) is type(mark)
                        for other_mark in other_measure.getElementsByClass(
                            repeat.RepeatExpression
                        )
                    ):
                        other_measure.insert(mark.offset, copy.deepcopy(mark))


def get_instrument(part, element):
    # unpitched notes can each name their own instrument
    stored = getattr(element, "storedInstrument", None)
    if stored is not None:
        return stored
    return part.getInstrument(returnDefault=False)


def read_score(filename):
    score = converter.parseFile(filename, format="musicxml", forceSource=True)
    if not isinstance(score, stream.Score):
        raise ImportError_("The file doesn't have a score")
    for repeat_barline in score.recurse().getElementsByClass(bar.Repeat):
        if repeat_barline.times is not None and repeat_barline.times > MAX_REPEAT_TIMES:
            measure = repeat_barline.getContextByClass(stream.Measure)
            raise ImportError_(
                f"The repeat in measure {measure.number} plays more than "
                f"{MAX_REPEAT_TIMES} times"
            )
    spell_arrows(score)
    share_jumps(score)
    # music21 renumbers measures when it unrolls jumps
    for measure in score.recurse().getElementsByClass(stream.Measure):
        measure.editorial.written_number = measure.number
    score = score.toSoundingPitch()
    try:
        score = score.expandRepeats()
    except repeat.ExpanderException:
        raise ImportError_(
            "The score's repeats, endings, or jumps can't be followed"
        ) from None
    return score


def get_most_recent(changes, time):
    """The value of the last change at or before time, or None"""
    earlier_times = [change_time for change_time in changes if change_time <= time]
    return changes[max(earlier_times)] if earlier_times else None


def get_tied_notes(part):
    """The part's notes, with each tied note combined into the note it's
    tied from. Ties are followed after unrolling, so e.g. a tie into both
    endings stops in each. music21 can combine the wrong notes when a measure
    has more than one voice, so each voice is combined separately"""
    for voice_part in part.voicesToParts().parts:
        yield from voice_part.stripTies(matchByPitch=True).flatten().notes


def convert(filename):
    score = read_score(filename)

    notes = []
    voices = {True: [], False: []}
    voice_numbers = {True: {}, False: {}}
    changes = {"midi_keys": {}, "measure_numbers": {}, "tempos": {}}
    # each pitched part's keys, keyed by the time of the change
    part_keys = []

    for part, part_number in zip(score.parts, get_part_numbers(score)):
        for measure in part.getElementsByClass(stream.Measure):
            changes["measure_numbers"][to_fraction(measure.offset)] = (
                measure.editorial.get("written_number", measure.number)
            )
        flat_part = part.flatten()
        # unpitched parts' keys don't matter
        if any(not is_unpitched(element) for element in flat_part.notes):
            part_keys.append(
                {
                    to_fraction(key_signature.offset): MIDDLE_C_MIDI
                    + (FIFTH_HALFSTEPS * key_signature.sharps) % HALFSTEPS_PER_OCTAVE
                    for key_signature in flat_part.getElementsByClass(
                        music21_key.KeySignature
                    )
                }
            )
        for metronome_mark in flat_part.getElementsByClass(tempo.MetronomeMark):
            quarters_per_minute = metronome_mark.getQuarterBPM()
            if quarters_per_minute is not None and round(quarters_per_minute) > 0:
                changes["tempos"][to_fraction(metronome_mark.offset)] = round(
                    quarters_per_minute
                )

        for element in get_tied_notes(part):
            duration = to_fraction(element.quarterLength)
            # e.g. grace notes
            if duration == 0:
                continue
            is_pitched = not is_unpitched(element)
            instrument = get_instrument(part, element)
            instrument_name = (
                "" if instrument is None else instrument.instrumentName or ""
            )
            # each instrument in each part gets its own voice
            voice_key = (part_number, instrument_name)
            some_voice_numbers = voice_numbers[is_pitched]
            if voice_key not in some_voice_numbers:
                some_voice_numbers[voice_key] = len(voices[is_pitched])
                voices[is_pitched].append(
                    {
                        "part_name": part.partName or "",
                        "instrument_name": instrument_name,
                        "midi_program": None
                        if instrument is None
                        else instrument.midiProgram,
                    }
                )
            for sub_note in get_sub_notes(element):
                new_note = {
                    "time": to_fraction(element.offset),
                    "duration": duration,
                    "is_pitched": is_pitched,
                    "voice": some_voice_numbers[voice_key],
                }
                if is_pitched:
                    midi_number = round(sub_note.pitch.ps)
                    if not 0 <= midi_number <= MAX_MIDI_NUMBER:
                        raise ImportError_(
                            f"A note in measure {element.measureNumber} is "
                            "outside the midi range"
                        )
                    new_note["midi_number"] = midi_number
                    new_note["septimal_quartertones"] = sub_note.editorial.get(
                        "septimal_quartertones", 0
                    )
                notes.append(new_note)

    # Justly has one key at a time, so every part, and every staff of a part,
    # has to be in the same key whenever any of them changes
    for time in sorted({time for keys in part_keys for time in keys}):
        midi_keys = {get_most_recent(keys, time) for keys in part_keys}
        if len(midi_keys) > 1:
            raise ImportError_(
                f"The key signatures in measure "
                f"{get_most_recent(changes['measure_numbers'], time)} don't match"
            )
        changes["midi_keys"][time] = midi_keys.pop()

    # the fewest divisions per quarter note that every time fits into
    times = [new_note[field] for new_note in notes for field in ("time", "duration")]
    for some_changes in changes.values():
        times.extend(some_changes)
    divisions = math.lcm(1, *(time.denominator for time in times))

    def to_divisions(time):
        result = time * divisions
        if result > MAX_TIME:
            raise ImportError_("The score is too long, or its rhythms too fine")
        return int(result)

    for new_note in notes:
        new_note["time"] = to_divisions(new_note["time"])
        new_note["duration"] = to_divisions(new_note["duration"])
    result = {
        "divisions": divisions,
        "voices": {"pitched": voices[True], "unpitched": voices[False]},
        "notes": notes,
    }
    for name, some_changes in changes.items():
        result[name] = [
            [to_divisions(time), value] for time, value in sorted(some_changes.items())
        ]
    return result


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    # music21 can print warnings, which would corrupt the json
    with contextlib.redirect_stdout(io.StringIO()):
        try:
            result = convert(sys.argv[1])
        except ImportError_ as error:
            result = {"error": str(error)}
        except Exception as error:  # pylint: disable=broad-exception-caught
            result = {"error": f"music21 can't read the score: {error}"}
    json.dump(result, sys.stdout)


if __name__ == "__main__":
    main()

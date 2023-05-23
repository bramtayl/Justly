using Base.Iterators: peel, rest
using IterTools: groupby
using JSON3: JSON3
using LightXML: attribute, child_elements, content, name, parse_file, root
using DataStructures: SortedDict

# MusicXML terminology:
# Harmony: a tonal center without an octave
# Key: a tonal center with an octave
# Chord: a group of notes that begin simultaneously
# Note that this differs from the Justly terminology
# where a Chord is a tonal center without an octave
# and Harmony doesn't mean anything

const HALFSTEPS = 12

const DEFAULT_SCALE = [
    1 // 1,
    16 // 15, # down perfect 5, down major 3
    9 // 8, # up perfect 5, up perfect 5
    6 // 5, # minor 3
    5 // 4, # major 3
    4 // 3, # down perfect 5
    45 // 32, # up perfect 5, up perfect 5, up major 3?
    3 // 2, # perfect 5
    8 // 5, # down perfect 5, up minor 3
    5 // 3, # down perect 5, up major 3
    9 // 5, # up fifth, up minor third
    15 // 8, # up perfect 5, up major 3
]

const NOTE_TO_DEGREE =
    Dict("C" => 0, "D" => 2, "E" => 4, "F" => 5, "G" => 7, "A" => 9, "B" => 11)

struct Harmony
    degree::Int
    scale::Vector{Rational{Int}}
end

struct Note
    beats::Int
    midi::Int
end

struct Chord
    measure_number::String
    notes::Vector{Note}
end

struct Interval
    numerator::Int
    denominator::Int
    octave::Int
end

function Interval(; numerator = 1, denominator = 1, octave = 0)
    Interval(numerator, denominator, octave)
end

function Chord(measure_number)
    Chord(measure_number, Note[])
end

function get_beats(element)
    parse(Int, content(only(element["duration"])))
end

function get_degree(steps, alters)
    unaltered = NOTE_TO_DEGREE[content(only(steps))]
    return if isempty(alters)
        unaltered
    else
        unaltered + parse(Int, content(only(alters)))
    end
end

function add_json_note!(json_notes, key, harmony_kind, note)
    interval = get_interval(harmony_kind, note.midi - key)
    push!(
        json_notes,
        Dict{String,Any}(
            "numerator" => interval.numerator,
            "denominator" => interval.denominator,
            "octave" => interval.octave,
            "beats" => note.beats,
            "words" => "",
            "volume_ratio" => 1.0,
            "tempo_ratio" => 1.0,
            "instrument" => DEFAULT_INSTRUMENT,
        ),
    )
    return nothing
end

function add_json_chord!(json_chords, interval, beats, words, json_notes)
    push!(
        json_chords,
        Dict{String,Any}(
            "numerator" => interval.numerator,
            "denominator" => interval.denominator,
            "octave" => interval.octave,
            "beats" => beats,
            "words" => words,
            "volume_ratio" => 1.0,
            "tempo_ratio" => 1.0,
            "children" => json_notes,
        ),
    )
    return nothing
end

function get_interval(harmony_kind, midi_interval)
    octave, relative = fldmod(midi_interval, HALFSTEPS)
    interval = DEFAULT_SCALE[relative+1]
    return Interval(interval.num, interval.den, octave)
end


function get_new_key_notes(harmony, chord)
    json_notes = Any[]
    first_note, other_notes = peel(chord.notes)
    first_midi = first_note.midi
    key = fld(first_midi, HALFSTEPS) + harmony.degree
    harmony_kind = harmony.kind
    add_json_note!(json_notes, key, harmony_kind, first_note)
    for note in other_notes
        add_json_note!(json_notes, key, harmony_kind, note)
    end
    return key, json_notes
end

function measure_string(measure_number)
    return "measure $measure_number"
end

# no previous measure number or previous key
# find a new key
function add_first_chord_in_song!(json_chords, harmony, chord, beats)
    (key, json_notes) = get_new_key_notes(harmony, chord)
    measure_number = chord.measure_number
    add_json_chord!(
        json_chords,
        Interval(),
        beats,
        measure_string(measure_number),
        json_notes,
    )
    return (measure_number, key)
end

# there's a previous measure number and previous key
# find a new key
function add_first_chord_in_harmony!(
    json_chords,
    previous_measure_number,
    previous_key,
    harmony,
    chord,
    beats,
)
    (key, json_notes) = get_new_key_notes(harmony, chord)
    measure_number = chord.measure_number
    add_json_chord!(
        json_chords,
        get_interval(harmony.kind, key - previous_key),
        beats,
        if previous_measure_number != measure_number
            measure_string(measure_number)
        else
            ""
        end,
        json_notes,
    )
    return (measure_number, key)
end

# there's a previous measure number and previous key
# no key change
function add_another_chord!(
    json_chords,
    previous_measure_number,
    key,
    harmony,
    chord,
    beats,
)
    harmony_kind = harmony.kind
    json_notes = Any[]
    for note in chord.notes
        add_json_note!(json_notes, key, harmony_kind, note)
    end

    measure_number = chord.measure_number
    add_json_chord!(
        json_chords,
        Interval(),
        beats,
        if previous_measure_number != measure_number
            measure_string(measure_number)
        else
            ""
        end,
        json_notes,
    )
    return measure_number
end

# keep going until the next harmony
function add_other_chords_in_harmony!(
    json_chords,
    previous_measure_number,
    key,
    harmony,
    next_harmony_start_time,
    chords,
    next_chord,
    next_chord_start_time,
    chord_state,
)
    chord_iteration = iterate(chords, chord_state)
    if chord_iteration === nothing
        # we know there's a next harmony, but no more chords to go with it
        error("Chord without notes!")
    end
    (chord, chord_start_time) = (next_chord, next_chord_start_time)
    ((next_chord_start_time, next_chord), chord_state) = chord_iteration
    while next_chord_start_time < next_harmony_start_time
        previous_measure_number = add_another_chord!(
            json_chords,
            previous_measure_number,
            key,
            harmony,
            chord,
            # go through the end of the chord
            next_chord_start_time - chord_start_time,
        )
        chord_iteration = iterate(chords, chord_state)
        if chord_iteration === nothing
            # we know there's a next harmony, but no more chords to go with it
            error("Chord without notes!")
        end
        (chord, chord_start_time) = (next_chord, next_chord_start_time)
        ((next_chord_start_time, next_chord), chord_state) = chord_iteration
    end

    previous_measure_number = add_another_chord!(
        json_chords,
        previous_measure_number,
        key,
        harmony,
        chord,
        # go through the end of the harmony
        next_harmony_start_time - chord_start_time,
    )
    return (previous_measure_number, next_chord_start_time, next_chord, chord_state)
end

# keep going until song ends
function add_other_chords_in_song!(
    json_chords,
    previous_measure_number,
    key,
    harmony,
    song_end_time,
    chords,
    next_chord,
    next_chord_start_time,
    chord_state,
)
    (chord, chord_start_time) = (next_chord, next_chord_start_time)
    for (next_chord_start_time, next_chord) in rest(chords, chord_state)
        previous_measure_number = add_another_chord!(
            json_chords,
            previous_measure_number,
            key,
            harmony,
            chord,
            # go through the end of the chord
            next_chord_start_time - chord_start_time,
        )
        (chord, chord_start_time) = (next_chord, next_chord_start_time)
    end
    previous_measure_number = add_another_chord!(
        json_chords,
        previous_measure_number,
        key,
        harmony,
        chord,
        # go through the end of the song
        song_end_time - chord_start_time,
    )
    return previous_measure_number
end

function write_justly(file; tempo = 350, volume_percent = 50)
    json_chords = Any[]

    chords = SortedDict{Int,Chord}()
    holdovers = Dict{Int,Tuple{Int,Note}}()
    harmonies = SortedDict{Int,Harmony}()
    previous_beats = nothing

    # TODO: instrument for each part?
    song_end_time = 0
    for part in root(parse_file(file))["part"]
        time = 1
        for measure in part["measure"]
            measure_number = attribute(measure, "number")
            for element in child_elements(measure)
                element_name = name(element)
                if element_name == "note"
                    if !isempty(element["chord"])
                        time = time - previous_beats
                    end
                    if isempty(element["rest"])
                        beats = get_beats(element)
                        song_end_time = max(song_end_time, time + beats)
                        pitch = only(element["pitch"])
                        midi =
                            (parse(Int, content(only(pitch["octave"]))) + 1) * HALFSTEPS +
                            get_degree(pitch["step"], pitch["alter"])
                        note = Note(beats, midi)
                        ties = element["tie"]
                        if isempty(ties)
                            push!((
                                get!(chords, time) do
                                    Chord(measure_number)
                                end
                            ).notes, note)
                        else
                            is_start = false
                            is_stop = false
                            for tie in ties
                                tie_type = attribute(tie, "type")
                                if tie_type == "start"
                                    is_start = true
                                elseif tie_type == "stop"
                                    is_stop = true
                                else
                                    error("Unrecognized tie type $tie_type!")
                                end
                            end
                            if is_stop
                                if is_start
                                    (start_time, holdover) = holdovers[midi]
                                    holdovers[midi] =
                                        (start_time, Note(holdover.beats + beats, midi))
                                else
                                    (start_time, holdover) = pop!(holdovers, midi)
                                    combined = Note(holdover.beats + beats, midi)
                                    if haskey(chords, start_time)
                                        push!(chords[start_time].notes, combined)
                                    else
                                        chords[start_time] =
                                            Chord(measure_number, [combined])
                                    end
                                end
                            else
                                if is_start
                                    if haskey(holdovers, midi)
                                        error("Overwriting tie!")
                                    end
                                    holdovers[midi] = (time, note)
                                else
                                    error("Unreachable!")
                                end
                            end
                        end
                        time = time + beats
                        previous_beats = beats
                    else
                        # is its a rest, just skip
                        time = time + get_beats(element)
                    end
                elseif element_name == "backup"
                    time = time - get_beats(element)
                elseif element_name == "harmony"
                    base = only(element["root"])
                    harmonies[time] = Harmony(
                        get_degree(base["root-step"], base["root-alter"]),
                        content(only(element["kind"])),
                    )
                end
            end
        end
    end

    if !isempty(holdovers)
        error("Unresolved ties: $holdovers")
    end

    chord_iteration = iterate(chords)
    if chord_iteration === nothing
        error("No notes!")
    end
    ((chord_start_time, chord), chord_state) = chord_iteration

    harmony_iteration = iterate(harmonies)
    if harmony_iteration === nothing
        # we know there's notes alraedy
        error("Notes without a chord!")
    end
    ((harmony_start_time, harmony), harmony_state) = harmony_iteration

    if chord_start_time < harmony_start_time
        # notes before the first harmony starts
        error("Notes without a chord!")
    end
    json_chords = Any[]

    harmony_iteration = iterate(harmonies, harmony_state)
    if harmony_iteration === nothing
        # only one harmony, handle it here
        chord_iteration = iterate(chords, chord_state)
        if chord_iteration === nothing
            # only one chord
            (previous_measure_number, key) = add_first_chord_in_song!(
                json_chords,
                harmony,
                chord,
                # go through the end of the song
                song_end_time - chord_start_time,
            )
        else
            # multiple chords
            ((next_chord_start_time, next_chord), chord_state) = chord_iteration
            (previous_measure_number, key) = add_first_chord_in_song!(
                json_chords,
                harmony,
                chord,
                # go through the end of the chord
                next_chord_start_time - chord_start_time,
            )
            previous_measure_number = add_other_chords_in_song!(
                json_chords,
                previous_measure_number,
                key,
                harmony,
                song_end_time,
                chords,
                next_chord,
                next_chord_start_time,
                chord_state,
            )
        end
        # start key is the first key
        start_key = key
    else
        ((next_harmony_start_time, next_harmony), harmony_state) = harmony_iteration
        if chord_start_time >= next_harmony_start_time
            # next chord belongs to next harmony, so this harmony has no chords
            error("Chord without notes!")
        end

        chord_iteration = iterate(chords, chord_state)
        if chord_iteration === nothing
            # there's another harmony, but no more chords to go with it
            error("Chord without notes!")
        end
        ((next_chord_start_time, next_chord), chord_state) = chord_iteration

        # handle the first harmony
        if next_chord_start_time < next_harmony_start_time
            # multiple chords in harmony
            (previous_measure_number, key) = add_first_chord_in_song!(
                json_chords,
                harmony,
                chord,
                # go through the end of the chord
                next_chord_start_time - chord_start_time,
            )
            (previous_measure_number, next_chord_start_time, next_chord, chord_state) =
                add_other_chords_in_harmony!(
                    json_chords,
                    previous_measure_number,
                    key,
                    harmony,
                    next_harmony_start_time,
                    chords,
                    next_chord,
                    next_chord_start_time,
                    chord_state,
                )
        else
            # only one chord in the harmony
            (previous_measure_number, key) = add_first_chord_in_song!(
                json_chords,
                harmony,
                chord,
                # go through the end of the harmony
                next_harmony_start_time - chord_start_time,
            )
        end
        # start key is the first key
        start_key = key

        previous_key = key
        harmony = next_harmony
        (chord, chord_start_time) = (next_chord, next_chord_start_time)
        for (next_harmony_start_time, next_harmony) in rest(harmonies, harmony_state)
            # handle all but last harmony

            if chord_start_time >= next_harmony_start_time
                # next chord belongs to next harmony, so this harmony has no chords
                error("Chord without notes!")
            end

            chord_iteration = iterate(chords, chord_state)
            if chord_iteration === nothing
                # there's another harmony, but no more chords to go with it
                error("Chord without notes!")
            end
            ((next_chord_start_time, next_chord), chord_state) = chord_iteration

            if next_chord_start_time < next_harmony_start_time
                # multiple chords
                (previous_measure_number, key) = add_first_chord_in_harmony!(
                    json_chords,
                    previous_measure_number,
                    previous_key,
                    harmony,
                    chord,
                    # go through the end of the chord
                    next_chord_start_time - chord_start_time,
                )

                (previous_measure_number, next_chord_start_time, next_chord, chord_state) =
                    add_other_chords_in_harmony!(
                        json_chords,
                        previous_measure_number,
                        key,
                        harmony,
                        next_harmony_start_time,
                        chords,
                        next_chord,
                        next_chord_start_time,
                        chord_state,
                    )
            else
                # only one chord in harmony
                (previous_measure_number, key) = add_first_chord_in_harmony!(
                    json_chords,
                    previous_measure_number,
                    previous_key,
                    harmony,
                    chord,
                    # go through the end of the harmony
                    next_harmony_start_time - chord_start_time,
                )
            end

            previous_key = key
            harmony = next_harmony
            (chord, chord_start_time) = (next_chord, next_chord_start_time)
        end

        # handle the last harmony
        chord_iteration = iterate(chords, chord_state)
        if chord_iteration === nothing
            # only one chord in harmony
            (previous_measure_number, key) = add_first_chord_in_harmony!(
                json_chords,
                previous_measure_number,
                previous_key,
                harmony,
                chord,
                song_end_time - chord_start_time,
            )
        else
            # multiple chords in harmony
            ((next_chord_start_time, next_chord), chord_state) = chord_iteration
            (previous_measure_number, key) = add_first_chord_in_harmony!(
                json_chords,
                previous_measure_number,
                previous_key,
                harmony,
                chord,
                # go through the end of the chord
                next_chord_start_time - chord_start_time,
            )
            previous_measure_number = add_other_chords_in_song!(
                json_chords,
                previous_measure_number,
                key,
                harmony,
                song_end_time,
                chords,
                next_chord,
                next_chord_start_time,
                chord_state,
            )
        end
    end

    return Dict{String,Any}(
        "frequency" => round(Int, 440 * 2.0^((start_key - 69) / 12)),
        "tempo" => tempo,
        "volume_percent" => volume_percent,
        "children" => json_chords,
    )
end

cd("/home/brandon/Documents")
file = "Prelude_I.musicxml"
open("Prelude_I.json", "w") do io
    JSON3.write(io, write_justly("Prelude_I.musicxml"))
    println(io)
end

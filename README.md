# Justly

[![codecov](https://codecov.io/github/bramtayl/Justly/branch/master/graph/badge.svg?token=MUNbRKjHpZ)](https://codecov.io/github/bramtayl/Justly/tree/master)

> [!IMPORTANT]
> Requires dependencies on Linux. To install, run `sudo apt install fluidsynth libfluidsynth-dev qt6-base-dev libqt6svg6-dev qt6-gtk-platformtheme libxml2-dev`.

> [!IMPORTANT]
> Requires dependencies on MacOS. To install, run `brew install fluid-synth qt libxml2`.

## Installation

You can download binaries for Justly [here](https://github.com/bramtayl/Justly/releases/latest).
The Justly executable is in the "bin" subfolder.

## Motivation

You can use Justly to both compose and play music using any pitches you want.
Using staff notation, you can only write the pitched notes of the 12-tone scale.
Some intervals in any 12-tone scale are close to harmonic, but other intervals are not.
Johnston [expanded staff notation](http://marsbat.space/pdfs/EJItext.pdf), but relying on staff notation limited him.

## Notation

### Ratios

In Justly, you write ratios as a a rational fraction (integer / integer).
Justly will omit the numerator if it is 1, and omit the denominator if it is 1.
Therefore:

- "" represents the ratio 1
- "/2" represents the ratio $\frac{1}{2}$

### Intervals

In Justly, you write intervals as a rational fraction (integer / integer) times a power of 2.
An "o" suffix is a short hand for "\*2^", similar to how the "e" suffix is shorthand for "*10^".

Justly will omit the numerator if it is 1, the denominator if it is 1, and the octave if it is 0.
Therefore:

- "" represents the interval 1
- "/3" represents the interval $\frac{1}{3}$
- "o1" represents the interval $2^1$
- "/3o1" represents the interval $\frac{1}{3}*2^1$

You will likely only need to know 4 "prime" intervals.

- Octave: 2 = o1
- Perfect fifth: 3/2
- Major third: 5/4
- Harmonic seventh: 7.4

Note that the numerators of these intervals are the first 4 prime numbers.

To go up by an interval, multiply by the interval.
So to go up by a fifth, multiply by 3/2.

To go down instead of up, divide by the interval.
So to go down by a fifth, divide by 3/2 = multiply by 2/3.

You can create new intervals by multiplying and dividing intervals.
For example, a minor third is up a perfect fifth and down a major third: 3/2 * 4/5 = 6/5.

Here are some useful composite intervals:

- Major second: 9/8 = perfect fifth * perfect fifth / octave
- Minor third: 6/5 = perfect fifth / major third
- Perfect fourth: 4/3 = octave / perfect fifth
- Minor sixth: 8/5 = octave / major third
- Major sixth: 5/3 = octave / perfect fifth * major third
- Minor seventh: 9/5 = perfect fifth * perfect fifth / major third
- Major seventh: 15/8 = perfect fifth * major third

I suggest using a [rational calculator](https://www.symbolab.com/solver/rational-expression-calculator) to multiply and divide intervals.

### Gain vs. velocity

In Justly, there are two kinds of volume: "gain", which is the speaker volume, and "velocity", the force with which Justly plays a note. You can adjust the gain of the whole song, or the velocity of different notes.

### Starting values

- "Gain" is the gain, between 0 and 10.
- "Starting key" is the starting key, in Hz. For reference, see the [piano key frequencies on Wikipedia](https://en.wikipedia.org/wiki/Piano_key_frequencies).
- "Starting velocity" is the starting velocity, between 0 and 127.
- "Starting tempo" is the starting tempo, in beats per minute (bpm).

### Instruments

You can use any of the instruments included with [MuseScore soundfont](https://ftp.osuosl.org/pub/musescore/soundfont/MuseScore_General/).

Percussion instruments are nested into "percussion sets". Each percussion set has a name and a set of sounds associated with the MIDI numbers 0-127. Some of these MIDI numbers will have no sounds associated with them, and the sound associated with a particular number will vary. The sounds roughly correspond to the [Roland MIDI standard](https://www.voidaudio.net/percussion.html).

### Chords, pitched notes, and unpitched notes

A chord is a set of pitched and unpitched notes that begin playing simulataneously.

Chords have the following fields, each corresponding to a column:

- "Pitched notes": the number of pitched notes in the chord.
- "Unpitched notes": the number of unpitched notes in the chord.
- "Instrument": If not empty, Justly changes the default instrument for pitched notes to this (see below).
- "Percussion instrument": If not empty, Justly changes the default percussion instrument for unpitched notes to this (see below).
- "Interval": Justly multiplies the current key by this ratio. 
- "Beats": The number of beats until the next chord starts
- "Velocity ratio": Justly multiplies the current velocity by this ratio. 
- "Tempo ratio": Justly multiplies the current tempo by this ratio.
- "Words": text associated with the chord

Both pitched and unpitched notes both have the following fields:

- "Beats": When Justly starts the chord, Justly will play the note for this number of beats.
- "Velocity ratio": Justly sets the note velocity to the current velocity times this ratio.
- "Words": text associated with the note.

Pitched notes have the following additional fields.

- "Instrument": The instrument of the pitched note. If empty, Justly will use the default instrument for the chord (see above).
- "Interval": Justly sets the note's pitch to this interval times the current key.

Likewise, unpitched notes have the following additional fields.

- "Percussion instrument": The percussion instrument of the unpitched note. If empty, Justly will use the default percussion instrument for the chord (see above).

## Interface

### Controls

You can edit the gain, starting key, starting velocity, and starting tempo using the controls on the left. There are also buttons for raising or lowering the interval of selected chords (see below).

### Table editor

Each row of the table represents a unit: a chord, pitched note, or unpitched note.

You can select a single cell by clicking on it.
Hold shift to select multiple cells.
When you select a chord or pitched note cell, Justly will show corresponding frequency and approximate piano key in the status bar at the bottom.

To edit the pitched notes of a chord, double click its "Pitched notes" cell. 
To edit the unpitched notes of a chord, double click its "Unpitched notes" cell.
To go back to the chords, select "Back to chords" from the "Edit" menu (see below).

### File Menu

In the "File" menu, you can choose among the following options:

- "Open" to open a previously saved song.
- "Import MusicXML" to import an uncompressed MusicXML file (see below). 
- "Save" to save the song in the previous location.
- "Save As" to save the song in a new location.
- "Export recording" to export a recording of the song as a wav file.

### Edit Menu

In the "Edit" menu, you can choose among the following options:

- "Undo" to undo any action.
- "Redo" to redo any action.
- "Cut" to cut a selected group of cells.
- "Copy" to copy a selected group of cells.
- "Paste" to choose among the following:
  - "Over" to paste over the selected cells.
  - "Into start" to paste rows into the start of the table.
  - "After" to paste rows after the selected cells.
- "Insert" to choose among the following:
  - "After" to insert a row after the selected cells.
  - "Into start" to insert a row into the start of the table.
- "Delete cells" to delete cell contents.
- "Remove rows" to remove selected rows.

### View Menu

In the "View" menu, you can choose among the following options:

- "Back to chords" to view the chords.
- "Previous chord" to view the pitched or unpitched notes of the previous chord.
- "Next chord" to view the pitched or unpitched notes of the next chord.

### Play Menu

In the play menu, you can choose among the following options:

- "Play selection" to play a selection of chords or notes. If you play a selection of chords, you will skip any previous chords, and only play the selected chords. If you play a selection of notes within a chord, you will skip any previous chords, and only play the selected notes within the current chord.
- "Stop Playing" to stop playing.

## Import

Justly can import sheet music written in MusicXML.
To do so, Justly uses a few heuristics.

Justly uses the current key signature to find the tonic. Just then uses the following scale relative to the tonic:

- Minor second: 16/5
- Major second: 9/8
- Minor third: 6/5
- Major third: 5/4
- Perfect fourth: 4/3
- Augmented fourth/dimished fifth: 45/32
- Perfect fifth: 3/2
- Minor sixth: 8/5
- Major sixth: 5/3
- Minor seventh: 9/5
- Perfect octave: 2

For example, in the key of A, the frequency of any E (a perfect fifth away from the tonic) will be 3/2 of the frequency of the tonic below it.

When the key signature changes, Justly modulates by the interval of the new key in the old key signature. So, if you modulate by from the key of A to the key of E, Justly will modulate by an interval of 3/2 (plus or minus an octave).

Here are some tips for importing music written in standard notation:

- Change key signatures every time the underlying chord in the music changes.
- You might need to manually adjust seventh intervals. In the context of a seventh chord, you might want to change the seventh of the chord to be a harmonic seventh above the tonic. Likewise, in the context of a half-diminished seventh chord, you might want to change the root note of the chord to be a harmonic seventh below the top note of the chord. Be careful, however, because harmonic seventh intervals are notically different from minor seventh intervals.

## Example

This example is the [simple.json](examples/simple.json) file in the examples folder.

Here is screenshot of the chords in the song:

![chords screenshot](examples/chords.png)

The song starts with a key of frequency 220Hz, that is, A3.

In the first chord, Justly sets the default instrument to Grand Piano.
The key does not change in the first chord.

After 1 beat, the key changes: Justly divides the key by $\frac{3}{2}$, so the key goes down by a fifth. Now, the key is close to D4.

After 1 more beat, Justly multiplies the key by $\frac{3}{2}$, so the key goes up by a fifth. Now, the key is back to A3.

Here is a screenshot of the pitched notes in the first chord:

![chord 1 pitched notes screenshot](examples/chord_1_pitched_notes.png)

The pitched notes in the first chord are the tonic (≈A3), third (≈C#4), and fifth (≈E4).

Here is a screenshot of the pitched notes in the second chord:

![chord 2 pitched notes screenshot](examples/chord_2_pitched_notes.png)

The pitched notes in the second chord are the fifth (≈A3), up 1 octave (≈D4), and up 1 octave and a third (≈F#4). 

Here is a screenshot of the pitched notes in the third chord:

![chord 3 pitched notes screenshot](examples/chord_3_pitched_notes.png)

The pitched notes in the third chord are the tonic (≈A3), third (≈C#4), and fifth (≈E4).

# Justly

[![codecov](https://codecov.io/github/bramtayl/Justly/branch/master/graph/badge.svg?token=MUNbRKjHpZ)](https://codecov.io/github/bramtayl/Justly/tree/master)

> [!IMPORTANT]  
> Requires dependencies on Linux. To install, run `sudo apt install fluidsynth qt6-base-dev qt6-gtk-platformtheme qt6-wayland`.

> [!IMPORTANT]  
> Requires dependencies on MacOS. To install, run `brew install fluid-synth qt`.

## Installation

You can download binaries for Justly [here](https://github.com/bramtayl/Justly/releases/latest).
The Justly executable is in the "bin" subfolder.

## Motivation

You can use Justly to both compose and play music using any pitches you want.
Using staff notation, you can only write the notes of the 12-tone scale.
Some intervals in any 12-tone scale are close to harmonic, but other intervals are not.
Johnston [expanded staff notation](http://marsbat.space/pdfs/EJItext.pdf), but relying on staff notation limited him.

## Notation

### Ratios

In Justly, you write ratios as a a rational fraction (integer / integer).
The numerator will be omitted if it is one, and the denominator will be omitted if it is one. Therefore:

- "" represents the ratio 1
- "/2" represents the ratio $\frac{1}{2}$

### Intervals

In Justly, you write intervals as a rational fraction (integer / integer) times a power of 2.
An "o" suffix is a short hand for "\*2^", similar to how the "e" suffix is shorthand for "*10^".

The numerator will be omitted if it is one, the denominator will be omitted if it is one, and the octave will be omitted if it is 0. Therefore:

- "" represents the interval 1
- "/3" represents the interval $\frac{1}{3}$
- "o1" represents the interval $2^1$
- "/3o1" represents the interval $\frac{1}{3}*2^1$

You will likely only need to know 4 "prime" intervals.

- Octave: 2
- Perfect fifth: $\frac{3}{2}$
- Major third: $\frac{5}{4}$
- Harmonic seventh: $\frac{7}{4}$

Note that the numerators of these fractions are the first 4 prime numbers.

To go up by an interval, multiply by the interval.
So to go up by a fifth, multiply by $\frac{3}{2}$.

To go down instead of up, divide by the interval.
So to go down by a fifth, divide by $\frac{3}{2}$ = multiply by $\frac{2}{3}$.

You can write the same interval in multiple ways.
For example, you can write a fifth as $\frac{3}{2}$, or "3o-1" = $\frac{3}{1}*2^{-1}$.

You can create new intervals by multiplying and dividing intervals.
For example, a minor third is up a perfect fifth and down a major third: $\frac{3}{2} \div \frac{5}{4} = \frac{6}{5}$.

Here are some useful composite intervals:

- Major second: $\frac{9}{8}$
- Minor third: $\frac{6}{5}$
- Perfect fourth: $\frac{4}{3}$
- Minor sixth: $\frac{8}{5}$
- Major sixth: $\frac{5}{3}$
- Minor seventh: $\frac{9}{5}$
- Major seventh: $\frac{15}{8}$

### Gain vs. velocity

In Justly, there are two kinds of volume: "gain", which is the speaker volume, and "velocity", the force with which an note is played. You can adjust the gain of the whole song, or the velocity of different notes.

### Starting values

- "Gain" is the gain, between 0 and 10.
- "Starting key" is the starting key, in Hz. For reference, see the [piano key frequencies on Wikipedia](https://en.wikipedia.org/wiki/Piano_key_frequencies).
- "Starting velocity" is the starting velocity, between 0 and 127.
- "Starting tempo" is the starting tempo, in beats per minute (bpm).

### Instruments

You can use any of the instruments included with [MuseScore soundfont](https://ftp.osuosl.org/pub/musescore/soundfont/MuseScore_General/).

### Chords, Notes, and Percussions

In Justly, there are three units: "chords", "notes", and "percussions".
A chord is a set of notes that begin playing simulataneously.

Chords have the following fields, each corresponding to a column:

- "Interval": The interval of a chord is the modulation of the current key. Changing the key of a chord changes the key of all future chords.
- "Beats": The beats of a chord is the number of beats until the next chord starts.
- "Velocity ratio": The velocity ratio of a chord is the modulation of the current velocity. Changing the velocity ratio of a chord changes the velocity of all future chords.
- "Tempo ratio": the tempo ratio. The tempo ratio of a chord is the modulation of the tempo. Changing the tempo ratio of a chord changes the tempo of all future chords.
- "Words": text associated with the chord.
- "Notes": the number of notes
- "Percussions": the number of percussions

Notes have the following fields, each corresponding to a column:

- "Instrument": The instrument of the note.
- "Beats": Once the chord starts, each note in the chord will play for its number of beats.
- "Velocity ratio": The velocity ratio of a note is its velocity ratio relative to the current velocity. Changing the velocity ratio of a note does not change the current velocity.
- "Tempo ratio": The tempo ratio of a note is its tempo ratio relative to the current tempo. Thus, notes with higher tempo ratios will sound more stacatto Changing the tempo ratio of a note does not change the current tempo.
- "Words": text associated with the note.

Percussions have the following fields, each corresponding to a column:

- "Set": A set of percussion instruments. Think of a percussion set like a drum kit.
- "Instrument": The instrument of the percsussions.
- "Beats": Once the chord starts, each percussion in the chord will play for its number of beats.
- "Velocity ratio": The velocity ratio of a percussion is its velocity ratio relative to the current velocity. Changing the velocity ratio of a percussion does not change the current velocity.
- "Tempo ratio": The tempo ratio of a percussion is its tempo ratio relative to the current tempo. Thus, percussion with higher tempo ratios will sound more stacatto . Changing the tempo ratio of a percussion does not change the current tempo.

## Interface

### Controls

You can edit the gain, starting key, starting velocity, and starting tempo using the controls on the left.

### Table editor

There is a table of chords, and for each chord, there is a table of notes and a table of percussions. Each row of a table represents a unit: a chord, note, or percussion.

You can select a single cell by clicking on it.
Hold shift to select multiple cells.
When you select a chord or note cell, you will see the frequency and approximate piano key of the chord or note in the status bar at the bottom.

To view the notes of a chord, double click the "Notes" cell. 
To view the percussions of a chord, double click the "Percussions" cell.
To go back to the chords table, select "Back to chords" from the "Edit" menu.

### File Menu

Using the file menu, you can choose among the following:

- "Open" to open a previously saved song.
- "Save" to save a song in a previous location.
- "Save As" to save the song in a new location.
- "Export recording" to export a recording of the song as a wav file.

### Edit Menu

Using the edit menu, you can choose among following:

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
- "Delete" to delete cell contents.
- "Remove rows" to remove selected rows.

### View Menu

- Check/uncheck "Controls" to view/hide the starting controls, respectively.

### Play Menu

Using the play menu, you can choose among following:

- "Play selection" to play a selection of chords or notes. If you play a selection of chords, you will skip the previous chords, and only play the selected chords. If you play a selection of notes within a chord, you will skip all previous chords, and only play the selected notes within the chord.
- "Stop Playing" to stop playing a previous request.

## Example

This example is the [simple.json](examples/simple.json) file in the examples folder.

Here is screenshot of the chords in the song:

![chords screenshot](examples/chords.png)

The song starts with a key of frequency 220Hz, that is, A3.
The key does not change in the first chord.
After 1 beat, the key changes: you divide the key by $\frac{3}{2}$, so the key goes down by a fifth. Then, the key is close to D4.
After 1 more beat, you multiply the key by $\frac{3}{2}$, so the key goes up by a fifth. Then, the key is back to A3.

Here is a screenshot of the notes in the first chord:

![chord 1 notes screenshot](examples/chord_1_notes.png)

The notes in the first chord are the tonic (≈A3), third (≈C#4), and fifth (≈E4).

Here is a screenshot of the notes in the second chord:

![chord 2 notes screenshot](examples/chord_2_notes.png)

The notes in the second chord are the fifth (≈A3), up one octave (≈D4), and up one octave and a third (≈F#4). 

Here is a screenshot of the notes in the third chord:

![chord 3 notes screenshot](examples/chord_3_notes.png)

The notes in the third chord are the tonic (≈A3), third (≈C#4), and fifth (≈E4).

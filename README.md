# Justly

## Motivation

You can use Justly to both compose and play music using any pitches you want.
Using staff notation, you can only write the notes of the 12-tone scale.
Some intervals in any 12-tone scale are close to harmonic, but other intervals are not.
Johnston [expanded staff notation](http://marsbat.space/pdfs/EJItext.pdf), but relying on staff notation limited him.

## Intervals

In Justly, you write intervals as a rational fraction (integer / integer) times a power of 2.
An `o` suffix as a short hand for `*2^`, similar to how the `e` suffix is shorthand for `*10^`.
You can write the same ratio in multiple ways.
For example, you can write a fifth as `3/2`, or `3/2o1`.

You will likely only need to know 4 "prime" intervals.

- Octave: `2`
- Perfect fifth: `3/2`
- Major third: `5/4`
- Harmonic seventh: `7/4`

Note that the numerators of these fractions are the first 4 prime numbers.

To go down instead of up, flip the fraction.
So to go down by a fifth, multiply by `2/3`.

You can create other intervals by multiplying and dividing these intervals.
For example, a minor third is up a perfect fifth and down a major third: `(3/2) / (5/4)` = `6/5`.

Useful composite intervals:

- Major second: `9/8`
- Minor third: `6/5`
- Perfect fourth: `4/3`
- Minor sixth: `8/5`
- Major sixth: `5/3`
- Minor seventh: `9/5`
- Major seventh: `15/8`

## Orchestra

You can use any of the instruments included with [MuseScore](https://musescore.org/en/instruments).

## Controls

You can edit the starting key, starting volume, starting tempo, and starting instrument using the controls at the top.

- `Starting key` is the starting key, in Hz.
- `Starting volume` is the starting volume, between 0 and 100%. To avoid peaking, lower the volume for songs with many voices.
- `Starting tempo` is the starting tempo, in beats per minute. These beats are indivisible, so for songs which subdivide beats, you will need to multiply the tempo accordingly.
- `Starting instrument` is the starting instrument. 

## Chords vs. Notes

In Justly, there are "chords" and "notes".
A chord is a set of "notes" that will begin playing simulataneously.
A chord modulates the song, while a note does not, in the following sense.
The interval, volume ratio, tempo ratio, and instrument changes in chords are cumulative, and will affect all future chords.
So for example, if you set the tempo ratio for a chord to `2.0`, you will double the tempo of that chord and all future chords.
The interval, volume ratio, tempo ratio, and instrument in a note are in reference to the chord, but only affect the note itself.
So for example, if you set the tempo ratio for a note to `2.0`, you will double the tempo of that note only (that is, you will make the note stacatto).

## Actions

There are several actions available from the menu, with shortcuts listed.
Some actions are only enabled after you select items.
You can select just chords, or just notes, but not a combination.
You can choose whether or not to view the controls and the chords.

## Example 1: Harmony

Here is screenshot of the song [examples/simple.json](examples/simple.json) in the examples folder.

![simple.json screenshot](examples/simple.png)

Cells with their default value have gray text, and cells with their non-default value have black text.

This song starts with a key of frequency 220Hz, that is, A3.
The key does not change in the first chord.
The three voices in the first chord play the tonic (≈A3), third (≈C#4), and fifth (≈E4).
All three voices play for `1` beat.

After 1 beat, the key changes: you divide the key by `3/2`, so the key goes down by a fifth.
Now the key is close to D4.
The three voices play the fifth (≈A3), up one octave (≈D4), and up one octave and a third (≈F#4). 

After 1 more beat, you multiply the key by `3/2`, so the key goes up by a fifth. The voices repeat the items in the first chord, but play for `2` beats.

## Example 2: Melody

Here is screenshot of the song [examples/well_tempered_clavier.json](examples/well_tempered_clavier.json) in the examples folder.

![well_tempered_clavier.json screenshot](examples/well_tempered_clavier.png)

This is the start of Bach's Well-Tempered Clavier (BMV 846). Ironically, Bach used this work to evangelize equal temperament (the arch-enemy of Justly). Here is the sheet music for reference:

![Start of BMV 846](examples/well_tempered_clavier_sheet_music.png)

The whole figure is in the key of 262 Hz (≈ middle C). 

In Justly, beats are indivisible, so for this song, 1 beat in Justly represents 1 sixteenth note.

All of the "chords" have a ratio of 1 because the key never changes.

Each note starts at a different time. Because a chord represents a set of notes that begin playing simultaneously, in this song, each note has its own "chord". 

Each "chord" lasts for 1 beat. The first note, however, plays for 8 beats.
1 beat into the first note, the second note starts, and plays for 7 beats.
The rest of the notes play for 1 beat. At the end of all 8 "chords", the first two notes sop playing.

## Build instructions

I'm struggling to build binaries for Justly, due to the complexity of packaging both Qt and CSound with cmake.
Contributions are greatly appreciated.
In the meantime, here are build instructions that I tested on Linux.

You will need `git` to download the code, `cmake` to build it, and the following dependencies:

- The CSound binary and headers
- The CSound C++ wrapper binary and headers.
- Qt base binaries and headers.

`cmake` must be able to find all of these things.

On Ubuntu, running this script should compile Justly for you.

```
# install build tools
sudo apt install cmake git

# install dependencies
sudo apt install libcsound64-dev libcsnd-dev qt6-base-dev

# download
git clone https://github.com/bramtayl/Justly.jl.git
cd Justly

# set up
mkdir build
cmake -S . -B build

# build
cmake --build build --config Release --target Justly

# install
sudo cmake --install build --config Release
```

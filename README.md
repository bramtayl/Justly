# Justly

## Motivation

You can use Justly to both compose and play music using any pitches you want.
Using staff notation, you can only write the notes of the 12-tone scale.
Some intervals in any 12-tone scale are close to harmonic, but other intervals are not.
Johnston [expanded staff notation](http://marsbat.space/pdfs/EJItext.pdf), but relying on staff notation limited him.

## Song files

You can pass 1 command line argument to Justly: a song file to open.
If you don't pass any arguments, Justly will prompt you to open or create a song file.
Justly will save your song when you close the editor.

## Intervals

In Justly, you write intervals as a rational fraction (integer / integer) times a power of 2.
You can write the same ratio in multiple ways.
For example, you can write a fifth as `3/2`, or `3*2^-1`.

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

## Top sliders

You can edit the starting frequency, starting volume, and starting tempo using the sliders on the top.

- `Starting frequency` is the starting frequency, in Hz.
- `Starting volume` is the starting volume, between 0 and 100%. To avoid peaking, lower the volume for songs with many voices.
- `Starting tempo` is the starting tempo, in beats per minute. These beats are indivisible, so for songs which subdivide beats, you will need to multiply the tempo accordingly.

## Chords vs. Notes

In Justly, there are "chords" and "notes".
A chord is a set of "notes" that will begin playing simulataneously.
A chord modulates the song, while a note does not, in the following sense.
The interval, volume ratio, and tempo ratio changes in chords are cumulative, and will affect all future chords.
So for example, if you set the tempo ratio for a chord to `2.0`, you will double the tempo of that chord and all future chords.
The interval, volume ratio, and tempo ratio in a note are in reference to the chord, but only affect the note itself.
So for example, if you set the tempo ratio for a note to `2.0`, you will double the tempo of that note only (that is, you will make the note stacatto).

## Instruments

You can change the instrument of notes, but not chords.
Currently, Justly can play the instrument_pointers from the CSound [STK plugin](https://csound.com/docs/manual/STKTop.html), namely:

- BandedWG
- BeeThree
- BlowBotl
- BlowHole
- Bowed
- Brass
- Clarinet
- Drummer
- Flute
- FMVoices
- HevyMetl
- Mandolin
- ModalBar
- Moog
- PercFlut
- Plucked
- Resonate
- Rhodey
- Saxofony
- Shakers
- Simple
- Sitar
- StifKarp
- TubeBell
- VoicForm
- Whistle
- Wurley

You must use one of these exact names.

## Controls

There are several controls available from the menu, with shortcuts listed.
Some controls are only enabled after you select items.
You can select just chords, or just notes, but not a combination.

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

Each "chord" lasts for 1 beat. The first note, however, plays for 8 beats. 1 beat into the first note, the second note starts, and plays for 7 beats. The rest of the notes play for 1 beat. At the end of all 8 "chords", the first two notes stop_playing playing.

## Build instructions

### Setup

```
sudo apt install cmake git lcov libcsound64-dev qt6-base-dev 
git clone https://github.com/bramtayl/Justly.jl.git
cd Justly
cmake -B build
```

### Build (Linux)

```
cmake --build build --config Release --target Justly
```

### Test

```
cmake --build build --config Release --target RunTests
cd build
ctest -C Release --output-on-failure
cd ..
lcov --capture --directory build --output-file coverage/lcov.info
```



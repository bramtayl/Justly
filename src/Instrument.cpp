#include "Instrument.h"

#include <qstring.h>  // for QString
#include <qiodevicebase.h>  // for QIODeviceBase::OpenMode, QIODevice...
#include <qtextstream.h>

#include <utility>  // for move

const auto PRESETS_PER_BANK = 128;

Instrument::Instrument(QString name_input, QString code_input,
                       int bank_number_input, int preset_number_input, int id_input)
    : name(std::move(name_input)),
      code(std::move(code_input)),
      bank_number(bank_number_input),
      preset_number(preset_number_input),
      id(id_input) {}

auto Instrument::get_all_instruments() -> std::vector<Instrument>& {
  static std::vector<Instrument> all_instruments = {
    Instrument("12-String Guitar", "twelve_string_guitar", 8, 25, 0),
    // Instrument("5th Saw Wave Expressive", "fifth_saw_wave_expr", 17, 86, 1),
    Instrument("5th Saw Wave", "fifth_saw_wave", 0, 86, 2),
    Instrument("808 Tom", "eight_hundred_eight_tom", 8, 118, 3),
    // Instrument("Accordion Expressive", "accordion_expr", 17, 21, 4),
    Instrument("Accordion", "accordion", 0, 21, 5),
    Instrument("Acoustic Bass", "acoustic_bass", 0, 32, 6),
    Instrument("Agogo", "agogo", 0, 113, 7),
    // Instrument("Alto Sax Expressive", "alto_sax_expr", 17, 65, 8),
    Instrument("Alto Sax", "alto_sax", 0, 65, 9),
    Instrument("Applause", "applause", 0, 126, 10),
    // Instrument("Atmosphere Expressive", "atmosphere_expr", 17, 99, 11),
    Instrument("Atmosphere", "atmosphere", 0, 99, 12),
    Instrument("Bagpipe", "bagpipe", 0, 109, 13),
    // Instrument("Bandoneon Expressive", "bandoneon_expr", 17, 23, 14),
    Instrument("Bandoneon", "bandoneon", 0, 23, 15),
    Instrument("Banjo", "banjo", 0, 105, 16),
    // Instrument("Baritone Sax Expressive", "bariton_sax_expr", 17, 17, 17),
    Instrument("Baritone Sax", "baritone_sax", 0, 67, 18),
    // Instrument("Bass & Lead Expressive", "bass_lead_expr", 17, 87, 19),
    Instrument("Bass & Lead", "bass_and_lead", 0, 87, 20),
    // Instrument("Basses Fast Expressive", "basses_fast_expr", 51, 49, 21),
    // Instrument("Basses Fast", "basses_fast", 50, 48, 22),
    // Instrument("Basses Pizzicato", "basses_pizzicato", 50, 45, 23),
    // Instrument("Basses Slow Expressive", "basses_slow_expr", 51, 49, 24),
    // Instrument("Basses Slow", "basses_slow", 50, 49, 25),
    // Instrument("Basses Trem Expressive", "basses_trem_expr", 51, 44, 26),
    // Instrument("Basses Tremolo", "basses_tremolo", 50, 44, 27),
    // Instrument("Bassoon Expressive", "bassoon_expr", 17, 70, 28),
    Instrument("Bassoon", "bassoon", 0, 70, 29),
    Instrument("Bird Tweet", "bird_tweet", 0, 123, 30),
    // Instrument("Bottle Chiff Expressive", "bottle_chiff_expr", 17, 76, 31),
    Instrument("Bottle Chiff", "bottle_chiff", 0, 76, 32),
    // Instrument("Bowed Glass Expressive", "bowed_glass_expr", 17, 92, 33),
    Instrument("Bowed Glass", "bowed_glass", 0, 92, 34),
    // Instrument("Brass 2 Expressive", "brass_2_expr", 18, 61, 35),
    Instrument("Brass 2", "brass_2", 8, 61, 36),
    // Instrument("Brass Section Expressive", "brass_section_expr", 17, 61, 37),
    Instrument("Brass Section", "brass_section", 0, 61, 38),
    Instrument("Breath Noise", "breath_noise", 0, 121, 39),
    Instrument("Bright Grand Piano", "bright_grand_piano", 0, 1, 40),
    Instrument("Brightness", "brightness", 0, 100, 41),
    Instrument("Brush 1", "brush_1", 128, 41, 42),
    Instrument("Brush 2", "brush_2", 128, 42, 43),
    Instrument("Brush", "brush", 128, 40, 44),
    // Instrument("Calliope Expressive", "calliope_expr", 17, 82, 45),
    Instrument("Calliope Lead", "calliope_lead", 0, 82, 46),
    Instrument("Castanets", "castanets", 8, 115, 47),
    Instrument("Celesta", "celesta", 0, 8, 48),
    // Instrument("Celli Fast Expressive", "celli_fast_expr", 41, 48, 49),
    // Instrument("Celli Fast", "celli_fast", 40, 48, 50),
    // Instrument("Celli Pizzicato", "celli_pizzicato", 40, 45, 51),
    // Instrument("Celli Slow Expressive", "celli_slow_expr", 41, 49, 52),
    // Instrument("Celli Slow", "celli_slow", 40, 49, 53),
    // Instrument("Celli Trem Expressive", "celli_trem_expr", 41, 44, 54),
    // Instrument("Celli Tremolo", "celli_tremolo", 40, 44, 55),
    // Instrument("Cello Expressive", "cello_expr", 17, 42, 56),
    Instrument("Cello", "cello", 0, 42, 57),
    // Instrument("Charang Expressive", "charang_expr", 17, 84, 58),
    Instrument("Charang", "charang", 0, 84, 59),
    // Instrument("Chiffer Lead Expressive", "chiffer_expr", 17, 83, 60),
    Instrument("Chiffer Lead", "chiffer_lead", 0, 83, 61),
    // Instrument("Choir Aahs Expressive", "choir_aahs_expr", 17, 52, 62),
    Instrument("Choir Aahs", "choir_aahs", 0, 52, 63),
    Instrument("Church Bell", "church_bell", 8, 14, 64),
    // Instrument("Church Organ 2 Expressive", "church_org_2_expr", 18, 19, 65),
    Instrument("Church Organ 2", "church_organ_2", 8, 19, 66),
    // Instrument("Church Organ Expressive", "church_organ_expr", 17, 19, 67),
    Instrument("Church Organ", "church_organ", 0, 19, 68),
    // Instrument("Clarinet Expressive", "clarinet_expr", 17, 71, 69),
    Instrument("Clarinet", "clarinet", 0, 71, 70),
    Instrument("Clavinet", "clavinet", 0, 7, 71),
    Instrument("Clean Guitar", "clean_guitar", 0, 27, 72),
    Instrument("Concert Bass Drum", "concert_bass_drum", 8, 116, 73),
    // Instrument("Contrabass Expressive", "contrabass_expr", 17, 43, 74),
    Instrument("Contrabass", "contrabass", 0, 43, 75),
    Instrument("Coupled Harpsichord", "coupled_harpischord", 8, 6, 76),
    Instrument("Crystal", "crystal", 0, 98, 77),
    Instrument("Detuned FM EP", "detuned_fm_ep", 8, 5, 78),
    // Instrument("Detuned Org. 1 Expressive", "detuned_org_1_expr", 18, 16, 79),
    // Instrument("Detuned Org. 2 Expressive", "detuned_org_2_expr", 18, 17, 80),
    Instrument("Detuned Organ 1", "detuned_organ_1", 8, 16, 81),
    Instrument("Detuned Organ 2", "detuned_organ_2", 8, 17, 82),
    // Instrument("Detuned Saw Expressive", "detuned_saw_expr", 21, 81, 83),
    Instrument("Detuned Saw", "detuned_saw", 20, 81, 84),
    Instrument("Detuned Tine EP", "detuned_tine_ep", 8, 4, 85),
    Instrument("Distortion Guitar", "distortion_guitar", 0, 30, 86),
    // Instrument("Drawback Organ Expressive", "drawback_organ_expr", 17, 16, 87),
    Instrument("Drawbar Organ", "drawbar_organ", 0, 16, 88),
    Instrument("Dulcimer", "dulcimer", 0, 15, 89),
    // Instrument("Echo Drops Expressive", "echo_drops_expr", 17, 102, 90),
    Instrument("Echo Drops", "echo_drops", 0, 102, 91),
    Instrument("Electric Grand Piano", "electric_grand_piano", 0, 2, 92),
    Instrument("Electronic", "electronic", 128, 24, 93),
    // Instrument("English Horn Expressive", "english_horn_expr", 17, 69, 94),
    Instrument("English Horn", "english_horn", 0, 69, 95),
    Instrument("FM Electric Piano", "fm_electric_piano", 0, 5, 96),
    Instrument("Fantasia", "fantasia", 0, 88, 97),
    Instrument("Feedback Guitar", "feedback_guitar", 8, 30, 98),
    // Instrument("Fiddle Expressive", "fiddle_expr", 17, 110, 99),
    Instrument("Fiddle", "fiddle", 0, 110, 100),
    Instrument("Fingered Bass", "fingered_bass", 0, 33, 101),
    // Instrument("Flute Expressive", "flute_expr", 17, 73, 102),
    Instrument("Flute", "flute", 0, 73, 103),
    // Instrument("French Horns Expressive", "french_horns_expr", 17, 60, 104),
    Instrument("French Horns", "french_horns", 0, 60, 105),
    Instrument("Fret Noise", "fret_noise", 0, 120, 106),
    Instrument("Fretless Bass", "fretless_bass", 0, 35, 107),
    Instrument("Funk Guitar", "funk_guitar", 8, 28, 108),
    Instrument("Glockenspiel", "glockenspiel", 0, 9, 109),
    // Instrument("Goblin Expressive", "goblin_expr", 17, 101, 110),
    Instrument("Goblin", "goblin", 0, 101, 111),
    Instrument("Grand Piano", "grand_piano", 0, 0, 112),
    // Instrument("Guitar Feedback", "guitar_feedback", 8, 31, 113),
    Instrument("Guitar Harmonics", "guitar_harmonics", 0, 31, 114),
    // Instrument("Halo Pad Expressive", "halo_pad_expr", 17, 94, 115),
    Instrument("Halo Pad", "halo_pad", 0, 94, 116),
    Instrument("Harmon Mute Trumpet", "harmon_mute_trumpet", 0, 59, 117),
    // Instrument("Harmonica Expressive", "harmonica_expr", 17, 22, 118),
    Instrument("Harmonica", "harmonica", 0, 22, 119),
    Instrument("Harp", "harp", 0, 46, 120),
    Instrument("Harpsichord", "harpsichord", 0, 6, 121),
    Instrument("Hawaiian Guitar", "hawaiian_guitar", 8, 26, 122),
    Instrument("Helicopter", "helicopter", 0, 125, 123),
    // Instrument("Hmn. Mute Tpt. Expressive", "hmn_mute_tpt_expr", 17, 59, 124),
    Instrument("Honky-Tonk Piano", "honky_tonk_piano", 0, 3, 125),
    Instrument("Ice Rain", "ice_rain", 0, 96, 126),
    // Instrument("It. Accordion Expressive", "it_accordion_expr", 18, 21, 127),
    Instrument("Italian Accordion", "italian_accordion", 8, 21, 128),
    Instrument("Jazz 1", "jazz_1", 128, 33, 129),
    Instrument("Jazz 2", "jazz_2", 128, 34, 130),
    Instrument("Jazz 3", "jazz_3", 128, 35, 131),
    Instrument("Jazz 4", "jazz_4", 128, 36, 132),
    Instrument("Jazz Guitar", "jazz_guitar", 0, 26, 133),
    Instrument("Jazz", "jazz", 128, 32, 134),
    Instrument("Kalimba", "kalimba", 0, 108, 135),
    Instrument("Koto", "koto", 0, 107, 136),
    Instrument("Mandolin", "mandolin", 16, 25, 137),
    // Instrument("Marching Bass", "marching_bass", 128, 59, 138),
    // Instrument("Marching Cymbals", "marching_cymbals", 128, 58, 139),
    Instrument("Marching Snare", "marching_snare", 128, 56, 140),
    // Instrument("Marching Tenor", "marching_tenor", 128, 96, 141),
    Instrument("Marimba", "marimba", 0, 12, 142),
    Instrument("Melo Tom 2", "melo_tom_2", 8, 117, 143),
    Instrument("Melodic Tom", "melodic_tom", 0, 117, 144),
    // Instrument("Metal Pad Expressive", "metal_pad_expr", 17, 93, 145),
    Instrument("Metal Pad", "metal_pad", 0, 93, 146),
    Instrument("Music Box", "music_box", 0, 10, 147),
    Instrument("Nylon String Guitar", "nylon_string_guitar", 0, 24, 148),
    // Instrument("Oboe Expressive", "oboe_expr", 17, 68, 149),
    Instrument("Oboe", "oboe", 0, 68, 150),
    // Instrument("Ocarina Expressive", "ocarina_expr", 17, 79, 151),
    Instrument("Ocarina", "ocarina", 0, 79, 152),
    // Instrument("Old Marching Tenor", "old_marching_tenor", 128, 95, 153),
    // Instrument("Old Marching Bass", "old_marching_bass", 128, 57, 154),
    Instrument("Orchestra Hit", "orchestra_hit", 0, 55, 155),
    Instrument("Orchestra Kit", "orhcestra_kit", 128, 48, 156),
    Instrument("Orchestra Pad", "orchestra_pad", 8, 48, 157),
    Instrument("Overdrive Guitar", "overdrive_guitar", 0, 29, 158),
    Instrument("Palm Muted Guitar", "palm_muted_guitar", 0, 28, 159),
    // Instrument("Pan Flute Expressive", "pan_flute_expr", 17, 75, 160),
    Instrument("Pan Flute", "pan_flute", 0, 75, 161),
    // Instrument("Perc. Organ Expressive", "perc_organ_expr", 17, 17, 162),
    Instrument("Percussive Organ", "percussive_organ", 0, 17, 163),
    // Instrument("Piccolo Expressive", "piccolo_expr", 17, 72, 164),
    Instrument("Piccolo", "piccolo", 0, 72, 165),
    Instrument("Picked Bass", "picked_bass", 0, 34, 166),
    // Instrument("Polysynth Expressive", "polysynth_expr", 17, 90, 167),
    Instrument("Polysynth", "polysynth", 0, 90, 168),
    Instrument("Pop Bass", "pop_bass", 0, 37, 169),
    Instrument("Power 1", "power_1", 128, 17, 170),
    Instrument("Power 2", "power_2", 128, 18, 171),
    Instrument("Power 3", "power_3", 128, 19, 172),
    Instrument("Power", "power", 128, 16, 173),
    // Instrument("Recorder Expressive", "recorder_expr", 17, 74, 174),
    Instrument("Recorder", "recorder", 0, 74, 175),
    // Instrument("Reed Organ Expressive", "reed_organ_expr", 17, 20, 176),
    Instrument("Reed Organ", "reed_organ", 0, 20, 177),
    Instrument("Reverse Cymbal", "reverse_cymbal", 0, 119, 178),
    // Instrument("Rock Organ Expressive", "rock_organ_expr", 17, 18, 179),
    Instrument("Rock Organ", "rock_organ", 0, 18, 180),
    Instrument("Room 1", "room_1", 128, 9, 181),
    Instrument("Room 2", "room_2", 128, 10, 182),
    Instrument("Room 3", "room_3", 128, 11, 183),
    Instrument("Room 4", "room_4", 128, 12, 184),
    Instrument("Room 5", "room_5", 128, 13, 185),
    Instrument("Room 6", "room_6", 128, 14, 186),
    Instrument("Room 7", "room_7", 128, 15, 187),
    Instrument("Room", "room", 128, 8, 188),
    // Instrument("Saw Lead Expressive", "saw_lead_expr", 17, 81, 189),
    Instrument("Saw Lead", "saw_lead", 0, 81, 190),
    Instrument("Sea Shore", "sea_shore", 0, 122, 191),
    // Instrument("Shakuhachi Expressive", "shakuhachi_expr", 17, 77, 192),
    Instrument("Shakuhachi", "shakuhachi", 0, 77, 193),
    Instrument("Shamisen", "shamisen", 0, 106, 194),
    Instrument("Shenai", "shenai", 0, 111, 195),
    // Instrument("Shennai Expressive", "shennai_expr", 17, 111, 196),
    // Instrument("Sine Wave Expressive", "sine_wave_expr", 18, 80, 197),
    Instrument("Sine Wave", "sine_wave", 8, 80, 198),
    Instrument("Sitar", "sitar", 0, 104, 199),
    Instrument("Slap Bass", "slap_bass", 0, 36, 200),
    // Instrument("Slow Violin Expressive", "slow_violin_expr", 18, 40, 201),
    Instrument("Slow Violin", "slow_violin", 8, 40, 202),
    // Instrument("Solo Vox Expressive", "solo_vox_expr", 17, 85, 203),
    Instrument("Solo Vox", "solo_vox", 0, 85, 204),
    // Instrument("Soprano Sax Expressive", "soprano_sax_expr", 17, 64, 205),
    Instrument("Soprano Sax", "soprano_sax", 0, 64, 206),
    // Instrument("Soundtrack Expressive", "soundtrack_expr", 17, 97, 207),
    Instrument("Soundtrack", "soundtrack", 0, 97, 208),
    // Instrument("Space Voice Expressive", "space_voice_expr", 17, 91, 209),
    Instrument("Space Voice", "space_voice", 0, 91, 210),
    // Instrument("Square Lead Expressive", "square_lead_expr", 17, 80, 211),
    Instrument("Square Lead", "square_lead", 0, 80, 212),
    Instrument("Standard 1", "standard_1", 128, 1, 213),
    Instrument("Standard 2", "standard_2", 128, 2, 214),
    Instrument("Standard 3", "standard_3", 128, 3, 215),
    Instrument("Standard 4", "standard_4", 128, 4, 216),
    Instrument("Standard 5", "standard_5", 128, 5, 217),
    Instrument("Standard 6", "standard_6", 128, 6, 218),
    Instrument("Standard 7", "standard_7", 128, 7, 219),
    Instrument("Standard", "standard", 128, 0, 220),
    // Instrument("Star Theme Expressive", "star_theme_expr", 17, 103, 221),
    Instrument("Star Theme", "star_theme", 0, 103, 222),
    Instrument("Steel Drums", "steel_drums", 0, 114, 223),
    Instrument("Steel String Guitar", "steel_string_guitar", 0, 25, 224),
    // Instrument("Strings Fast Expressive", "strings_fast_expr", 17, 48, 225),
    Instrument("Strings Fast", "strings_fast", 0, 48, 226),
    Instrument("Strings Pizzicato", "strings_pizzicato", 0, 45, 227),
    // Instrument("Strings Slow Expressive", "strings_slow_expr", 17, 49, 228),
    Instrument("Strings Slow", "strings_slow", 0, 49, 229),
    // Instrument("Strings Trem Expressive", "strings_trem_expr", 17, 44, 230),
    Instrument("Strings Tremelo", "strings_tremelo", 0, 44, 231),
    // Instrument("Sweep Pad Expressive", "sweep_pad_expr", 17, 95, 232),
    Instrument("Sweep Pad", "sweep_pad", 0, 95, 233),
    // Instrument("Syn. Strings 1 Expressive", "syn_strings_1_expr", 17, 50, 234),
    // Instrument("Syn. Strings 2 Expressive", "syn_stirngs_2_expr", 17, 51, 235),
    // Instrument("Synth Bass 1 Expressive", "synth_bass_1_expr", 17, 62, 236),
    Instrument("Synth Bass 1", "synth_bass_1", 0, 38, 237),
    // Instrument("Synth Bass 2 Expressive", "synth_bass_2_expr", 17, 63, 238),
    Instrument("Synth Bass 2", "synth_bass_2", 0, 39, 239),
    Instrument("Synth Bass 3", "synth_bass_3", 8, 38, 240),
    Instrument("Synth Bass 4", "synth_bass_4", 8, 39, 241),
    Instrument("Synth Brass 1", "synth_brass_1", 0, 62, 242),
    Instrument("Synth Brass 2", "synth_brass_2", 0, 63, 243),
    // Instrument("Synth Brass 3 Expressive", "synth_brass_3_expr", 18, 62, 244),
    Instrument("Synth Brass 3", "synth_brass_3", 8, 62, 245),
    // Instrument("Synth Brass 4 Expressive", "synth_brass_4_expr", 18, 63, 246),
    Instrument("Synth Brass 4", "synth_brass_4", 8, 63, 247),
    Instrument("Synth Drum", "synth_drum", 0, 118, 248),
    Instrument("Synth Strings 1", "synth_strings_1", 0, 50, 249),
    Instrument("Synth Strings 2", "synth_strings_2", 0, 51, 250),
    // Instrument("Synth Strings 3 Expr", "synth_strings_3_Expr", 18, 50, 251),
    Instrument("Synth Strings 3", "synth_strings_3", 8, 50, 252),
    // Instrument("Synth Voice Expressive", "synth_voice_expr", 17, 54, 253),
    Instrument("Synth Voice", "synth_voice", 0, 54, 254),
    Instrument("TR-808", "tr_808", 128, 25, 255),
    Instrument("Taiko Drum", "taiko_drum", 0, 116, 256),
    Instrument("Taisho Koto", "taisho_koto", 8, 107, 257),
    Instrument("Telephone", "telephone", 0, 124, 258),
    Instrument("Temple Blocks", "temple_blocks", 8, 0, 259),
    // Instrument("Tenor Sax Expressive", "tenor_sax_expr", 17, 66, 260),
    Instrument("Tenor Sax", "tenor_sax", 0, 66, 261),
    Instrument("Timpani", "timpani", 0, 47, 262),
    Instrument("Tine Electric Piano", "tine_electric_piano", 0, 4, 263),
    Instrument("Tinker Bell", "tinker_bell", 0, 112, 264),
    // Instrument("Trombone Expressive", "trombone_expr", 17, 57, 265),
    Instrument("Trombone", "trombone", 0, 57, 266),
    // Instrument("Trumpet Expressive", "trumpet_expr", 17, 56, 267),
    Instrument("Trumpet", "trumpet", 0, 56, 268),
    // Instrument("Tuba Expressive", "tuba_expr", 17, 58, 269),
    Instrument("Tuba", "tuba", 0, 58, 270),
    Instrument("Tubular Bells", "tubular_bells", 0, 14, 271),
    Instrument("Ukelele", "ukelele", 8, 24, 272),
    Instrument("Vibraphone", "vibraphone", 0, 11, 273),
    // Instrument("Viola Expressive", "viola_expr", 17, 41, 274),
    Instrument("Viola", "viola", 0, 41, 275),
    // Instrument("Violas Fast Expressive", "violas_fast_expr", 31, 48, 276),
    // Instrument("Violas Fast", "violas_fast", 30, 48, 277),
    // Instrument("Violas Pizzicato", "violas_pizzicato", 30, 45, 278),
    // Instrument("Violas Slow Expressive", "violas_slow_expr", 31, 49, 279),
    // Instrument("Violas Slow", "violas_slow", 30, 49, 280),
    // Instrument("Violas Trem Expressive", "violas_trem_expr", 31, 44, 281),
    // Instrument("Violas Tremolo", "violas_tremolo", 30, 44, 282),
    // Instrument("Violin Expressive", "violin_expr", 17, 40, 283),
    Instrument("Violin", "violin", 0, 40, 284),
    // Instrument("Violins Fast Expressive", "violins_fast_expr", 21, 48, 285),
    // Instrument("Violins Fast", "violins_fast", 20, 48, 286),
    // Instrument("Violins Pizzicato", "violins_pizzicato", 20, 45, 287),
    // Instrument("Violins Slow Expressive", "violins_slow_expr", 21, 49, 288),
    // Instrument("Violins Slow", "violins_slow", 20, 49, 289),
    // Instrument("Violins Trem Expressive", "violins_trem_expr", 21, 44, 290),
    // Instrument("Violins Tremolo", "violins_tremolo", 20, 44, 291),
    // Instrument("Violins2 Fast Expressive", "violins2_fast_expr", 26, 48, 292),
    // Instrument("Violins2 Fast", "violins2_fast", 25, 48, 293),
    // Instrument("Violins2 Pizzicato", "violin2_pizzicato", 25, 45, 294),
    // Instrument("Violins2 Slow Expressive", "violins2_slow_expr", 26, 49, 295),
    // Instrument("Violins2 Slow", "violins2_slow", 25, 49, 296),
    // Instrument("Violins2 Trem Expressive", "violins2_trem_expr", 26, 44, 297),
    // Instrument("Violins2 Tremolo", "violins2_tremolo", 25, 44, 298),
    // Instrument("Voice Oohs Expressive", "voice_oohs_expr", 17, 53, 299),
    Instrument("Voice Oohs", "voice_oohs", 0, 53, 300),
    // Instrument("Warm Pad Expressive", "warm_pad_expr", 17, 89, 301),
    Instrument("Warm Pad", "warm_pad", 0, 89, 302),
    // Instrument("Whistle Expressive", "whistle_expr", 17, 78, 303),
    Instrument("Whistle", "whistle", 0, 78, 304),
    Instrument("Woodblock", "woodblock", 0, 115, 305),
    Instrument("Xylophone", "xylophone", 0, 13, 306),
  };
  return all_instruments;
}

auto Instrument::get_all_instrument_names() -> QString& {
  QString instrument_feed;
  QTextStream orchestra_io(&instrument_feed, QIODeviceBase::WriteOnly);
  orchestra_io << "[";
  auto first_one = true;
  for (const auto &instrument : get_all_instruments()) {
    if (first_one) {
      first_one = false;
    } else {
      orchestra_io << ", ";
    }
    orchestra_io << "\"";
    orchestra_io << instrument.name;
    orchestra_io << "\"";
  }
  orchestra_io << "]";
  orchestra_io.flush();
  static auto all_instrument_names = instrument_feed;
  return all_instrument_names;
}

#pragma once

#include <qstring.h>
#include <csound/csound.hpp>        // for Csound

class QTextStream;
class Song;
class TreeNode;

const auto HALFSTEPS_PER_OCTAVE = 12;
const auto CONCERT_A_FREQUENCY = 440;
const auto CONCERT_A_MIDI = 69;
const auto PERCENT = 100;

class Player : public Csound {
    public:
        double current_key = 0.0;
        double current_volume = 0.0;
        double current_tempo = 0.0;
        double current_time = 0.0;
        int current_instrument_id = 0;
        Song& song;

        explicit Player(Song& song, const QString& output, const QString& format = "");

        void initialize_song();
        void update_with_chord(const TreeNode &node);
        void move_time(const TreeNode& node);
        [[nodiscard]] auto get_beat_duration() const -> double;
        void csound_note_event(QTextStream& output_stream, const TreeNode &node) const;
        void play_song();
};
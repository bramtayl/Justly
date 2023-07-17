#pragma once

#include <csound/csound.hpp>        // for Csound
#include <csound/csPerfThread.hpp>  // for CsoundPerformanceThread

class Player;
class TreeNode;

class Performer : public CsoundPerformanceThread {
    public:
        Player& player;

        explicit Performer(Player& player_input);

        ~Performer();
        // prevent moving and copying;
        Performer(const Performer &) = delete;
        auto operator=(const Performer &) -> Performer = delete;
        Performer(Performer &&) = delete;
        auto operator=(Performer &&) -> Performer = delete;

        void play(int first_index, int number_of_children,
                  const TreeNode &parent_node);
        void stop_playing();
        void schedule_note(const TreeNode &node);
     
};
#pragma once

#include <csound/csound.hpp>        // for Csound
#include <csound/csPerfThread.hpp>  // for CsoundPerformanceThread

class Performer : public CsoundPerformanceThread {
    public:
        explicit Performer(Csound* player_pointer_input);
        ~Performer();
        // prevent moving and copying;
        Performer(const Performer &) = delete;
        auto operator=(const Performer &) -> Performer = delete;
        Performer(Performer &&) = delete;
        auto operator=(Performer &&) -> Performer = delete;
        void stop_playing();
     
};
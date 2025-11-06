#pragma once

#include "event.h"
#include <array>

namespace gruvbok {

/**
 * Track contains 16 Events (one for each button B1-B16)
 */
class Track {
public:
    Track();

    Event& getEvent(int step);  // step: 0-15
    const Event& getEvent(int step) const;

    void setEvent(int step, const Event& event);
    void clear();

    static constexpr int NUM_EVENTS = 16;

private:
    std::array<Event, NUM_EVENTS> events_;
};

/**
 * Pattern contains 8 Tracks
 */
class Pattern {
public:
    Pattern();

    Track& getTrack(int track_num);  // track_num: 0-7
    const Track& getTrack(int track_num) const;

    Event& getEvent(int track_num, int step);
    const Event& getEvent(int track_num, int step) const;

    void setEvent(int track_num, int step, const Event& event);
    void clear();

    static constexpr int NUM_TRACKS = 8;

private:
    std::array<Track, NUM_TRACKS> tracks_;
};

} // namespace gruvbok

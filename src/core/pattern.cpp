#include "pattern.h"
#ifndef NO_EXCEPTIONS
#include <stdexcept>
#include <iostream>
#endif
#include <algorithm>

namespace gruvbok {

// ============================================================================
// Track
// ============================================================================

Track::Track() {
    clear();
}

Event& Track::getEvent(int step) {
#ifndef NO_EXCEPTIONS
    if (step < 0 || step >= NUM_EVENTS) {
        throw std::out_of_range("Track step out of range");
    }
#endif
    // Clamp to valid range for embedded builds (defensive programming)
    step = std::max(0, std::min(step, NUM_EVENTS - 1));
    return events_[step];
}

const Event& Track::getEvent(int step) const {
#ifndef NO_EXCEPTIONS
    if (step < 0 || step >= NUM_EVENTS) {
        throw std::out_of_range("Track step out of range");
    }
#endif
    // Clamp to valid range for embedded builds (defensive programming)
    step = std::max(0, std::min(step, NUM_EVENTS - 1));
    return events_[step];
}

void Track::setEvent(int step, const Event& event) {
#ifndef NO_EXCEPTIONS
    if (step < 0 || step >= NUM_EVENTS) {
        throw std::out_of_range("Track step out of range");
    }
#endif
    // Clamp to valid range for embedded builds (defensive programming)
    step = std::max(0, std::min(step, NUM_EVENTS - 1));
    events_[step] = event;
}

void Track::clear() {
    for (auto& event : events_) {
        event.clear();
    }
}

// ============================================================================
// Pattern
// ============================================================================

Pattern::Pattern() {
    clear();
}

Track& Pattern::getTrack(int track_num) {
#ifndef NO_EXCEPTIONS
    if (track_num < 0 || track_num >= NUM_TRACKS) {
        throw std::out_of_range("Track number out of range");
    }
#endif
    // Clamp to valid range for embedded builds (defensive programming)
    track_num = std::max(0, std::min(track_num, NUM_TRACKS - 1));
    return tracks_[track_num];
}

const Track& Pattern::getTrack(int track_num) const {
#ifndef NO_EXCEPTIONS
    if (track_num < 0 || track_num >= NUM_TRACKS) {
        throw std::out_of_range("Track number out of range");
    }
#endif
    // Clamp to valid range for embedded builds (defensive programming)
    track_num = std::max(0, std::min(track_num, NUM_TRACKS - 1));
    return tracks_[track_num];
}

Event& Pattern::getEvent(int track_num, int step) {
    return getTrack(track_num).getEvent(step);
}

const Event& Pattern::getEvent(int track_num, int step) const {
    return getTrack(track_num).getEvent(step);
}

void Pattern::setEvent(int track_num, int step, const Event& event) {
    getTrack(track_num).setEvent(step, event);
}

void Pattern::clear() {
    for (auto& track : tracks_) {
        track.clear();
    }
}

} // namespace gruvbok

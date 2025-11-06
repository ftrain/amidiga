#include "pattern.h"
#include <stdexcept>

namespace gruvbok {

// ============================================================================
// Track
// ============================================================================

Track::Track() {
    clear();
}

Event& Track::getEvent(int step) {
    if (step < 0 || step >= NUM_EVENTS) {
        throw std::out_of_range("Track step out of range");
    }
    return events_[step];
}

const Event& Track::getEvent(int step) const {
    if (step < 0 || step >= NUM_EVENTS) {
        throw std::out_of_range("Track step out of range");
    }
    return events_[step];
}

void Track::setEvent(int step, const Event& event) {
    if (step < 0 || step >= NUM_EVENTS) {
        throw std::out_of_range("Track step out of range");
    }
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
    if (track_num < 0 || track_num >= NUM_TRACKS) {
        throw std::out_of_range("Track number out of range");
    }
    return tracks_[track_num];
}

const Track& Pattern::getTrack(int track_num) const {
    if (track_num < 0 || track_num >= NUM_TRACKS) {
        throw std::out_of_range("Track number out of range");
    }
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

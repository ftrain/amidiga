#include "desktop_hardware.h"
#include "RtMidi.h"
#include <iostream>
#include <iomanip>

namespace gruvbok {

DesktopHardware::DesktopHardware()
    : midi_out_(nullptr)
    , led_state_(false)
    , midi_initialized_(false)
    , current_port_(-1) {

    buttons_.fill(false);
    rotary_pots_.fill(64);  // Default to middle value
    slider_pots_.fill(64);
}

DesktopHardware::~DesktopHardware() {
    shutdown();
}

bool DesktopHardware::init() {
    start_time_ = std::chrono::steady_clock::now();

    // Initialize RtMidi
    try {
        midi_out_ = std::make_unique<RtMidiOut>();

        unsigned int port_count = midi_out_->getPortCount();

        if (port_count == 0) {
            addLog("No MIDI ports available. Creating virtual port.");
            midi_out_->openVirtualPort("GRUVBOK Output");
            current_port_ = -1;  // Virtual port
        } else {
            addLog("Found " + std::to_string(port_count) + " MIDI port(s)");
            // Auto-open first port by default
            std::string port_name = midi_out_->getPortName(0);
            midi_out_->openPort(0);
            current_port_ = 0;
            addLog("Opened MIDI port 0: " + port_name);
        }

        midi_initialized_ = true;
        addLog("Hardware initialized successfully");
        return true;

    } catch (RtMidiError& error) {
        addLog("RtMidi error: " + error.getMessage());
        midi_initialized_ = false;
        return false;
    }
}

void DesktopHardware::shutdown() {
    if (midi_out_ && midi_initialized_) {
        midi_out_->closePort();
    }
}

bool DesktopHardware::readButton(int button) {
    if (button < 0 || button >= 16) {
        return false;
    }
    return buttons_[button];
}

uint8_t DesktopHardware::readRotaryPot(int pot) {
    if (pot < 0 || pot >= 4) {
        return 0;
    }
    return rotary_pots_[pot];
}

uint8_t DesktopHardware::readSliderPot(int pot) {
    if (pot < 0 || pot >= 4) {
        return 0;
    }
    return slider_pots_[pot];
}

void DesktopHardware::sendMidiMessage(const MidiMessage& msg) {
    if (!midi_initialized_ || !midi_out_) {
        return;
    }

    try {
        midi_out_->sendMessage(&msg.data);
        // No console spam!

    } catch (RtMidiError& error) {
        addLog("Error sending MIDI: " + error.getMessage());
    }
}

void DesktopHardware::setLED(bool on) {
    led_state_ = on;
    // No console spam for LED!
}

uint32_t DesktopHardware::getMillis() {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
    return static_cast<uint32_t>(duration.count());
}

void DesktopHardware::update() {
    // Nothing to update for desktop (keyboard input handled elsewhere)
}

void DesktopHardware::simulateButton(int button, bool pressed) {
    if (button >= 0 && button < 16) {
        buttons_[button] = pressed;
    }
}

void DesktopHardware::simulateRotaryPot(int pot, uint8_t value) {
    if (pot >= 0 && pot < 4) {
        rotary_pots_[pot] = std::min(value, static_cast<uint8_t>(127));
    }
}

void DesktopHardware::simulateSliderPot(int pot, uint8_t value) {
    if (pot >= 0 && pot < 4) {
        slider_pots_[pot] = std::min(value, static_cast<uint8_t>(127));
    }
}

// MIDI port management
int DesktopHardware::getMidiPortCount() {
    if (!midi_out_) return 0;
    return static_cast<int>(midi_out_->getPortCount());
}

std::string DesktopHardware::getMidiPortName(int port) {
    if (!midi_out_ || port < 0 || port >= getMidiPortCount()) {
        return "";
    }
    try {
        return midi_out_->getPortName(static_cast<unsigned int>(port));
    } catch (RtMidiError&) {
        return "";
    }
}

bool DesktopHardware::selectMidiPort(int port) {
    if (!midi_out_) return false;

    try {
        // Close existing port if open
        if (midi_initialized_ && current_port_ >= 0) {
            midi_out_->closePort();
        }

        if (port < 0) {
            // Virtual port
            midi_out_->openVirtualPort("GRUVBOK Output");
            current_port_ = -1;
            addLog("Opened virtual MIDI port");
        } else if (port < getMidiPortCount()) {
            midi_out_->openPort(static_cast<unsigned int>(port));
            current_port_ = port;
            addLog("Opened MIDI port: " + getMidiPortName(port));
        } else {
            return false;
        }

        midi_initialized_ = true;
        return true;

    } catch (RtMidiError& error) {
        addLog("Error selecting MIDI port: " + error.getMessage());
        return false;
    }
}

// Logging
void DesktopHardware::addLog(const std::string& message) {
    log_messages_.push_back(message);
    if (log_messages_.size() > MAX_LOG_MESSAGES) {
        log_messages_.pop_front();
    }
    // GUI-only logging - no console spam!
}

void DesktopHardware::clearLog() {
    log_messages_.clear();
}

} // namespace gruvbok
